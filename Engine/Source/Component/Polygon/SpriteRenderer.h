#pragma once

#include "Engine/Public/EngineAPI.h"
#include "Renderer.h"
#include "RenderManager.h"
#include <memory>
namespace EngineCore::General {
    class ENGINE_API SpriteRenderer :
        public Renderer {

		std::unique_ptr<EngineCore::Render::Types::VERTEX_BUFFER> m_VertexBuffer;
		REGISTER_COMPONENT(SpriteRenderer)
    public:
        SpriteRenderer();

        void Update() override;
		void Draw() override;
    
    };
}

