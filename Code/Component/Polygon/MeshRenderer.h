#pragma once
#include "Renderer.h"
#include "Code/Render/Material.h"
#include <DirectXMath.h>

namespace EngineCore::General {

	class MeshRenderer : public Renderer {
		EngineCore::Render::Material m_Material;

		// シャドウマップに深度を書き込むか（スカイドーム等、影を落とさない物はfalse）
		bool m_CastShadows = true;

		REGISTER_COMPONENT(MeshRenderer)
	protected:
		// bindMaterial=false is valid only for consecutive draws of this same material/pass.
		void DrawMesh(const DirectX::XMMATRIX& world, bool bindMaterial = true);

	public:
		MeshRenderer() = default;
		~MeshRenderer() override = default;

		void Draw() override;
		void Inspector() override;

		void SetShaderFile(const std::string &shaderFile) {
            m_Material.SetShaderFile(shaderFile);
        }

		void SetCastShadows(bool castShadows) { m_CastShadows = castShadows; }
		bool GetCastShadows() const { return m_CastShadows; }

		EngineCore::Render::Material& GetMaterial() { return m_Material; }
		const EngineCore::Render::Material& GetMaterial() const { return m_Material; }
	};

}
