#include "ObjectManager.h"

#include "Object.h"
#include "GameObject/GameObject.h"
#include "../Render/RenderSystem.h"

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
		Generation generation;
		if (!_FreeEntries.empty()) {
			i = _FreeEntries.back();
			_FreeEntries.pop_back();
			generation = _Objects[i].generation; // 解放時に進めた世代を引き継ぐ
			_Objects[i].object = std::move(object);
		}
		else {
			i = static_cast<Index>(_Objects.size());
			generation = 0;
			_Objects.push_back(GameObjectEntry{std::move(object), generation});
		}

		// index と generation からハンドル(ID)を生成して紐付ける
		_Objects[i].object->SetID(CreateID(i, generation));
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
		if (_Objects[index].generation != GetGeneration(id)) {
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
		EngineCore::RenderSystem::RenderSystem::GetInstance()->Render(_Objects);
	}

}