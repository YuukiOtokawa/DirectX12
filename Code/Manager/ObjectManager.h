#pragma once

#include "Object.h"
#include <vector>
#include <memory>

namespace EngineCore::Manager {
	

	class ObjectManager {
		static ObjectManager* _Instance;

		std::vector<std::unique_ptr<EngineCore::Object::Object>> _Objects;



	public:
		static ObjectManager* GetInstance() {
			if (!_Instance) {
				_Instance = new ObjectManager();
			}
			return _Instance;
		}

		std::unique_ptr<EngineCore::Object::Object> CreateObject() {
			std::unique_ptr<EngineCore::Object::Object> object = std::make_unique<EngineCore::Object::Object>();
			_Objects.push_back(std::move(object));
			return object;
		}

		const std::vector<std::unique_ptr<EngineCore::Object::Object>>& GetObjects() const {
			return _Objects;
		}
	};

}


