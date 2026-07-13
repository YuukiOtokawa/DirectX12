#pragma once
#include "Renderer.h"
#include "../../Render/Material.h"

namespace EngineCore::General {

	class MeshRenderer : public Renderer {
		EngineCore::Render::Material m_Material;

		// シャドウマップに深度を書き込むか（スカイドーム等、影を落とさない物はfalse）
		bool m_CastShadows = true;

		REGISTER_COMPONENT(MeshRenderer)
	public:
		MeshRenderer() = default;
		~MeshRenderer() override = default;

		void Draw() override;
		void Inspector() override;

		void SetShader(const std::string &shaderName) {
            m_Material.SetShader(shaderName);
        }

		void SetCastShadows(bool castShadows) { m_CastShadows = castShadows; }
		bool GetCastShadows() const { return m_CastShadows; }

		EngineCore::Render::Material& GetMaterial() { return m_Material; }
		const EngineCore::Render::Material& GetMaterial() const { return m_Material; }
	};

}
