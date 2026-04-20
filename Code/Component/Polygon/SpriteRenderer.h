#pragma once

#include "Renderer.h"
#include "RenderManager.h"
#include <memory>
namespace EngineCore::Object::Component::Renderer {
    using namespace Render::RenderStructure;
    class SpriteRenderer :
        public Renderer {

		std::unique_ptr<VERTEX_BUFFER> _VertexBuffer;
		std::unique_ptr<TEXTURE> _Texture;
    public:
        SpriteRenderer();

        void Update() override;
		void Draw() override;
    
    };
}


