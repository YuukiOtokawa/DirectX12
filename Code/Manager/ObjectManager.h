#pragma once

#include "Object.h"
#include <vector>
#include <memory>

namespace EngineCore::Manager::ObjectManager {
	

	class ObjectManager {
		std::vector<std::shared_ptr<Object::Object>> _Objects;

	};

}


