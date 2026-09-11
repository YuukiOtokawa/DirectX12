#pragma once

#include "Renderer.h"

// GPU バッファは unique_ptr 越しにしか触らないので、RenderManager.h（= <d3d12.h>）は include しない。
// VERTEX_BUFFER の前方宣言は RenderTypes.h にある。デストラクタは .cpp 側。
#include "../../Render/RenderTypes.h"

#include <memory>
namespace EngineCore::General {
    class SpriteRenderer :
        public Renderer {

		std::unique_ptr<EngineCore::Render::Types::VERTEX_BUFFER> m_VertexBuffer;
		REGISTER_COMPONENT(SpriteRenderer)
    public:
        SpriteRenderer();
		~SpriteRenderer() override;

        void Update() override;
		void Draw() override;

    };
}


