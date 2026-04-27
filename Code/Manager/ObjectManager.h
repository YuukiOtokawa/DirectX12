#pragma once

#include "Object.h"
#include "GameObject/GameObject.h"
#include <vector>
#include <memory>

namespace EngineCore::Manager {
	

	class ObjectManager {
		static ObjectManager _Instance;

		std::vector<std::unique_ptr<EngineCore::GameObject::GameObject>> _Objects;

		EngineCore::GameObject::GameObject* _SelectedObject = nullptr;

	public:
		static ObjectManager& GetInstance() {
			return _Instance;
		}

		EngineCore::GameObject::GameObject* CreateObject() {
			std::unique_ptr<EngineCore::GameObject::GameObject> object = std::make_unique<EngineCore::GameObject::GameObject>();
			auto raw = object.get();
			AddObject(std::move(object));
			return raw;
		}

		void AddObject(std::unique_ptr<EngineCore::GameObject::GameObject> object) {
			if (!object) {
				return;
			}

			object->SetName(object->GetName());

			_Objects.push_back(std::move(object));
		}

		const std::vector<std::unique_ptr<EngineCore::GameObject::GameObject>>& GetObjects() const {
			return _Objects;
		}

		bool CheckObjectExists(const std::string& name) const {
			for (const auto& object : _Objects) {
				if (object->GetName() == name) {
					return true;
				}
			}
			return false;
		}

		void SelectObject(EngineCore::GameObject::GameObject* object) {
			_SelectedObject = object;
		}

		EngineCore::GameObject::GameObject* GetSelectedObject() const {
			return _SelectedObject;
		}
	};

}


