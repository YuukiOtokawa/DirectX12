#include "Transform.h"

#include "../../../ImGui/Code/imgui.h"

void EngineCore::General::Transform::Update() {

}

void EngineCore::General::Transform::Inspector() {
	ImGui::DragFloat3("Position", &Position.x, 0.1f);
	ImGui::DragFloat3("Rotation", &Rotation.x, 0.1f);
	ImGui::DragFloat3("Scale", &Scale.x, 0.1f);
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
	right.x = sin(Rotation.y);
	right.y = 0.0f;
	right.z = -cos(Rotation.y);
	return right.Normalize();
}

Vector3 EngineCore::General::Transform::GetUp() const {
	Vector3 up;
	up.x = cos(Rotation.y) * cos(Rotation.x - XM_PIDIV2);
	up.y = sin(Rotation.x - XM_PIDIV2);
	up.z = sin(Rotation.y) * cos(Rotation.x - XM_PIDIV2);
	return up.Normalize();
}
