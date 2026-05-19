#include "Transform.h"

void EngineCore::General::Transform::Update() {

}

Vector3 EngineCore::General::Transform::GetForward() const {
	Vector3 forward;
	forward.x = cos(Rotation.y) * cos(Rotation.x);
	forward.y = sin(Rotation.x);
	forward.z = sin(Rotation.y) * cos(Rotation.x);
	return forward.Normalize();
}

Vector3 EngineCore::General::Transform::GetRight() const {
	Vector3 right;
	right.x = cos(Rotation.y - XM_PIDIV2) * cos(Rotation.x);
	right.y = sin(Rotation.x);
	right.z = sin(Rotation.y - XM_PIDIV2) * cos(Rotation.x);
	return right.Normalize();
}

Vector3 EngineCore::General::Transform::GetUp() const {
	Vector3 up;
	up.x = cos(Rotation.y) * cos(Rotation.x - XM_PIDIV2);
	up.y = sin(Rotation.x - XM_PIDIV2);
	up.z = sin(Rotation.y) * cos(Rotation.x - XM_PIDIV2);
	return up.Normalize();
}
