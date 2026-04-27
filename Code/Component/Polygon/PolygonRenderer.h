#pragma once

#include "Renderer.h"
#include "RenderManager.h"
#include <memory>
namespace EngineCore::Object::Component::Renderer {
    using namespace Render::RenderStructure;
    class PolygonRenderer :
        public Renderer {

        std::unique_ptr<VERTEX_BUFFER> _VertexBuffer;
        std::unique_ptr<TEXTURE> _Texture;
    public:
        PolygonRenderer();

        void Update() override;
        void Draw() override;

    };
}


