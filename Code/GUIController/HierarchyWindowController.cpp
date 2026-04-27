#include "HierarchyWindowController.h"

#include "../Manager/ObjectManager.h"

using namespace EngineCore::Manager;

void GUIController::Window::HierarchyWindowController::Draw() {
	if (ImGui::Button("Create Object")) {
		ObjectManager::GetInstance().CreateObject();
	}

	auto& objects = ObjectManager::GetInstance().GetObjects();

	for (auto& object : objects) {
		if (object.get() == ObjectManager::GetInstance().GetSelectedObject()) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		}
		if (ImGui::Button(object->GetName().c_str())) {
			ObjectManager::GetInstance().SelectObject(object.get());
		}
		ImGui::PopStyleColor();
	}
}
