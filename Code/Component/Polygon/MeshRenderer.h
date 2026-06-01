#pragma once
#include "Renderer.h"

namespace EngineCore::General {

	class MeshRenderer : public Renderer {
		REGISTER_COMPONENT(MeshRenderer)
	public:
		MeshRenderer() = default;
		~MeshRenderer() override = default;

		void Draw() override;
		void Inspector() override;
	};

}
