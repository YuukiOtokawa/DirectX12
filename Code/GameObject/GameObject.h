#pragma once
#include "Object.h"

#include <string>

namespace EngineCore::GameObject {
    class GameObject :
        public EngineCore::Object::Object {

		bool _IsActive = true;
        std::string _Name = "object";
    
	public:

        GameObject();

		virtual void Update();
        void Draw();

		bool IsActive() const { return _IsActive; }
		void SetActive(bool active) { _IsActive = active; }
		std::string GetName() const { return _Name; }
		void SetName(const std::string& name);	
    };

}

namespace EngineCore::Object {
	namespace GameObject = ::EngineCore::GameObject;
}

