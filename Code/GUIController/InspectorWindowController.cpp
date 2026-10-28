#include "InspectorWindowController.h"

#include "Manager/ObjectManager.h"
#include "Manager/ComponentFactory.h"
using namespace EngineCore::Manager;

#include "Code/GameObject/GameObject.h"
#include "Code/Component/Component.h"
using namespace EngineCore::General;

#include "ObjectNameEditor.h"
#include "Editor/Inspector/InspectorRegistry.h"

void GUIController::Window::InspectorWindowController::Draw() {
	auto selectedObject = ObjectManager::GetInstance()->GetSelectedObject();

	// No object selected
	if (!selectedObject) {
		ImGui::Text("No object selected.");
		return;
	}

	// Active Checkbox
	bool isActive = selectedObject->IsActive();
	if (ImGui::Checkbox("###Active", &isActive)) {
		selectedObject->SetActive(isActive);
	}

	// GameObject Name
	ImGui::TextUnformatted("Name:");
	ImGui::SameLine();
	GUIController::Gui::ObjectNameEditor::Draw(*selectedObject);
	ImGui::Separator();

	for (const auto& component : selectedObject->GetComponents()) {
		if (component) {
			EditorUI::InspectorRegistry::Get().Draw(*component);
		}
	}

	// Add Component Button
	const char* label = "Add Component";

	float windowWidth = ImGui::GetContentRegionAvail().x;
	float textWidth = ImGui::CalcTextSize(label).x;

	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	if (ImGui::Button(label)) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	// Add Component Popup
	if (ImGui::BeginPopup("AddComponentPopup")) {
		auto componentNames = ComponentFactory::GetInstance()->GetRegisteredComponentNames();

		for (const auto& componentName : componentNames) {
			if (ImGui::MenuItem(componentName.c_str())) {
				selectedObject->AddComponent(ComponentFactory::GetInstance()->CreateComponent(componentName));
			}
		}

		ImGui::EndPopup();
	}
}
