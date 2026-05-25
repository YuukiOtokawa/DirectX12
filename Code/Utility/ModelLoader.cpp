#include "ModelLoader.h"


#include <DirectXMath.h>
using namespace DirectX;

#include <unordered_map>

#include "../Utility/VectorClass.h"

#include "../DirectXTex/DirectXTex.h"
#include <D3DX12.h>
//#pragma comment(lib, "../assimp/lib/assimp-vc145-mt.lib")

using UINT = unsigned int;


struct FBXBone {
	aiMatrix4x4 matrix;
	aiMatrix4x4 animationMatrix;
	aiMatrix4x4 offsetMatrix;
	XMMATRIX finalMatrix;
};

#pragma region Loading Functions

std::vector<ComPtr<ID3D12Resource>> resources;

void LoadVertexData(const aiScene* scene, std::string filename, std::vector<DEFORM_VERTEX>* deformVertices, std::unordered_map<std::string, FBXBone>* boneMap, FBXData* fbxData) {
	auto vertexData = new VertexData * [scene->mNumMeshes];

	for (UINT m = 0; m < scene->mNumMeshes; ++m) {
		aiMesh* mesh = scene->mMeshes[m];

		// Extract vertex data
		std::vector<VERTEX> vertices;

		{
			VERTEX vertex;
			for (UINT v = 0; v < mesh->mNumVertices; ++v) {
				vertex.Position = Vector3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
				vertex.Normal = Vector3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				if (mesh->HasTextureCoords(0)) {
					vertex.TexCoord = Vector2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
				}
				else {
					vertex.TexCoord = Vector2(0.0f, 0.0f);
				}
				vertex.Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
				vertices.push_back(vertex);
			}
		}

		// Extract index data
		std::vector<UINT> indices;

		{
			UINT index;
			for (UINT f = 0; f < mesh->mNumFaces; ++f) {
				aiFace face = mesh->mFaces[f];
				assert(face.mNumIndices == 3 && "Non-triangulated face found");
				for (UINT i = 0; i < face.mNumIndices; ++i) {
					index = face.mIndices[i];
					indices.push_back(index);
				}
			}
		}

		vertexData[m] = new VertexData(filename.c_str(), vertices, indices);

		// Create deform vertices
		for (UINT i = 0; i < vertexData[m]->GetVertices().size(); ++i) {
			DEFORM_VERTEX  deformVertex;
			deformVertex.position = mesh->mVertices[i];
			deformVertex.normal = mesh->mNormals[i];
			deformVertex.boneNum = 0;

			for (UINT b = 0; b < 4; ++b) {
				deformVertex.boneName[b] = "";
				deformVertex.boneWeight[b] = 0.0f;
			}

			deformVertices[m].push_back(deformVertex);
		}

		for (UINT b = 0; b < mesh->mNumBones; ++b) {
			aiBone* bone = mesh->mBones[b];

			boneMap->at(bone->mName.C_Str()).offsetMatrix = bone->mOffsetMatrix;

			for (UINT w = 0; w < bone->mNumWeights; ++w) {
				aiVertexWeight weight = bone->mWeights[w];
				int num = deformVertices[m][weight.mVertexId].boneNum;
				
				deformVertices[m][weight.mVertexId].boneName[num] = bone->mName.C_Str();
				deformVertices[m][weight.mVertexId].boneWeight[num] = weight.mWeight;
				deformVertices[m][weight.mVertexId].boneNum++;

				assert(deformVertices[m][weight.mVertexId].boneNum <= 4 && "A vertex is influenced by more than 4 bones");
			}
		}
	}

	fbxData->vertexData = vertexData;
	fbxData->deformVertex = deformVertices;
}

void LoadTexture(const aiScene* scene, std::unordered_map<std::string, unsigned int>& textureSrvByAssimpPath, FBXData* fbxData) {
	auto render = Render::RenderManager::GetInstance();
	auto device = render->GetDevice();
	auto cmd = render->GetGraphicsCommandList();

	std::vector<std::unique_ptr<TEXTURE>> textures;
	textures.reserve(scene->mNumTextures);
	resources.reserve(scene->mNumTextures);

	for (UINT i=0;i<scene->mNumTextures;++i) {
		aiTexture* texture = scene->mTextures[i];
		if (!texture)			continue;

		HRESULT hr;

		TexMetadata metaData;
		ScratchImage image;

		if (texture->mHeight) {
			hr = LoadFromWICMemory(
				reinterpret_cast<const uint8_t*>(texture->pcData),
				static_cast<size_t>(texture->mWidth),
				WIC_FLAGS_NONE,
				&metaData,
				image
			);
		}
		else {
			hr = image.Initialize2D(
				DXGI_FORMAT_R8G8B8A8_UNORM,
				texture->mWidth,
				texture->mHeight,
				1, 1
			);
			if (SUCCEEDED(hr)) {
				const auto dst = image.GetImage(0, 0, 0);
				const auto src = texture->pcData;

				for (uint32_t y = 0; y < texture->mHeight; ++y) {
					uint8_t* row = dst->pixels + y * dst->rowPitch;
					for (uint32_t x = 0; x < texture->mWidth; ++x) {
						const auto s = src[y * texture->mWidth + x];
						row[x * 4 + 0] = s.r;
						row[x * 4 + 1] = s.g;
						row[x * 4 + 2] = s.b;
						row[x * 4 + 3] = s.a;
					}
				}
			}
		}
		if (FAILED(hr)) continue;

		auto tex = std::make_unique<TEXTURE>();

		ComPtr<ID3D12Resource> resource;
		hr = CreateTexture(render->GetDevice(), metaData, &resource);
		if (FAILED(hr)) continue;

		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		hr = PrepareUpload(device, image.GetImages(),
						   image.GetImageCount(),metaData,
						   subresources);
		if (FAILED(hr)) continue;

		UINT64 uploadSize = GetRequiredIntermediateSize(
			resource.Get(), 0,
			static_cast<UINT>(subresources.size()));
		ComPtr<ID3D12Resource> uploadBuffer;
		{
			auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto bufDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

			hr = device->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&bufDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadBuffer)
			);
			if (FAILED(hr)) continue;
		}

		UpdateSubresources(
			cmd,
			resource.Get(), uploadBuffer.Get(),
			0, 0, static_cast<UINT>(subresources.size()),
			subresources.data()
		);

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		cmd->ResourceBarrier(1, &barrier);

		tex->Resource = resource;
		tex->SRVIndex = render->CreateShaderResourceView(resource.Get());

		// ここで map 登録
		// Assimpの埋め込みテクスチャ参照キーは "*0", "*1", ...
		const std::string embeddedKey = "*" + std::to_string(i);
		textureSrvByAssimpPath[embeddedKey] = tex->SRVIndex;

		// mFilename でも引けるようにしておく（空でなければ）
		if (texture->mFilename.length > 0) {
			textureSrvByAssimpPath[texture->mFilename.C_Str()] = tex->SRVIndex;
		}

		resources.push_back(uploadBuffer);
		textures.push_back(std::move(tex));
	}

	fbxData->textures = std::move(textures);
	fbxData->materials.reserve(scene->mNumMaterials);
}

#include <filesystem>

std::pair<const aiScene*, std::string> LoadAnimation(std::string filename) {
	auto animation = aiImportFile(filename.c_str(), aiProcess_ConvertToLeftHanded);
	assert(animation && "Failed to load animation");

	std::filesystem::path path(filename);

	return { animation, path.stem().string() };
}

void CreateMaterial(
	const aiScene* scene,
	const std::unordered_map<std::string, unsigned int>& textureSrvByAssimpPath, // "*0", "diffuse.png" など
	std::vector<MATERIAL>& materials,
	std::vector<unsigned int>& baseColorTexOfMaterial) {
	materials.clear();
	baseColorTexOfMaterial.clear();

	if (!scene) return;

	materials.resize(scene->mNumMaterials);
	baseColorTexOfMaterial.resize(scene->mNumMaterials, UINT_MAX);

	for (UINT m = 0; m < scene->mNumMaterials; ++m) {
		aiMaterial* src = scene->mMaterials[m];
		if (!src) continue;

		MATERIAL dst{};
		dst.BaseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		dst.EmissionColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		dst.Metallic = 0.0f;
		dst.Specular = 0.5f;
		dst.Roughness = 1.0f;

		aiColor3D c3;
		aiColor4D c4;
		float f = 0.0f;

		// BaseColor / Diffuse
		if (src->Get(AI_MATKEY_BASE_COLOR, c4) == AI_SUCCESS) {
			dst.BaseColor = Vector4(c4.r, c4.g, c4.b, c4.a);
		}
		else if (src->Get(AI_MATKEY_COLOR_DIFFUSE, c3) == AI_SUCCESS) {
			dst.BaseColor = Vector4(c3.r, c3.g, c3.b, dst.BaseColor.w);
		}
		if (src->Get(AI_MATKEY_OPACITY, f) == AI_SUCCESS) {
			dst.BaseColor.w = f;
		}

		// Emissive
		if (src->Get(AI_MATKEY_COLOR_EMISSIVE, c3) == AI_SUCCESS) {
			dst.EmissionColor = Vector4(c3.r, c3.g, c3.b, 1.0f);
		}

		// PBR寄りパラメータ（なければ従来キーでフォールバック）
		if (src->Get(AI_MATKEY_METALLIC_FACTOR, f) == AI_SUCCESS) {
			dst.Metallic = f;
		}
		if (src->Get(AI_MATKEY_ROUGHNESS_FACTOR, f) == AI_SUCCESS) {
			dst.Roughness = f;
		}
		else if (src->Get(AI_MATKEY_SHININESS, f) == AI_SUCCESS) {
		 // Shininess -> Roughness の簡易変換
			dst.Roughness = std::clamp(1.0f - (f / 256.0f), 0.04f, 1.0f);
		}
		if (src->Get(AI_MATKEY_SPECULAR_FACTOR, f) == AI_SUCCESS) {
			dst.Specular = f;
		}

		// BaseColorテクスチャ探索（PBR優先 -> Diffuse）
		aiString texPath;
		aiReturn tr = src->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath);
		if (tr != AI_SUCCESS) {
			tr = src->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
		}

		if (tr == AI_SUCCESS) {
			auto it = textureSrvByAssimpPath.find(texPath.C_Str());
			if (it != textureSrvByAssimpPath.end()) {
				baseColorTexOfMaterial[m] = it->second; // SRVIndex
			}
		}

		materials[m] = dst;
	}
}

void CreateBone(const aiNode* node, std::unordered_map<std::string, FBXBone>* boneMap) {
	FBXBone bone = {};

	boneMap->emplace(node->mName.C_Str(), bone);

	for (UINT n = 0; n < node->mNumChildren; ++n) {
		CreateBone(node->mChildren[n], boneMap);
	}
}
#pragma endregion Loading Functions

FBXData LoadFBX(const char* FileName, VertexData* pVertexData) {
	FBXData model;
	auto bone = new std::unordered_map<std::string, FBXBone>;

	const std::string m_ModelPath(FileName);

	model._Scene = aiImportFile(m_ModelPath.c_str(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
	assert(model._Scene && "Failed to load model");

	auto deformVertex = new std::vector<DEFORM_VERTEX>[model._Scene->mNumMeshes];
	std::unordered_map<std::string, UINT> textureSrvByAssimpPath; // "*0", "diffuse.png" など

	std::vector<MATERIAL> materials;
	std::vector<UINT> baseColorTexOfMaterial;

	// CreateBone
	CreateBone(model._Scene->mRootNode, bone);

	// LoadVertexData
	LoadVertexData(model._Scene, m_ModelPath, deformVertex, bone, &model);

	// LoadTexture
	LoadTexture(model._Scene, textureSrvByAssimpPath, &model);

	// LoadAnimation
	LoadAnimation(FileName);

	// CreateMaterial
	CreateMaterial(model._Scene, textureSrvByAssimpPath, materials, baseColorTexOfMaterial);
	model.baseColorTexOfMaterial = std::move(baseColorTexOfMaterial);

	Render::RenderManager::GetInstance()->WaitGPU();
	
	resources.clear();

	return model;
}
