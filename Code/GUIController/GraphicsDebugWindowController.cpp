#include "GraphicsDebugWindowController.h"

#include "../Render/RenderManager.h"

void GUIController::Window::GraphicsDebugWindowController::Draw() {
	ImGui::Text("Color Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetColorBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
    ImGui::Text("Shadow Buffer");
    ImGui::Image((void *)EngineCore::Render::RenderManager::GetInstance()->GetShadowMapBuffer()->SRVHandle.ptr,
                 ImVec2(300.0f, 200.0f));
	ImGui::Text("Normal Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetNormalBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
    ImGui::Text("Position Buffer");
    ImGui::Image((void *)EngineCore::Render::RenderManager::GetInstance()->GetPositionBuffer()->SRVHandle.ptr,
                 ImVec2(300.0f, 200.0f));
	ImGui::Text("Material Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetMaterialBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
	ImGui::Text("Emission Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetEmissionBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
	ImGui::Text("PostProcess Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetPostProcessBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
	ImGui::Text("Lighted Color Buffer");
	ImGui::Image((void*)EngineCore::Render::RenderManager::GetInstance()->GetLightedColorBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
}
