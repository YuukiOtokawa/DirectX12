#include "PostProcessComponent.h"
#include "RenderManager.h"
#include "../../ImGui/Code/imgui.h"
#include "../Utility/FilePicker.h"
#include "MaterialPropertyInspector.h"
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
	// シェーダ登録済みのこのタイミングで各パスのマテリアルを構成
	for (auto& pass : m_Passes) {
		SetupPassMaterial(pass);
	}
	ApplyPassesToRenderManager();
}

void PostProcessComponent::Update() {
}

void PostProcessComponent::SetupPassMaterial(PostProcessPassInfo& pass) {
	// SetShader が GetShaderMetadata でメタデータを引き、
	// バッファサイズの確定とデフォルト値の初期化を行う
	pass.material.SetShaderFilePath(pass.shaderPath);
	pass.material.SetShader(pass.name);
}

void PostProcessComponent::ApplyPassesToRenderManager() {
	auto* renderManager = Render::RenderManager::GetInstance();
	if (!renderManager) return;

	renderManager->ClearPostProcessPasses();
	for (const auto& pass : m_Passes) {
		// パス独自のプロパティバッファも一緒に渡す（b3にバインドされる）
		renderManager->AddPostProcessPass(pass.name, pass.material.GetBufferData(), pass.material.GetBufferSize());
	}
}

void PostProcessComponent::Inspector() {
	auto* renderManager = Render::RenderManager::GetInstance();

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

		// パス独自のマテリアルプロパティ（HLSLのcbufferから動的生成）を編集
		if (renderManager) {
			const Render::ShaderMetadata* meta = renderManager->GetShaderMetadata(m_Passes[i].name);
			if (meta && !meta->properties.empty()) {
				ImGui::Indent();
				if (GUIHelper::DrawMaterialProperties(m_Passes[i].material, meta)) {
					// 値が変わったらバッファをRenderManagerへ反映
					ApplyPassesToRenderManager();
				}
				ImGui::Unindent();
			}
		}
		ImGui::Separator();
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
					// 登録直後にメタデータが利用可能なのでマテリアルを構成
					SetupPassMaterial(m_Passes.back());
					ApplyPassesToRenderManager();
					// クリア
					m_NewName[0] = '\0';
					m_NewShaderPath[0] = '\0';
				}
			}
		}
	}
}
