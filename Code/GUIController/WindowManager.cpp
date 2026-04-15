#include "WindowManager.h"

#include "../../ImGui/Code/imgui.h"

#include "ImGuiWindowController.h"
#include <Windows.h>

using namespace GUIController::EngineWindow::WindowManager;
using namespace GUIController::ImGuiControl;

void WindowManager::Initialize() {}

void WindowManager::Draw() {
	ImGui::Begin("WindowManager");

	for (const auto& window : _Windows) {
		if (!window) {
			continue;
		}
		ImGui::PushID(window.get());
		
		ImGui::Checkbox("##isActive", window->GetIsActive());
		ImGui::SameLine();
		ImGui::Text(window->GetName().c_str());
		
		ImGui::PopID();
	}
	ImGui::End();
}

void GUIController::EngineWindow::WindowManager::WindowManager::DrawWindows() {
	for (auto& window : _Windows) {
		if (window && *window->GetIsActive()) {
			window->DrawSystem();
		}
	}
}

void WindowManager::Finalize() {}
