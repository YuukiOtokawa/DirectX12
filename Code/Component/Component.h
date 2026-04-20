#pragma once

#include "Object.h"

namespace EngineCore::Object::Component {
	class Component : public EngineCore::Object::Object {
	public:
		void Update() override;
	};

}

