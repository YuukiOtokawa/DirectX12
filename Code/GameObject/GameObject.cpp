#include "GameObject.h"

#include "../../ImGui/Code/imgui.h"

#include "../Manager/ObjectManager.h"
using namespace EngineCore::Manager;

#include "../Component/Component.h"

EngineCore::General::GameObject::GameObject() {}

EngineCore::General::GameObject::~GameObject() = default;

void EngineCore::General::GameObject::Update() {}

void EngineCore::General::GameObject::Draw() {}

void EngineCore::General::GameObject::DrawInspector() {
	ImGui::Text(_Name.c_str());
	ImGui::Separator();
	
	for (const auto& component : m_Components) {
		if (component) {
			component->DrawInspector();
			
		}
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

template<typename T>
void EngineCore::General::GameObject::AddComponent() {
	// TがComponentの派生クラスであることを確認
	static_assert(std::is_base_of<Component, T>::value, "T must be a subclass of Component");
	std::unique_ptr<T> component = std::make_unique<T>();
	component->_Owner = this;
	m_Components.push_back(std::move(component));
}

template<typename T>
T* EngineCore::General::GameObject::GetComponent() {
	// TがComponentの派生クラスであることを確認
	static_assert(std::is_base_of<Component, T>::value, "T must be a subclass of Component");

	for (const auto& component : m_Components) {
		if (auto casted = dynamic_cast<T*>(component.get())) {
			return casted;
		}
	}
	return nullptr;
}