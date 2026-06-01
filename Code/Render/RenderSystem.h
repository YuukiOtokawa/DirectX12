#pragma once
#include <vector>
#include "../Manager/ObjectManager.h"

namespace EngineCore::RenderSystem {

	class RenderSystem {
	private:
		static RenderSystem* _Instance;
		RenderSystem() = default;
	public:
		static RenderSystem* GetInstance() {
			if (!_Instance) {
				_Instance = new RenderSystem();
			}
			return _Instance;
		}

		// Execute render for all active objects
		void Render(const std::vector<EngineCore::Manager::ObjectManager::GameObjectEntry>& objects);
	};

}
