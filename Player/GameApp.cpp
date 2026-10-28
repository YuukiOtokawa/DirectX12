#include "GameApp.h"

#include "Code/Render/RenderManager.h"

namespace Player {

	GameApp::GameApp(HWND hWnd) {
		// 製品は実行フォルダ直下の Resource フォルダを読む
		m_Engine.Initialize(hWnd, EngineCore::ResourcePath::Layout::Distribution);
	}

	void GameApp::Initialize() {
		m_Engine.CreateDefaultScene();
	}

	void GameApp::Update() {
		m_Engine.Update();
	}

	void GameApp::Draw() {
		using EngineCore::Render::RenderTargetType;

		m_Engine.BeginFrame();

		// ゲームカメラの絵を GameView に描く（ライティングとポストプロセスはこの経路でしか行われない）
		m_Engine.RenderView(m_Engine.FindGameCamera(), RenderTargetType::GAME_VIEW);

		// GameView をバックバッファ全面に表示する
		auto& renderManager = m_Engine.GetRenderManager();
		renderManager.SetCurrentTarget(RenderTargetType::BACK_BUFFER);
		renderManager.DrawGeometryBegin();
		renderManager.DrawTargetToBackBuffer(RenderTargetType::GAME_VIEW);
		renderManager.DrawEnd();

		m_Engine.EndFrame();
	}

	void GameApp::Resize(unsigned int width, unsigned int height) {
		auto& renderManager = m_Engine.GetRenderManager();
		renderManager.Resize(width, height);
		// GameView をウィンドウと同じ大きさにして、拡縮せずに表示する
		renderManager.ResizeTarget(EngineCore::Render::RenderTargetType::GAME_VIEW, width, height);
	}

}
