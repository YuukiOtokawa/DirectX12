#pragma once

#include "Component.h"
#include <d3d12.h>

#include "RenderManager.h"

namespace EngineCore::General {
	class MeshFilter :
		public Component {
		REGISTER_COMPONENT(MeshFilter)

		std::unique_ptr<Render::Types::VERTEX_BUFFER> _VertexBuffer;

	public:
		MeshFilter();
		void Update() override;

	};
}