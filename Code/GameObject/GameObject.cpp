#include "GameObject.h"

#include "../Manager/ObjectManager.h"
using namespace EngineCore::Manager;

EngineCore::GameObject::GameObject::GameObject() {}

void EngineCore::GameObject::GameObject::Update() {}

void EngineCore::GameObject::GameObject::Draw() {}

void EngineCore::GameObject::GameObject::SetName(const std::string& name) {
	std::string n = name;
	if (n.empty()) {
		n = "object";
	}

	if (ObjectManager::GetInstance().CheckObjectExists(n)) {
		int i = 1;
		while (ObjectManager::GetInstance().CheckObjectExists(n + std::to_string(i))) {
			i++;
		}
		n += std::to_string(i);
	}
	_Name = n;
}
