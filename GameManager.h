#pragma once


#include "RenderManager.h"
#include "Code/GUIController/ImGuiController.h"
#include "Code/GUIController/WindowManager.h"

namespace EngineManager {

	using namespace Render;
	using namespace GUIController::ImGuiControl;
	using namespace GUIController::EngineWindow::WindowManager;

	class GameManager {
	private:

		static GameManager* m_Instance;

		RenderManager	m_RenderManger;

		ImGuiController m_ImGuiController;

		WindowManager  _WindowManager;

	public:
		static GameManager* GetInstance() { return m_Instance; }

		GameManager();
		~GameManager();

		void Initialize();

		void Update();
		void Draw();

		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
			if (m_ImGuiController.WindowProcHandler(hWnd, message, wParam, lParam))
				return true;
			return false;
		}

		void EngineManagerMenuBarCommand(WPARAM wParam);
	};

}

