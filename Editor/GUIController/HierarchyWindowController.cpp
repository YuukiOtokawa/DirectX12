#include "HierarchyWindowController.h"
#include "ObjectNameEditor.h"

#include "Engine/Source/Manager/ObjectManager.h"
#include "Editor/EditorApp.h"
using namespace EngineCore::Manager;

#include "Engine/Source/GameObject/GameObject.h"
using namespace EngineCore::General;

void GUIController::Window::HierarchyWindowController::Draw() {

	if (ImGui::Button("Create Object")) {
		ObjectManager::GetInstance()->CreateObject();
	}

	auto& objects = ObjectManager::GetInstance()->GetObjects();

	std::optional<uint64_t> editorCameraID;
	if (auto editorApp = Editor::EditorApp::GetInstance()) {
		editorCameraID = editorApp->GetEditorCameraID();
	}

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
		if (ImGui::BeginPopupContextItem("ObjectContextMenu")) {
			// EditorCameraを消すとシーンビューを復帰できないため削除不可にする
			const bool canDelete = !editorCameraID || raw->GetID() != *editorCameraID;
			if (ImGui::MenuItem("Delete", nullptr, false, canDelete)) {
				ObjectManager::GetInstance()->RequestRemoveObject(raw->GetID());
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	// 項目の上では開かず、何もないところの右クリックだけで開く
	if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_NoOpenOverItems)) {
		if (ImGui::MenuItem("New GameObject")) {
			ObjectManager::GetInstance()->CreateObject();
		}
		ImGui::EndPopup();
	}
}
