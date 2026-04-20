#pragma once


#include "RenderManager.h"
#include "Code/GUIController/ImGuiController.h"
#include "Code/GUIController/WindowManager.h"
#include "Code/Manager/ObjectManager.h"

namespace EngineManager {

	class GameManager {
	private:

		static GameManager* m_Instance;

		Render::RenderManager	m_RenderManger;

		GUIController::Gui::ImGuiController m_ImGuiController;

		GUIController::Window::WindowManager  _WindowManager;

		EngineCore::Manager::ObjectManager   _ObjectManager;

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

