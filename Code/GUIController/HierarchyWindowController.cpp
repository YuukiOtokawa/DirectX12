#include "HierarchyWindowController.h"
#include "ObjectNameEditor.h"

#include "../Manager/ObjectManager.h"
using namespace EngineCore::Manager;

#include "../GameObject/GameObject.h"
using namespace EngineCore::General;

void GUIController::Window::HierarchyWindowController::Draw() {

	if (ImGui::Button("Create Object")) {
		ObjectManager::GetInstance()->CreateObject();
	}

	auto& objects = ObjectManager::GetInstance()->GetObjects();

	for (auto& object : objects) {
		auto raw = object.object.get();
		if (!raw) {
			continue;
		}
		const auto objectKey = std::to_string(raw->GetID());
		ImGui::PushID(objectKey.c_str());
		auto isActive = raw->IsActive();
		ImGui::Checkbox("##Active", &isActive);
		raw->SetActive(isActive);
		ImGui::SameLine();
		const bool isSelected = raw == ObjectManager::GetInstance()->GetSelectedObject();
		if (Gui::ObjectNameEditor::Draw(*raw, isSelected, isSelected)) {
			ObjectManager::GetInstance()->SelectObject(raw);
		}
		ImGui::PopID();
	}
}
