#include "InspectorRegistry.h"

#include "ThirdParty/ImGui/Code/imgui.h"
#include "Engine/Source/Component/Camera/Camera.h"

static void DrawCamera(EngineCore::General::Component& component) {
	auto& camera = static_cast<EngineCore::General::Camera&>(component);

	float fov = camera.GetFov();
	if (ImGui::DragFloat("FOV", &fov, 0.5f, 1.0f, 180.0f, "%.1f")) {
		camera.SetFov(fov);
	}
	float nearZ = camera.GetNear();
	if (ImGui::DragFloat("Near", &nearZ, 0.05f, 0.01f, 10.0f, "%.2f")) {
		camera.SetNear(nearZ);
	}
	float farZ = camera.GetFar();
	if (ImGui::DragFloat("Far", &farZ, 1.0f, 10.0f, 10000.0f, "%.1f")) {
		camera.SetFar(farZ);
	}
	Vector3 target = camera.GetTargetPosition();
	if (ImGui::DragFloat3("Target Position", &target.x, 0.1f, -1000.0f, 1000.0f, "%.1f")) {
		camera.SetTargetPosition(target);
	}
}
REGISTER_INSPECTOR(Camera, DrawCamera)
