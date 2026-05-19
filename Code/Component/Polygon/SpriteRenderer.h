#pragma once

#include "Renderer.h"
#include "RenderManager.h"
#include <memory>
namespace EngineCore::General {
    class SpriteRenderer :
        public Renderer {

		std::unique_ptr<Render::Types::VERTEX_BUFFER> _VertexBuffer;
		REGISTER_COMPONENT(SpriteRenderer)
    public:
        SpriteRenderer();

        void Update() override;
		void Draw() override;
    
    };
}


