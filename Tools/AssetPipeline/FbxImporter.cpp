#include "ModelImporter.h"

#include <cstdio>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using EngineCore::Render::Types::VERTEX;

// FBX（など assimp が読める形式）の取り込み。
// 旧 Engine/Source/Utility/FBXLoader.cpp の LoadFBX は、テクスチャ・マテリアル・ボーン・アニメーションも読んでいたが、
// 呼び出し元（MeshFilter::LoadModel）が使っていたのは 1 つ目のメッシュの頂点とインデックスだけだったので、それだけを取り出す。
namespace AssetPipeline {

	bool ImportFbx(const std::string& sourcePath, EngineCore::Asset::MeshAsset& mesh) {
		const aiScene* scene = aiImportFile(sourcePath.c_str(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
		if (!scene) {
			fprintf(stderr, "Failed to load model: %s (%s)\n", sourcePath.c_str(), aiGetErrorString());
			return false;
		}
		if (scene->mNumMeshes == 0 || !scene->mMeshes[0]) {
			fprintf(stderr, "Model has no mesh: %s\n", sourcePath.c_str());
			aiReleaseImport(scene);
			return false;
		}

		const aiMesh* src = scene->mMeshes[0];

		mesh.vertices.clear();
		mesh.vertices.reserve(src->mNumVertices);
		for (unsigned int v = 0; v < src->mNumVertices; ++v) {
			VERTEX vertex;
			vertex.Position = Vector3(src->mVertices[v].x, src->mVertices[v].y, src->mVertices[v].z);
			if (src->HasNormals()) {
				vertex.Normal = Vector3(src->mNormals[v].x, src->mNormals[v].y, src->mNormals[v].z);
			}
			if (src->HasTextureCoords(0)) {
				vertex.TexCoord = Vector2(src->mTextureCoords[0][v].x, src->mTextureCoords[0][v].y);
			}
			else {
				vertex.TexCoord = Vector2(0.0f, 0.0f);
			}
			vertex.Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			mesh.vertices.push_back(vertex);
		}

		mesh.indices.clear();
		mesh.indices.reserve(static_cast<size_t>(src->mNumFaces) * 3);
		for (unsigned int f = 0; f < src->mNumFaces; ++f) {
			const aiFace& face = src->mFaces[f];
			// aiProcess_Triangulate で三角形になっているはず。点や線の面は飛ばす
			if (face.mNumIndices != 3) continue;
			for (unsigned int i = 0; i < face.mNumIndices; ++i) {
				mesh.indices.push_back(face.mIndices[i]);
			}
		}

		mesh.topology = EngineCore::Render::PrimitiveTopology::TriangleList;
		mesh.textures.clear();

		aiReleaseImport(scene);
		return true;
	}
}
