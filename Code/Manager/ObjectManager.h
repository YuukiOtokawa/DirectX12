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

		static ObjectManager* m_Instance;

		std::vector<GameObjectEntry> m_Objects;
		std::vector<Index> m_FreeEntries;

		General::GameObject* m_SelectedObject = nullptr;

	public:
		
		static ObjectManager* GetInstance() {
			if (!m_Instance) {
				m_Instance = new ObjectManager();
			}
			return m_Instance;
		}

		General::GameObject* CreateObject();

		void AddObject(std::unique_ptr<General::GameObject> object);

		void RemoveObject(const uint64_t id);

		bool IsValid(const uint64_t id) const;

		const std::vector<GameObjectEntry>& GetObjects() const {
			return m_Objects;
		}

		bool CheckObjectExists(const std::string& name) const;

		void SelectObject(General::GameObject* object) {
			m_SelectedObject = object;
		}

		General::GameObject* GetSelectedObject() const {
			return m_SelectedObject;
		}

		void UpdateObjects();
		void DrawObjects();
	};

}


