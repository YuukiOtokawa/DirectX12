#include "ResourcePath.h"

namespace EngineCore::ResourcePath {

	namespace {
		Layout s_Layout = Layout::Repository;

		// 拡張子（最後の '.' 以降）を newExtension に置き換える。ディレクトリ名の '.' は無視する
		std::string ReplaceExtension(const std::string& path, const std::string& newExtension) {
			size_t lastSlash = path.find_last_of("\\/");
			size_t lastDot = path.find_last_of('.');
			if (lastDot == std::string::npos || (lastSlash != std::string::npos && lastDot < lastSlash))
				return path + newExtension;
			return path.substr(0, lastDot) + newExtension;
		}
	}

	void SetLayout(Layout layout) {
		s_Layout = layout;
	}

	Layout GetLayout() {
		return s_Layout;
	}

	std::string Asset(const std::string& fileName) {
		return (s_Layout == Layout::Repository ? "Assets\\" : "Resource\\Asset\\") + fileName;
	}

	std::string Shader(const std::string& fileName) {
		return (s_Layout == Layout::Repository ? "Engine/Shader/" : "Resource/Shader/") + fileName;
	}

	std::string CompiledShader(const std::string& shaderFile, const std::string& entryPoint) {
		return ReplaceExtension(shaderFile, "." + entryPoint + ".cso");
	}

	std::string ShaderMetadataFile(const std::string& shaderFile) {
		return ReplaceExtension(shaderFile, ".shadermeta");
	}

	std::string MeshBin(const std::string& modelFile) {
		return ReplaceExtension(modelFile, ".meshbin");
	}
}
