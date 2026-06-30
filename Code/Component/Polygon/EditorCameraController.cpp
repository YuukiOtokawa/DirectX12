#include "EditorCameraController.h"
#include "../../../ImGui/Code/imgui.h"
#include "../../GameObject/GameObject.h"
#include "../Transform/Transform.h"

using namespace EngineCore::General;

EditorCameraController::EditorCameraController() : m_BaseMoveSpeed(0.05f), m_LookSensitivity(0.001f) {}

void EditorCameraController::Draw() {
	// Right click must be held down to control editor camera
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		return;
	}

	auto transform = GetOwner()->GetComponent<Transform>();
	if (!transform) return;

	ImGuiIO& io = ImGui::GetIO();

	// 1. Camera Look Rotation (Right Click + Drag)
	Vector3 rot = transform->GetRotation();
	rot.y += io.MouseDelta.x * m_LookSensitivity; // Yaw (left/right)
	rot.x += io.MouseDelta.y * m_LookSensitivity; // Pitch (up/down)

	// Clamp pitch to prevent camera flipping
	if (rot.x > 1.5f) rot.x = 1.5f;
	if (rot.x < -1.5f) rot.x = -1.5f;

	transform->SetRotation(rot);

	// 2. Camera Translation Movement (WASD + QE)
	float currentSpeed = m_BaseMoveSpeed;
	if (io.KeyShift) {
		currentSpeed *= 3.0f; // Fast move speed (Shift + WASD)
	}

	Vector3 pos = transform->GetPosition();
	Vector3 forward = transform->GetForward();
	Vector3 right = transform->GetRight();

	if (ImGui::IsKeyDown(ImGuiKey_W)) {
		pos = pos + forward * currentSpeed;
	}
	if (ImGui::IsKeyDown(ImGuiKey_S)) {
		pos = pos - forward * currentSpeed;
	}
	if (ImGui::IsKeyDown(ImGuiKey_A)) {
		pos = pos - right * currentSpeed;
	}
	if (ImGui::IsKeyDown(ImGuiKey_D)) {
		pos = pos + right * currentSpeed;
	}
	if (ImGui::IsKeyDown(ImGuiKey_Q)) {
		pos.y = pos.y - currentSpeed; // Downward
	}
	if (ImGui::IsKeyDown(ImGuiKey_E)) {
		pos.y = pos.y + currentSpeed; // Upward
	}

	transform->SetPosition(pos);
}
