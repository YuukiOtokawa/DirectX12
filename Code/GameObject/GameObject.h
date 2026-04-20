#pragma once
#include "Object.h"

#include <string>

namespace EngineCore::GameObject {
    class GameObject :
        public EngineCore::Object::Object {

        std::string _Name;
    
	public:
        GameObject();

		virtual void Update();
        void Draw();
    };

}

namespace EngineCore::Object {
	namespace GameObject = ::EngineCore::GameObject;
}

