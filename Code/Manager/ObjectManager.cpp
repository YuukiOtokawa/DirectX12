#include "ObjectManager.h"

#include "Object.h"
#include "GameObject/GameObject.h"
#include "Code/Render/RenderSystem.h"
#include "Code/Render/RenderManager.h"

EngineCore::Manager::ObjectManager* EngineCore::Manager::ObjectManager::m_Instance;

EngineCore::Manager::ObjectManager* EngineCore::Manager::ObjectManager::GetInstance() {
	if (!m_Instance) {
		m_Instance = new ObjectManager();
	}
	return m_Instance;
}

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
		if (!m_FreeEntries.empty()) {
			i = m_FreeEntries.back();
			m_FreeEntries.pop_back();
			generation = m_Objects[i].generation; // 解放時に進めた世代を引き継ぐ
			m_Objects[i].object = std::move(object);
		}
		else {
			i = static_cast<Index>(m_Objects.size());
			generation = 0;
			m_Objects.push_back(GameObjectEntry{std::move(object), generation});
		}

		// index と generation からハンドル(ID)を生成して紐付ける
		m_Objects[i].object->SetID(CreateID(i, generation));
	}

	void ObjectManager::RemoveObject(const uint64_t id) {
		// 世代も確認し、解放済みスロットを再利用した別オブジェクトを消さないようにする
		if (!IsValid(id)) {
			return;
		}
		auto index = GetIndex(id);
		if (m_SelectedObject == m_Objects[index].object.get()) {
			m_SelectedObject = nullptr;
		}
		m_Objects[index].object.reset();
		m_Objects[index].generation++;
		m_FreeEntries.push_back(index);
	}

	void ObjectManager::RequestRemoveObject(const uint64_t id) {
		if (IsValid(id)) {
			m_PendingRemoveIDs.push_back(id);
		}
	}

	void ObjectManager::FlushRemovedObjects() {
		if (m_PendingRemoveIDs.empty()) {
			return;
		}
		// 実行中のコマンドが頂点バッファ等を参照しているため、GPU完了を待ってから破棄する
		auto renderManager = EngineCore::Render::RenderManager::GetInstance();
		if (renderManager) {
			renderManager->WaitGPU();
		}
		for (auto id : m_PendingRemoveIDs) {
			RemoveObject(id);
		}
		m_PendingRemoveIDs.clear();
	}

	bool ObjectManager::IsValid(const uint64_t id) const {
		auto index = GetIndex(id);
		if (index >= m_Objects.size() || !m_Objects[index].object) {
			return false;
		}
		if (m_Objects[index].generation != GetGeneration(id)) {
			return false;
		}
		return true;
	}

	bool ObjectManager::CheckObjectExists(const std::string& name, const General::GameObject* exclude) const {
		for (const auto& object : m_Objects) {
			if (object.object && object.object.get() != exclude && object.object->GetName() == name) {
				return true;
			}
		}
		return false;
	}

	void ObjectManager::UpdateObjects() {
		for (const auto& object : m_Objects) {
			if (object.object) {
				object.object->ExecUpdate();
			}
		}
	}

	void ObjectManager::DrawObjects() {
		if (m_Objects.empty()) {
			return;
		}
		EngineCore::RenderSystem::RenderSystem::GetInstance()->Render(m_Objects);
	}

}
