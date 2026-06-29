#pragma once
#include "Component.h"
#include "Material.h"
#include <string>
#include <vector>

namespace EngineCore::General {

	struct PostProcessPassInfo {
		std::string name;
		std::string shaderPath;
		Render::Material material; // パス独自のマテリアルプロパティ（HLSLのcbufferから動的生成）
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

	private:
		// メタデータからパスのマテリアルバッファを構成（シェーダ登録後に呼ぶ）
		void SetupPassMaterial(PostProcessPassInfo& pass);
	};

}
