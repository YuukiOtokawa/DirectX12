#pragma once
#include "Object.h"

#include <string>

namespace EngineCore::Object::GameObject {
    class GameObject :
        public Object {

        std::string _Name;
    
	public:
        GameObject();

		void Update() override;
        void Draw();
    };

}

