#pragma once

#include "Component.h"

namespace EngineCore::Object::Component::Renderer {
    class Renderer :
        public Component {
            
    public:
		void Update() override;
        virtual void Draw() = 0;

    };

}
