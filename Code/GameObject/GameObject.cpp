#include "GameObject.h"

#include "Code/Manager/ObjectManager.h"
using namespace EngineCore::Manager;

#include "Code/Component/Component.h"

EngineCore::General::GameObject::GameObject() {}

EngineCore::General::GameObject::~GameObject() = default;

void EngineCore::General::GameObject::Update() {}

void EngineCore::General::GameObject::Draw() {
	// グローバル描画順ソート（RenderSystem）に移行したため、ここでは何もしません
}

EngineCore::General::Component* EngineCore::General::GameObject::AddComponent(std::unique_ptr<Component> component) {
	if (!component) {
		return nullptr;
	}
	component->SetOwner(this);
	m_Components.push_back(std::move(component));
	return m_Components.back().get();
}

void EngineCore::General::GameObject::SetName(const std::string& name) {
	std::string n = name;
	if (n.empty()) {
		n = "object";
	}

	if (ObjectManager::GetInstance()->CheckObjectExists(n, this)) {
		int i = 1;
		while (ObjectManager::GetInstance()->CheckObjectExists(n + std::to_string(i), this)) {
			i++;
		}
		n += std::to_string(i);
	}
	m_Name = n;
}

const std::vector<std::unique_ptr<EngineCore::General::Component>>& EngineCore::General::GameObject::GetComponents() const {
	return m_Components;
}


