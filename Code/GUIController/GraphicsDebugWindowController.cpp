#include "GraphicsDebugWindowController.h"

#include "../Render/RenderManager.h"

void GUIController::Window::GraphicsDebugWindowController::Draw() {
	ImGui::Text("Color Buffer");
	ImGui::Image((void*)Render::RenderManager::GetInstance()->GetColorBuffer()->SRVHandle.ptr,
				 ImVec2(200.0f, 100.0f));
}
