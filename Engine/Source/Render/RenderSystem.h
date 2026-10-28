#pragma once
#include <vector>
#include "Engine/Source/Manager/ObjectManager.h"

namespace EngineCore::RenderSystem {

	class RenderSystem {
	private:
		static RenderSystem* m_Instance;
		RenderSystem() = default;
	public:
		// 定義は .cpp に置く（モジュールをまたいでも実体が1つになるように）
		static RenderSystem* GetInstance();

		// Execute render for all active objects
		void Render(const std::vector<EngineCore::Manager::ObjectManager::GameObjectEntry>& objects);
	};

}
