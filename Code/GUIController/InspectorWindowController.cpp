#include "InspectorWindowController.h"

#include "Manager/ObjectManager.h"
using namespace EngineCore::Manager;

#include "../GameObject/GameObject.h"
using namespace EngineCore::General;

void GUIController::Window::InspectorWindowController::Draw() {
	auto selectedObject = ObjectManager::GetInstance()->GetSelectedObject();

	// No object selected
	if (!selectedObject) {
		ImGui::Text("No object selected.");
		return;
	}
	else {
		// Object Inspector
		selectedObject->DrawInspector();
	}






}
