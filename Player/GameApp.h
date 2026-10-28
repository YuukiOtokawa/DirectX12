#pragma once

#include "Code/Manager/Engine.h"

namespace Player {

	// 製品 exe のアプリケーション。Engine だけを持ち、エディタの UI は一切持たない
	// ゲームカメラの絵を GameView に描いてから、それをバックバッファ全面に表示する
	class GameApp {
		EngineCore::Engine m_Engine;

	public:
		// 描画デバイスを作る。ウィンドウ作成後、表示前に生成する
		explicit GameApp(HWND hWnd);

		// 初期シーンを準備する
		void Initialize();

		void Update();
		void Draw();

		// ウィンドウのクライアント領域の大きさが変わったとき
		void Resize(unsigned int width, unsigned int height);
	};

}
