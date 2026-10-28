#include "InspectorRegistry.h"

#include "ThirdParty/ImGui/Code/imgui.h"
#include "Engine/Source/Component/Polygon/MeshRenderer.h"
#include "Engine/Source/Render/RenderManager.h"
#include "Editor/Utility/FilePicker.h"
#include "MaterialPropertyInspector.h"

static void DrawMeshRenderer(EngineCore::General::Component& component) {
	auto& meshRenderer = static_cast<EngineCore::General::MeshRenderer&>(component);
	auto& material = meshRenderer.GetMaterial();
	auto renderManager = EngineCore::Render::RenderManager::GetInstance();

	bool castShadows = meshRenderer.GetCastShadows();
	if (ImGui::Checkbox("Cast Shadows", &castShadows)) {
		meshRenderer.SetCastShadows(castShadows);
	}

	// --- Render Pass Picker ---
	ImGui::Text("Render Pass");
	ImGui::SameLine();
	int currentPassType = static_cast<int>(material.GetRenderPassType());
	const char* passTypeNames[] = { "Deferred Opaque", "Forward Opaque", "Forward Transparent" };
	if (ImGui::Combo("##RenderPass", &currentPassType, passTypeNames, _countof(passTypeNames))) {
		// PSOは描画時にシェーダー×パス種別で引き直されるので、ここでは種別を変えるだけ
		material.SetRenderPassType(static_cast<EngineCore::Render::RenderPassType>(currentPassType));
	}

	// --- Shader Picker ---
	ImGui::Text("Shader (HLSL)");
	ImGui::SameLine();
	if (ImGui::Button("Select Shader")) {
		COMDLG_FILTERSPEC shaderFilter[] = { { L"HLSL Files (*.hlsl)", L"*.hlsl" }, { L"All Files", L"*.*" } };
		std::string path = OpenFileDialog(shaderFilter, _countof(shaderFilter));
		// 同じファイルを選び直したときも再コンパイルされる（ホットリロード）
		if (!path.empty() && renderManager &&
			renderManager->ReloadPipeline(path, material.GetRenderPassType())) {
			material.SetShaderFile(path);
		}
	}

	// --- Texture Picker ---
	ImGui::Text("Base Color Texture");
	if (material.GetTextureBaseColor() && renderManager) {
		auto handle = renderManager->GetShaderResourceViewHandle(material.GetTextureBaseColor()->SRVIndex);
		ImGui::Image((void*)handle.ptr, ImVec2(64.0f, 64.0f));
		ImGui::SameLine();
	}
	if (ImGui::Button("Select Texture")) {
		COMDLG_FILTERSPEC texFilter[] = { { L"Texture Files", L"*.png;*.jpg;*.tga;*.dds" }, { L"All Files", L"*.*" } };
		std::string path = OpenFileDialog(texFilter, _countof(texFilter));
		if (!path.empty() && renderManager) {
			std::shared_ptr<EngineCore::Render::Types::TEXTURE> tex = renderManager->LoadTexture(path.c_str());
			if (tex) {
				material.SetTextureBaseColor(std::move(tex));
			}
		}
	}

	ImGui::Separator();

	// --- Dynamic Properties UI ---
	const EngineCore::Render::ShaderMetadata* meta = nullptr;
	if (renderManager && !material.GetShaderName().empty()) {
		meta = renderManager->GetShaderMetadata(material.GetShaderName());
	}

	if (meta) {
		ImGui::Text("Shader Properties (%s)", material.GetShaderName().c_str());
		GUIHelper::DrawMaterialProperties(material, meta);
	} else {
		// Legacy Properties UI
		// Base Color
		Vector4 baseColor = material.GetBaseColor();
		float color[4] = { baseColor.x, baseColor.y, baseColor.z, baseColor.w };
		if (ImGui::ColorEdit4("Base Color", color)) {
			material.SetBaseColor(Vector4(color[0], color[1], color[2], color[3]));
		}

		// Emission Color
		Vector4 emissionColor = material.GetEmissionColor();
		float emissive[4] = { emissionColor.x, emissionColor.y, emissionColor.z, emissionColor.w };
		if (ImGui::ColorEdit4("Emission Color", emissive)) {
			material.SetEmissionColor(Vector4(emissive[0], emissive[1], emissive[2], emissive[3]));
		}

		// Metallic
		float metallic = material.GetMetallic();
		if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
			material.SetMetallic(metallic);
		}

		// Roughness
		float roughness = material.GetRoughness();
		if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
			material.SetRoughness(roughness);
		}

		// Specular
		float specular = material.GetSpecular();
		if (ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f)) {
			material.SetSpecular(specular);
		}
	}
}
REGISTER_INSPECTOR(MeshRenderer, DrawMeshRenderer)
