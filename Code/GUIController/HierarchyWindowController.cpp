#include "HierarchyWindowController.h"

#include "../Manager/ObjectManager.h"

using namespace EngineCore::Manager;

void GUIController::Window::HierarchyWindowController::Draw() {
	if (ImGui::Button("Create Object")) {
		ObjectManager::GetInstance().CreateObject();
	}

	auto& objects = ObjectManager::GetInstance().GetObjects();

	for (auto& object : objects) {
		ImGui::Text("%s", object->GetName().c_str());
	}
}
