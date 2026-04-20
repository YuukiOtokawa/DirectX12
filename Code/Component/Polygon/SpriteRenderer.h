#pragma once

#include "Renderer.h"
#include "RenderManager.h"
#include <memory>
namespace EngineCore::Component {
    class SpriteRenderer :
        public Renderer {

		std::unique_ptr<Render::Types::VERTEX_BUFFER> _VertexBuffer;
    public:
        SpriteRenderer();

        void Update() override;
		void Draw() override;
    
    };
}


