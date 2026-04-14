#pragma once


#include "RenderManager.h"
#include "Code/GUIController/ImGuiController.h"

namespace EngineManager {

	using namespace Render;
	using namespace GUIController::ImGuiControl;

	class GameManager {
	private:

		static GameManager* m_Instance;

		RenderManager	m_RenderManger;

		ImGuiController m_ImGuiController;

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
	};

}

