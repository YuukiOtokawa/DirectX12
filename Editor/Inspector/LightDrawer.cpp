#include "InspectorRegistry.h"

#include "ImGui/Code/imgui.h"
#include "Code/Component/Polygon/Light.h"

#include <cstdlib>

static void DrawLight(EngineCore::General::Component& component) {
	using EngineCore::General::Light;
	auto& light = static_cast<Light&>(component);

	// Color Picker
	Vector4 lightColor = light.GetColor();
	float color[4] = { lightColor.x, lightColor.y, lightColor.z, lightColor.w };
	if (ImGui::ColorEdit4("Light Color", color)) {
		light.SetColor(Vector4(color[0], color[1], color[2], color[3]));
	}

	// Exposure Slider
	float exposure = light.GetExposure();
	if (ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f)) {
		light.SetExposure(exposure);
	}

	// Light Type
	const char* lightTypeNames[] = { "Directional", "Point", "Spot" };
	int currentType = (int)light.GetLightType();
	if (ImGui::Combo("Light Type", &currentType, lightTypeNames, _countof(lightTypeNames))) {
		light.SetLightType((Light::LightType)currentType);
	}

	if (light.GetLightType() == Light::LightType::Directional) {
		float orthoSize = light.GetShadowOrthoSize();
		if (ImGui::DragFloat("Shadow Ortho Size", &orthoSize, 0.5f, 1.0f, 500.0f, "%.1f")) {
			light.SetShadowOrthoSize(orthoSize);
		}
	}
	else {
		float shadowFov = light.GetShadowFov();
		if (ImGui::DragFloat("Shadow Fov", &shadowFov, 0.5f, 1.0f, 180.0f, "%.1f")) {
			light.SetShadowFov(shadowFov);
		}
	}
	float shadowNear = light.GetShadowNear();
	if (ImGui::DragFloat("Shadow Near", &shadowNear, 0.05f, 0.01f, 10.0f, "%.2f")) {
		light.SetShadowNear(shadowNear);
	}
	float shadowFar = light.GetShadowFar();
	if (ImGui::DragFloat("Shadow Far", &shadowFar, 1.0f, 10.0f, 10000.0f, "%.1f")) {
		light.SetShadowFar(shadowFar);
	}
}
REGISTER_INSPECTOR(Light, DrawLight)
