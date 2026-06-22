#pragma once
#include "Component.h"
#include <string>
#include <vector>

namespace EngineCore::General {

	struct PostProcessPassInfo {
		std::string name;
		std::string shaderPath;
	};

	class PostProcessComponent : public Component {
		REGISTER_COMPONENT(PostProcessComponent)

	private:
		std::vector<PostProcessPassInfo> m_Passes;
		char m_NewName[256];
		char m_NewShaderPath[256];

	public:
		PostProcessComponent();
		virtual ~PostProcessComponent() = default;

		void Start() override;
		void Update() override;
		void Inspector() override;

		void ApplyPassesToRenderManager();
	};

}
