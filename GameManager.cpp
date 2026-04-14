

#include "Main.h"
#include "GameManager.h"

// ImGuiテスト用 早めに消す
#include "ImGui/Code/imgui.h"

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

		m_ImGuiController.EndFrame();

		m_RenderManger.DrawEnd();

	}


}



