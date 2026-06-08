#pragma once
#include "Renderer.h"
#include "../../Render/Material.h"

namespace EngineCore::General {

	class MeshRenderer : public Renderer {
		Render::Material m_Material;

		REGISTER_COMPONENT(MeshRenderer)
	public:
		MeshRenderer() = default;
		~MeshRenderer() override = default;

		void Draw() override;
		void Inspector() override;

		Render::Material& GetMaterial() { return m_Material; }
		const Render::Material& GetMaterial() const { return m_Material; }
	};

}
