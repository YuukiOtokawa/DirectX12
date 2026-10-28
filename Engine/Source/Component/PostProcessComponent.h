#pragma once
#include "Engine/Public/EngineAPI.h"
#include "Component.h"
#include "Material.h"
#include <string>
#include <vector>

namespace EngineCore::General {

	struct PostProcessPassInfo {
		std::string name;
		std::string shaderPath;
		EngineCore::Render::Material material; // パス独自のマテリアルプロパティ（HLSLのcbufferから動的生成）
	};

	class ENGINE_API PostProcessComponent : public Component {
		REGISTER_COMPONENT(PostProcessComponent)

	private:
		std::vector<PostProcessPassInfo> m_Passes;

	public:
		PostProcessComponent();
		virtual ~PostProcessComponent() = default;

		void Start() override;
		void Update() override;

		void ApplyPassesToRenderManager();

		std::vector<PostProcessPassInfo>& GetPasses() { return m_Passes; }
		const std::vector<PostProcessPassInfo>& GetPasses() const { return m_Passes; }

		// シェーダーのコンパイルが通ったときだけパスを追加して RenderManager に反映する。追加できたら true
		bool AddPass(const std::string& name, const std::string& shaderPath);
		void RemovePass(size_t index);

	private:
		// メタデータからパスのマテリアルバッファを構成（シェーダ登録後に呼ぶ）
		void SetupPassMaterial(PostProcessPassInfo& pass);
	};

}
