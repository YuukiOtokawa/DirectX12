#include "GameObject.h"

#include "../../ImGui/Code/imgui.h"

#include "../Manager/ObjectManager.h"
using namespace EngineCore::Manager;

#include "../Component/Component.h"

EngineCore::General::GameObject::GameObject() {}

EngineCore::General::GameObject::~GameObject() = default;

void EngineCore::General::GameObject::Update() {}

void EngineCore::General::GameObject::Draw() {
	for (const auto& component : m_Components) {
		if (component) {
			component->Draw();
		}
	}

}

void EngineCore::General::GameObject::DrawInspector() {

	// Active Checkbox
	ImGui::Checkbox("###Active", &_IsActive);

	// GameObject Name
	std::string name = GetName();
	ImGui::Text("Name: %s", name.c_str());
	ImGui::Separator();
	
	for (const auto& component : m_Components) {
		if (component) {
			component->DrawInspector();
			
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
				auto component = ComponentFactory::GetInstance()->CreateComponent(componentName);
				if (component) {
					component->SetOwner(this);
					m_Components.push_back(std::move(component));
				}
			}
		}

		ImGui::EndPopup();
	}

}

void EngineCore::General::GameObject::SetName(const std::string& name) {
	std::string n = name;
	if (n.empty()) {
		n = "object";
	}

	if (ObjectManager::GetInstance()->CheckObjectExists(n)) {
		int i = 1;
		while (ObjectManager::GetInstance()->CheckObjectExists(n + std::to_string(i))) {
			i++;
		}
		n += std::to_string(i);
	}
	_Name = n;
}


