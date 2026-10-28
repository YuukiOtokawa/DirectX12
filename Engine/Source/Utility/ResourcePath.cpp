#include "ResourcePath.h"

namespace EngineCore::ResourcePath {

	namespace {
		Layout s_Layout = Layout::Repository;
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
}
