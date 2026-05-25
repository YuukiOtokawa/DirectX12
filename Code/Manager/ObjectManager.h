#pragma once

#include <vector>
#include <memory>
#include <string>

#include "../Utility/ObjectIDManipulator.h"

#include "../GameObject/GameObject.h"

namespace EngineCore::General {
	class GameObject;
}

namespace EngineCore::Manager {
	using namespace General;

	class ObjectManager {
	public:
		struct GameObjectEntry {
			std::unique_ptr<General::GameObject> object;
			uint32_t generation;

		};

	private:

		static ObjectManager* _Instance;

		std::vector<GameObjectEntry> _Objects;
		std::vector<Index> _FreeEntries;

		General::GameObject* _SelectedObject = nullptr;

	public:
		
		static ObjectManager* GetInstance() {
			if (!_Instance) {
				_Instance = new ObjectManager();
			}
			return _Instance;
		}

		General::GameObject* CreateObject();

		void AddObject(std::unique_ptr<General::GameObject> object);

		void RemoveObject(const uint64_t id);

		bool IsValid(const uint64_t id) const;

		const std::vector<GameObjectEntry>& GetObjects() const {
			return _Objects;
		}

		bool CheckObjectExists(const std::string& name) const;

		void SelectObject(General::GameObject* object) {
			_SelectedObject = object;
		}

		General::GameObject* GetSelectedObject() const {
			return _SelectedObject;
		}

		void UpdateObjects();
		void DrawObjects();
	};

}


