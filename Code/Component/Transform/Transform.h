#pragma once

#include "Component/Component.h"
#include "Utility/VectorClass.h"

namespace EngineCore::Component {
	class Transform : Component {
	public:
		Vector3 Position;
		Vector3 Rotation;
		Vector3 Scale;
		Vector4 Quaternion;
		Transform() : Position(0, 0, 0), Rotation(0, 0, 0), Scale(1, 1, 1), Quaternion(0, 0, 0, 1) {}

		void Update() override;
	};
}