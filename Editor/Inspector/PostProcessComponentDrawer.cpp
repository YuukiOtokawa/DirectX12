#include "InspectorRegistry.h"

#include "ThirdParty/ImGui/Code/imgui.h"
#include "Engine/Source/Component/PostProcessComponent.h"
#include "Engine/Source/Render/RenderManager.h"
#include "Editor/Utility/FilePicker.h"
#include "MaterialPropertyInspector.h"

#include <cstring>
#include <filesystem>

namespace {
	// 「Add New Pass」欄の入力途中の文字列。UI の状態なのでコンポーネントではなく Drawer が持つ
	// （Inspector に同時に表示されるのは選択中の1オブジェクトだけなので、1組を共有する）
	char s_NewName[256] = "";
	char s_NewShaderPath[256] = "";
}

static void DrawPostProcessComponent(EngineCore::General::Component& component) {
	auto& postProcess = static_cast<EngineCore::General::PostProcessComponent&>(component);
	auto& passes = postProcess.GetPasses();
	auto* renderManager = EngineCore::Render::RenderManager::GetInstance();

	ImGui::Text("Active Post-Process Passes:");
	ImGui::Separator();

	int removeIndex = -1;
	for (size_t i = 0; i < passes.size(); ++i) {
		ImGui::PushID((int)i);
		ImGui::Text("%d: %s -> %s", (int)(i + 1), passes[i].name.c_str(), passes[i].shaderPath.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {
			removeIndex = (int)i;
		}

		// パス独自のマテリアルプロパティ（HLSLのcbufferから動的生成）を編集
		if (renderManager) {
			// メタデータはファイル名で登録されるので、表示名ではなくマテリアルのシェーダー名で引く
			const EngineCore::Render::ShaderMetadata* meta = renderManager->GetShaderMetadata(passes[i].material.GetShaderName());
			if (meta && !meta->properties.empty()) {
				ImGui::Indent();
				if (GUIHelper::DrawMaterialProperties(passes[i].material, meta)) {
					// 値が変わったらバッファをRenderManagerへ反映
					postProcess.ApplyPassesToRenderManager();
				}
				ImGui::Unindent();
			}
		}
		ImGui::Separator();
		ImGui::PopID();
	}

	if (removeIndex != -1) {
		postProcess.RemovePass((size_t)removeIndex);
	}

	ImGui::Separator();
	ImGui::Text("Add New Pass:");
	ImGui::InputText("Pass Name", s_NewName, sizeof(s_NewName));
	ImGui::InputText("Shader Path", s_NewShaderPath, sizeof(s_NewShaderPath));
	ImGui::SameLine();
	if (ImGui::Button("Browse...")) {
		COMDLG_FILTERSPEC shaderFilter = { L"HLSL Shader Files (*.hlsl)", L"*.hlsl" };
		std::string selectedPath = OpenFileDialog(&shaderFilter, 1);
		if (!selectedPath.empty()) {
			std::filesystem::path currentPath = std::filesystem::current_path();
			std::filesystem::path fullPath(selectedPath);
			std::filesystem::path relativePath = std::filesystem::relative(fullPath, currentPath);

			std::string relStr = relativePath.generic_string();
			std::string stemName = fullPath.stem().string();

			strcpy_s(s_NewShaderPath, sizeof(s_NewShaderPath), relStr.c_str());
			strcpy_s(s_NewName, sizeof(s_NewName), stemName.c_str());
		}
	}

	if (ImGui::Button("Add and Register")) {
		if (strlen(s_NewName) > 0 && strlen(s_NewShaderPath) > 0) {
			if (postProcess.AddPass(s_NewName, s_NewShaderPath)) {
				// クリア
				s_NewName[0] = '\0';
				s_NewShaderPath[0] = '\0';
			}
		}
	}
}
REGISTER_INSPECTOR(PostProcessComponent, DrawPostProcessComponent)
