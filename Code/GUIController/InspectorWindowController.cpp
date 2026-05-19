#include "InspectorWindowController.h"

#include "Manager/ObjectManager.h"
using namespace EngineCore::Manager;

#include "../GameObject/GameObject.h"
using namespace EngineCore::General;

void GUIController::Window::InspectorWindowController::Draw() {
	auto selectedObject = ObjectManager::GetInstance()->GetSelectedObject();

	if (!selectedObject) {
		ImGui::Text("No object selected.");
		return;
	}


	bool active = selectedObject->IsActive();
	ImGui::Checkbox("###Active", &active);
	selectedObject->SetActive(active);

	std::string name = selectedObject->GetName();
	ImGui::Text("Name: %s", name.c_str());


	const char* label = "Add Component";

	float windowWidth = ImGui::GetContentRegionAvail().x;
	float textWidth = ImGui::CalcTextSize(label).x;

	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	if (ImGui::Button(label)) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup")) {
		if (ImGui::MenuItem("Sprite Renderer")) {
			
		}
		ImGui::EndPopup();
	}


}
