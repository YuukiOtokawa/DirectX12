#include "HierarchyWindowController.h"

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
		auto isActive = raw->IsActive();
		ImGui::Checkbox(("###Active" + raw->GetName()).c_str(), &isActive);
		raw->SetActive(isActive);
		ImGui::SameLine();
		if (raw == ObjectManager::GetInstance()->GetSelectedObject()) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		}
		if (ImGui::Button(raw->GetName().c_str())) {
			ObjectManager::GetInstance()->SelectObject(raw);
		}
		ImGui::PopStyleColor();
	}
}
