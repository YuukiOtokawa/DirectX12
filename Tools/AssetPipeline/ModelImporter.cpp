#include "ModelImporter.h"

#include <filesystem>

namespace AssetPipeline {

	namespace {
		std::string LowerExtension(const std::string& path) {
			std::string ext = std::filesystem::path(path).extension().string();
			for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
			return ext;
		}
	}

	bool IsModelFile(const std::string& path) {
		const std::string ext = LowerExtension(path);
		return ext == ".obj" || ext == ".fbx";
	}

	bool ImportModel(const std::string& sourcePath, EngineCore::Asset::MeshAsset& mesh) {
		const std::string ext = LowerExtension(sourcePath);
		if (ext == ".obj") return ImportObj(sourcePath, mesh);
		if (ext == ".fbx") return ImportFbx(sourcePath, mesh);
		return false;
	}
}
