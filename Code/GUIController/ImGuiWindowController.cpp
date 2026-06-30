#include "ImGuiWindowController.h"
#include <Windows.h>

uint32_t GUIController::Gui::ImGuiWindowController::m_WindowIDCounter = 0;

void GUIController::Gui::ImGuiWindowController::DrawSystem() {
	const std::string windowLabel = m_WindowName + "###Window_" + std::to_string(m_WindowID);
	if (ImGui::Begin(windowLabel.c_str(), &m_IsActive)) {
		Draw();
	}
	AfterDraw();

}

void GUIController::Gui::ImGuiWindowController::AfterDraw() {
	ImGui::End();
}


