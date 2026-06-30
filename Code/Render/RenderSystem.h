#pragma once
#include <vector>
#include "../Manager/ObjectManager.h"

namespace EngineCore::RenderSystem {

	class RenderSystem {
	private:
		static RenderSystem* m_Instance;
		RenderSystem() = default;
	public:
		static RenderSystem* GetInstance() {
			if (!m_Instance) {
				m_Instance = new RenderSystem();
			}
			return m_Instance;
		}

		// Execute render for all active objects
		void Render(const std::vector<EngineCore::Manager::ObjectManager::GameObjectEntry>& objects);
	};

}
