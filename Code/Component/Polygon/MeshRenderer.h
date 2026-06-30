#pragma once
#include "Renderer.h"
#include "../../Render/Material.h"

namespace EngineCore::General {

	class MeshRenderer : public Renderer {
		EngineCore::Render::Material m_Material;

		REGISTER_COMPONENT(MeshRenderer)
	public:
		MeshRenderer() = default;
		~MeshRenderer() override = default;

		void Draw() override;
		void Inspector() override;

		void SetShader(const std::string &shaderName) {
            m_Material.SetShader(shaderName);
        }

		EngineCore::Render::Material& GetMaterial() { return m_Material; }
		const EngineCore::Render::Material& GetMaterial() const { return m_Material; }
	};

}
