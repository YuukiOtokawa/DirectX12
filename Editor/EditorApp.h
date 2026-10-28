#pragma once

#include <cstdint>
#include <optional>

#include "Code/Manager/Engine.h"
#include "Code/GUIController/ImGuiController.h"
#include "Code/GUIController/WindowManager.h"

namespace EngineCore::General {
	class Camera;
}

namespace Editor {

	// エディタ exe のアプリケーション。Engine に加えて ImGui・エディタのウィンドウ・エディタカメラを所有する
	class EditorApp {
		static EditorApp* m_Instance;

		EngineCore::Engine m_Engine;

		GUIController::Gui::ImGuiController m_ImGuiController;
		GUIController::Window::WindowManager m_WindowManager;

		std::optional<std::uint64_t> m_EditorCameraID;

	public:
		static EditorApp* GetInstance() { return m_Instance; }

		// 描画デバイスを作る。ウィンドウ作成後、表示前に生成する
		explicit EditorApp(HWND hWnd);
		~EditorApp();

		// ImGui・エディタのウィンドウ・エディタカメラ・初期シーンを準備する
		void Initialize(HWND hWnd);

		void Update();
		void Draw();

		EngineCore::General::Camera* FindEditorCamera();
		const std::optional<std::uint64_t>& GetEditorCameraID() const { return m_EditorCameraID; }

		bool WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
			return m_ImGuiController.WindowProcHandler(hWnd, message, wParam, lParam);
		}

		void MenuBarCommand(WPARAM wParam);
	};

}
