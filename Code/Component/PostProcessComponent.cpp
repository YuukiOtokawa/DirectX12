#include "PostProcessComponent.h"
#include "RenderManager.h"
#include "../../ImGui/Code/imgui.h"
#include "../Utility/FilePicker.h"
#include <cstring>
#include <filesystem>

using namespace EngineCore::General;

PostProcessComponent::PostProcessComponent() {
	m_NewName[0] = '\0';
	m_NewShaderPath[0] = '\0';
	
	// デフォルトで InvertColor を登録しておく
	m_Passes.push_back({ "InvertColor", "Code/Shader/InvertColor.hlsl" });
}

void PostProcessComponent::Start() {
	ApplyPassesToRenderManager();
}

void PostProcessComponent::Update() {
}

void PostProcessComponent::ApplyPassesToRenderManager() {
	auto* renderManager = Render::RenderManager::GetInstance();
	if (!renderManager) return;

	renderManager->ClearPostProcessPasses();
	for (const auto& pass : m_Passes) {
		renderManager->AddPostProcessPass(pass.name);
	}
}

void PostProcessComponent::Inspector() {
	ImGui::Text("Active Post-Process Passes:");
	ImGui::Separator();

	int removeIndex = -1;
	for (size_t i = 0; i < m_Passes.size(); ++i) {
		ImGui::PushID((int)i);
		ImGui::Text("%d: %s -> %s", (int)(i + 1), m_Passes[i].name.c_str(), m_Passes[i].shaderPath.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {
			removeIndex = (int)i;
		}
		ImGui::PopID();
	}

	if (removeIndex != -1) {
		m_Passes.erase(m_Passes.begin() + removeIndex);
		ApplyPassesToRenderManager();
	}

	ImGui::Separator();
	ImGui::Text("Add New Pass:");
	ImGui::InputText("Pass Name", m_NewName, sizeof(m_NewName));
	ImGui::InputText("Shader Path", m_NewShaderPath, sizeof(m_NewShaderPath));
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

			strcpy_s(m_NewShaderPath, sizeof(m_NewShaderPath), relStr.c_str());
			strcpy_s(m_NewName, sizeof(m_NewName), stemName.c_str());
		}
	}

	if (ImGui::Button("Add and Register")) {
		if (strlen(m_NewName) > 0 && strlen(m_NewShaderPath) > 0) {
			auto* renderManager = Render::RenderManager::GetInstance();
			if (renderManager) {
				// 動的に登録を試みる
				if (renderManager->RegisterDynamicPostProcess(m_NewName, m_NewShaderPath)) {
					m_Passes.push_back({ m_NewName, m_NewShaderPath });
					ApplyPassesToRenderManager();
					// クリア
					m_NewName[0] = '\0';
					m_NewShaderPath[0] = '\0';
				}
			}
		}
	}
}
