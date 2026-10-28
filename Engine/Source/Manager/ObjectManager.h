#pragma once

#include <vector>
#include <memory>
#include <string>

#include "Engine/Source/Utility/ObjectIDManipulator.h"

#include "Engine/Source/GameObject/GameObject.h"

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
		std::vector<uint64_t> m_PendingRemoveIDs;

		General::GameObject* m_SelectedObject = nullptr;

	public:
		
		// 定義は .cpp に置く（モジュールをまたいでも実体が1つになるように）
		static ObjectManager* GetInstance();

		General::GameObject* CreateObject();

		void AddObject(std::unique_ptr<General::GameObject> object);

		void RemoveObject(const uint64_t id);

		// 描画コマンド記録中でも安全に呼べる削除要求。実際の破棄はFlushRemovedObjectsで行う
		void RequestRemoveObject(const uint64_t id);
		void FlushRemovedObjects();

		bool IsValid(const uint64_t id) const;

		const std::vector<GameObjectEntry>& GetObjects() const {
			return m_Objects;
		}

		bool CheckObjectExists(const std::string& name, const General::GameObject* exclude = nullptr) const;

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


