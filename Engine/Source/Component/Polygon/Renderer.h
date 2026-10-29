#pragma once

#include "Engine/Public/EngineAPI.h"
#include "Component.h"

namespace EngineCore::General {
    class ENGINE_API Renderer :
        public Component {
    public:
		void Update() override;
        virtual void Draw() = 0;

    };

}
