#pragma once

#include "Object.h"

namespace EngineCore::Component {
	class Component : public EngineCore::Object::Object {
	public:
		virtual void Update();
	};

}

