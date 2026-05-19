#pragma once

#include "Component.h"

namespace EngineCore::General {
    class Renderer :
        public Component {
    public:
		void Update() override;
        virtual void Draw() = 0;

    };

}
