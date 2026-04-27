

#include "Main.h"
#include "GameManager.h"

// ImGuiテスト用 早めに消す
#include "ImGui/Code/imgui.h"

#include "Resource/resource.h"

#include "Code/GUIController/DefaultWindowController.h"

namespace EngineManager {

	GameManager* GameManager::m_Instance = nullptr;




	GameManager::GameManager() {
		m_Instance = this;

	}




	GameManager::~GameManager() {
		m_RenderManger.WaitGPU();
	}

	void GameManager::Initialize() {
		m_ImGuiController.Initialize();

		_WindowManager.Initialize();
	}





	void GameManager::Update() {

	}




	void GameManager::Draw() {
		static bool show = true;

		m_RenderManger.DrawBegin();
		m_ImGuiController.BeginFrame();

		if (show) {
			ImGui::Begin("Window", &show);
			ImGui::Text("Hello, world!");
			ImGui::End();
		}

		ImGui::Begin("Settings");
		ImGui::Checkbox("Window Visible", &show);
		ImGui::End();

		_WindowManager.Draw();
		_WindowManager.DrawWindows();

		m_ImGuiController.EndFrame();

		m_RenderManger.DrawEnd();

	}

	void GameManager::EngineManagerMenuBarCommand(WPARAM wParam) {
		switch (LOWORD(wParam)) {
		case ID_WINDOW_NEWWINDOW:
			_WindowManager.NewWindow<GUIController::Window::DefaultWindowController>()->Initialize();
			break;
		default:
			break;
		}
	}


}




