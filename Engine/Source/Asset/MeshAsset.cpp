#include "MeshAsset.h"

#include <filesystem>

#include "Engine/Source/Render/RenderManager.h"
#include "Engine/Source/Utility/BinaryStream.h"
#include "Engine/Source/Utility/ResourcePath.h"
#include "Engine/Source/Utility/VertexData.h"

namespace EngineCore::Asset {

	namespace {
		constexpr uint32_t kMeshMagic = 0x48534D41; // ファイル先頭のバイト列が "AMSH" になる
		constexpr uint32_t kMeshVersion = 1;

		MeshImporter s_Importer = nullptr;

		bool HasExtension(const std::string& path, const char* extension) {
			std::string ext = std::filesystem::path(path).extension().string();
			return _stricmp(ext.c_str(), extension) == 0;
		}
	}

	bool SaveMeshAsset(const std::string& filePath, const MeshAsset& mesh) {
		std::ofstream out(filePath, std::ios::binary);
		if (!out) return false;

		BinaryStream::WriteHeader(out, kMeshMagic, kMeshVersion);
		BinaryStream::Write(out, static_cast<uint32_t>(mesh.topology));
		BinaryStream::WriteArray(out, mesh.vertices);
		BinaryStream::WriteArray(out, mesh.indices);
		BinaryStream::Write(out, static_cast<uint32_t>(mesh.textures.size()));
		for (const auto& texture : mesh.textures)
			BinaryStream::Write(out, texture);

		return static_cast<bool>(out);
	}

	bool LoadMeshAsset(const std::string& filePath, MeshAsset& mesh) {
		std::ifstream in(filePath, std::ios::binary);
		if (!in || !BinaryStream::ReadHeader(in, kMeshMagic, kMeshVersion)) return false;

		uint32_t topology = 0, textureNum = 0;
		if (!BinaryStream::Read(in, topology)) return false;
		mesh.topology = static_cast<Render::PrimitiveTopology>(topology);
		if (!BinaryStream::ReadArray(in, mesh.vertices)) return false;
		if (!BinaryStream::ReadArray(in, mesh.indices)) return false;
		if (!BinaryStream::Read(in, textureNum)) return false;
		mesh.textures.resize(textureNum);
		for (auto& texture : mesh.textures) {
			if (!BinaryStream::Read(in, texture)) return false;
		}
		return true;
	}

	void SetMeshImporter(MeshImporter importer) {
		s_Importer = importer;
	}

	VertexData* LoadMesh(const std::string& filePath) {
		MeshAsset mesh;
		bool loaded = false;
		if (HasExtension(filePath, ".meshbin")) {
			loaded = LoadMeshAsset(filePath, mesh);
		}
		else if (ResourcePath::GetLayout() == ResourcePath::Layout::Distribution) {
			loaded = LoadMeshAsset(ResourcePath::MeshBin(filePath), mesh);
		}
		else if (s_Importer) {
			loaded = s_Importer(filePath, mesh);
		}

		if (!loaded) {
			OutputDebugStringA(("Failed to load mesh: " + filePath + "\n").c_str());
			return nullptr;
		}

		auto vertexData = new VertexData(filePath.c_str(), mesh.vertices, mesh.indices);
		vertexData->SetPrimitiveTopology(mesh.topology);

		const std::filesystem::path dir = std::filesystem::path(filePath).parent_path();
		std::vector<std::unique_ptr<Render::Types::TEXTURE>> textures;
		for (const auto& name : mesh.textures) {
			auto texture = Render::RenderManager::GetInstance()->LoadTexture((dir / name).string().c_str());
			if (texture)
				textures.push_back(std::move(texture));
		}
		vertexData->SetTextures(std::move(textures));

		return vertexData;
	}
}
