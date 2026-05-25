#include "GraphicsDebugWindowController.h"

#include "../Render/RenderManager.h"

void GUIController::Window::GraphicsDebugWindowController::Draw() {
	ImGui::Text("Color Buffer");
	ImGui::Image((void*)Render::RenderManager::GetInstance()->GetColorBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
	ImGui::Text("Normal Buffer");
	ImGui::Image((void*)Render::RenderManager::GetInstance()->GetNormalBuffer()->SRVHandle.ptr,
				 ImVec2(300.0f, 200.0f));
}
