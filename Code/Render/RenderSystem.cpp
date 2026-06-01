#include "RenderSystem.h"
#include "../Component/Component.h"
#include <algorithm>

namespace EngineCore::RenderSystem {

	RenderSystem* RenderSystem::_Instance = nullptr;

	void RenderSystem::Render(const std::vector<EngineCore::Manager::ObjectManager::GameObjectEntry>& objects) {
		if (objects.empty()) {
			return;
		}

		// 1. Gather all components from active game objects
		std::vector<EngineCore::General::Component*> allComponents;
		for (const auto& entry : objects) {
			if (entry.object && entry.object->IsActive()) {
				for (const auto& component : entry.object->GetComponents()) {
					if (component) {
						allComponents.push_back(component.get());
					}
				}
			}
		}

		// 2. Sort components globally by DrawOrder
		std::sort(allComponents.begin(), allComponents.end(), [](EngineCore::General::Component* a, EngineCore::General::Component* b) {
			return static_cast<int>(a->GetDrawOrder()) < static_cast<int>(b->GetDrawOrder());
		});

		// 3. Draw sorted components
		for (auto* component : allComponents) {
			component->Draw();
		}
	}

}
