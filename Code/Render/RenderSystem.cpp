#include "RenderSystem.h"
#include "../Component/Component.h"
#include "../Component/Polygon/MeshRenderer.h"
#include "RenderManager.h"
#include <algorithm>

namespace EngineCore::RenderSystem {

	RenderSystem* RenderSystem::_Instance = nullptr;

	void RenderSystem::Render(const std::vector<EngineCore::Manager::ObjectManager::GameObjectEntry>& objects) {
		if (objects.empty()) {
			return;
		}

		// 1. Gather and sort components into Deferred Opaque and Forward passes
		std::vector<EngineCore::General::Component*> deferredOpaqueComponents;
		std::vector<EngineCore::General::Component*> forwardComponents;

		for (const auto& entry : objects) {
			if (entry.object && entry.object->IsActive()) {
				for (const auto& component : entry.object->GetComponents()) {
					if (component) {
						auto* meshRenderer = dynamic_cast<EngineCore::General::MeshRenderer*>(component.get());
						if (meshRenderer && 
							(meshRenderer->GetMaterial().GetRenderPassType() == Render::RenderPassType::ForwardOpaque ||
							 meshRenderer->GetMaterial().GetRenderPassType() == Render::RenderPassType::ForwardTransparent)) {
							forwardComponents.push_back(component.get());
						} else {
							deferredOpaqueComponents.push_back(component.get());
						}
					}
				}
			}
		}

		// 2. Sort components globally by DrawOrder within each pass
		auto sortByDrawOrder = [](EngineCore::General::Component* a, EngineCore::General::Component* b) {
			return static_cast<int>(a->GetDrawOrder()) < static_cast<int>(b->GetDrawOrder());
		};
		std::sort(deferredOpaqueComponents.begin(), deferredOpaqueComponents.end(), sortByDrawOrder);
		std::sort(forwardComponents.begin(), forwardComponents.end(), sortByDrawOrder);

		auto* renderManager = Render::RenderManager::GetInstance();

		// 3. Draw Deferred Opaque pass (G-Buffer targets are already bound by DrawBegin)
		for (auto* component : deferredOpaqueComponents) {
			component->Draw();
		}

		if (renderManager) {
			// 4. Resolve Deferred Lighting (composite G-Buffer to final color target)
			renderManager->ResolveDeferredLighting();

			// 5. Begin Forward Pass (bind final target RTV and original depth DSV)
			renderManager->BeginForwardPass();
		}

		// 6. Draw Forward pass (Unlit, transparents, etc.)
		for (auto* component : forwardComponents) {
			component->Draw();
		}

		// 7. Apply Post-Process to the fully composited image (deferred + forward)
		if (renderManager) {
			renderManager->ApplyPostProcess();
		}
	}

}
