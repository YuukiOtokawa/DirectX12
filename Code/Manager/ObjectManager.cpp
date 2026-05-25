#include "ObjectManager.h"

#include "Object.h"
#include "GameObject/GameObject.h"

EngineCore::Manager::ObjectManager* EngineCore::Manager::ObjectManager::_Instance;

namespace EngineCore::Manager {
	using namespace General;

	GameObject* ObjectManager::CreateObject() {
		std::unique_ptr<General::GameObject> object = std::make_unique<General::GameObject>();
		auto raw = object.get();
		AddObject(std::move(object));
		return raw;
	}

	void ObjectManager::AddObject(std::unique_ptr<General::GameObject> object) {
		if (!object) {
			return;
		}

		object->SetName(object->GetName());

		Index i;
		if (!_FreeEntries.empty()) {
			i = _FreeEntries.back();
			_FreeEntries.pop_back();
			_Objects[i].object = { std::move(object) };
		}
		else {
			i = _Objects.size();
			_Objects.push_back(GameObjectEntry{std::move(object), 0});
		}

	}

	void ObjectManager::RemoveObject(const uint64_t id) {
		auto index = GetIndex(id);
		if ((index) >= _Objects.size() || !_Objects[index].object) {
			return;
		}
		_Objects[index].object.reset();
		_Objects[index].generation++;
		_FreeEntries.push_back(index);
	}

	bool ObjectManager::IsValid(const uint64_t id) const {
		auto index = GetIndex(id);
		if (index >= _Objects.size() || !_Objects[index].object) {
			return false;
		}
		if (_Objects[index].generation == GetGeneration(id)) {
			return false;
		}
		return true;
	}

	bool ObjectManager::CheckObjectExists(const std::string& name) const {
		for (const auto& object : _Objects) {
			if (object.object->GetName() == name) {
				return true;
			}
		}
		return false;
	}

	void ObjectManager::UpdateObjects() {
		for (const auto& object : _Objects) {
			if (object.object) {
				object.object->ExecUpdate();
			}
		}
	}

	void ObjectManager::DrawObjects() {
		if (_Objects.empty()) {
			return;
		}
		for (const auto& object : _Objects) {
			if (object.object) {
				object.object->Draw();
			}
		}
	}

}