#include "InspectorRegistry.h"

#include "ThirdParty/ImGui/Code/imgui.h"
#include "Engine/Source/Component/Transform/Transform.h"

static void DrawTransform(EngineCore::General::Component& component) {
	auto& transform = static_cast<EngineCore::General::Transform&>(component);

	ImGui::DragFloat3("Position", &transform.Position.x, 0.1f);
	ImGui::DragFloat3("Rotation", &transform.Rotation.x, 0.1f);
	ImGui::DragFloat3("Scale", &transform.Scale.x, 0.1f);
}
REGISTER_INSPECTOR(Transform, DrawTransform)
