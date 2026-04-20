#pragma once

#include "Object.h"
#include <vector>
#include <memory>

namespace EngineCore::Manager::ObjectManager {
	

	class ObjectManager {
		std::vector<std::unique_ptr<Object::Object>> _Objects;

	public:
		std::unique_ptr<Object::Object> CreateObject() {
			std::unique_ptr<Object::Object> object = std::make_unique<Object::Object>();
			_Objects.push_back(std::move(object));
			return object;
		}

	};

}


