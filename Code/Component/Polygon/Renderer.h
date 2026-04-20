#pragma once

#include "Component.h"

namespace EngineCore::Component {
    class Renderer :
        public Component {
            
    public:
		void Update() override;
        virtual void Draw() = 0;

    };

}
