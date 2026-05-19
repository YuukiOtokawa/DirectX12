#pragma once

#include "Component/Component.h"
#include "Utility/VectorClass.h"

namespace EngineCore::General {
	class Transform : public Component {
		REGISTER_COMPONENT(Transform)
	public:
		Vector3 Position;
		Vector3 Rotation;
		Vector3 Scale;
		Vector4 Quaternion;
		Transform() : Position(0, 0, 0), Rotation(0, 0, 0), Scale(1, 1, 1), Quaternion(0, 0, 0, 1) {}

		void Update() override;

		void SetPosition(const Vector3& position) { Position = position; }
		void SetRotation(const Vector3& rotation) { Rotation = rotation; }
		void SetScale(const Vector3& scale) { Scale = scale; }

		Vector3 GetPosition() const { return Position; }
		Vector3 GetRotation() const { return Rotation; }
		Vector3 GetScale() const { return Scale; }

		Vector3 GetForward() const;
		Vector3 GetRight() const;
		Vector3 GetUp() const;

		float GetDistanceTo(const Transform& other) const;
		float GetZ(const Transform& other) const;
	};
}