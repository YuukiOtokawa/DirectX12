#include "ImGuiWindowController.h"
#include <Windows.h>

uint32_t GUIController::Gui::ImGuiWindowController::_WindowIDCounter = 0;

void GUIController::Gui::ImGuiWindowController::DrawSystem() {
	const std::string windowLabel = _WindowName + "###Window_" + std::to_string(_WindowID);
	if (ImGui::Begin(windowLabel.c_str(), &_isActive)) {
		Draw();
	}
	AfterDraw();

}

void GUIController::Gui::ImGuiWindowController::AfterDraw() {
	ImGui::End();
}


