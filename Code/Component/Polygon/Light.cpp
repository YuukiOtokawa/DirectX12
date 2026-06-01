#include "Light.h"
#include "../../../ImGui/Code/imgui.h"
#include "../../GameObject/GameObject.h"
#include "../Transform/Transform.h"
#include "Render/RenderManager.h"

using namespace EngineCore::General;

Light::Light() : _Color(1.0f, 1.0f, 1.0f, 1.0f), _Intensity(1.0f) {}

void Light::Draw() {
	auto transform = GetOwner()->GetComponent<Transform>();
	if (!transform) return;

	auto renderManager = Render::RenderManager::GetInstance();
	if (!renderManager) return;

	// Calculate light direction (opposite of transform forward)
	Vector3 forward = transform->GetForward();
	
	Render::RenderStructure::ENV_CONSTANT envConstant{};
	envConstant.LightDirection = Vector4(-forward.x, -forward.y, -forward.z, 0.0f);
	envConstant.LightColor = Vector4(_Color.x * _Intensity, _Color.y * _Intensity, _Color.z * _Intensity, _Color.w);

	renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::ENV, &envConstant, sizeof(envConstant));
}

void Light::Inspector() {
	// Color Picker
	float color[4] = { _Color.x, _Color.y, _Color.z, _Color.w };
	if (ImGui::ColorEdit4("Light Color", color)) {
		_Color = Vector4(color[0], color[1], color[2], color[3]);
	}

	// Intensity Slider
	ImGui::SliderFloat("Intensity", &_Intensity, 0.0f, 10.0f);
}
