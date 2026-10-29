#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "Engine/Source/Manager/Main.h"
#include "Engine/Source/Render/RenderTargetType.h"
#include "Engine/Source/Utility/ResourcePath.h"
#include "Engine/Public/EngineAPI.h"

namespace EngineCore::General {
	class Camera;
	class Light;
}

namespace EngineCore::Manager {
	class ObjectManager;
}

namespace EngineCore::Render {
	class RenderManager;
}

namespace EngineCore {

	// エンジン本体。描画とオブジェクト管理を所有し、エディタ・製品の両方の exe から使う
	// エディタの UI やエディタカメラは持たない。それらは exe 側（EditorApp など）が持つ
	class ENGINE_API Engine {
		std::unique_ptr<Render::RenderManager> m_RenderManager;
		Manager::ObjectManager* m_ObjectManager = nullptr;

	public:
		Engine();
		~Engine();

		// 描画デバイスを作る。ウィンドウ作成後、表示前に呼ぶ
		// layout はアセット・シェーダーの参照先（エディタは Repository、製品は Distribution）。シェーダーのコンパイル前に決める必要がある
		void Initialize(HWND hWnd, ResourcePath::Layout layout);

		// 初期ワールド（地面・ライト・カメラ等）と PostProcessManager を作る
		void CreateDefaultScene();

		void Update();

		void BeginFrame();
		// 1つのカメラで1つのターゲットに、シャドウ込みで描く。camera が nullptr ならターゲットを消去するだけ
		void RenderView(General::Camera* camera, Render::RenderTargetType target);
		void EndFrame();

		// アクティブなカメラのうち、excludeObjectID のオブジェクト以外で最初に見つかったもの
		General::Camera* FindGameCamera(const std::optional<std::uint64_t>& excludeObjectID = std::nullopt);
		std::vector<General::Light*> FindLights();

		Render::RenderManager& GetRenderManager() { return *m_RenderManager; }
		Manager::ObjectManager& GetObjectManager() { return *m_ObjectManager; }
	};
}
