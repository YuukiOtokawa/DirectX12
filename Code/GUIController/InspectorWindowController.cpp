#include "InspectorWindowController.h"

#include "Manager/ObjectManager.h"
using namespace EngineCore::Manager;

void GUIController::Window::InspectorWindowController::Draw() {
	auto selectedObject = ObjectManager::GetInstance().GetSelectedObject();

	if (selectedObject) {
		bool active = selectedObject->IsActive();
		ImGui::Checkbox("###Active", &active);
		selectedObject->SetActive(active);

		std::string name = selectedObject->GetName();
		ImGui::Text("Name: %s", name.c_str());
	}
	else {
		ImGui::Text("No object selected.");
	}
}
