#include "Light.h"
#include "../../../ImGui/Code/imgui.h"
#include "../../GameObject/GameObject.h"
#include "../Transform/Transform.h"
#include "Render/RenderManager.h"

using namespace EngineCore::General;

Light::Light() : m_Color(1.0f, 1.0f, 1.0f, 1.0f), m_Intensity(1.0f) {}

void Light::Draw() {
	auto transform = GetOwner()->GetComponent<Transform>();
	if (!transform) return;

	auto renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return;

	// Calculate light direction (opposite of transform forward)
	Vector3 forward = transform->GetForward();
	
	EngineCore::Render::RenderStructure::ENV_CONSTANT envConstant{};
	envConstant.LightDirection = Vector4(-forward.x, -forward.y, -forward.z, 0.0f);
	envConstant.LightColor = Vector4(m_Color.x * m_Intensity, m_Color.y * m_Intensity, m_Color.z * m_Intensity, m_Color.w);

	renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::ENV, &envConstant, sizeof(envConstant));
}

void Light::Inspector() {
	// Color Picker
	float color[4] = { m_Color.x, m_Color.y, m_Color.z, m_Color.w };
	if (ImGui::ColorEdit4("Light Color", color)) {
		m_Color = Vector4(color[0], color[1], color[2], color[3]);
	}

	// Intensity Slider
	ImGui::SliderFloat("Intensity", &m_Intensity, 0.0f, 10.0f);
}
