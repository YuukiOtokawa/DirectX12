#include "Main.h"
#include "GameManager.h"

#include "ImGui/Code/imgui.h"
#include "Resource/resource.h"

#include "Code/GUIController/DefaultWindowController.h"
#include "Code/Component/Camera/Camera.h"
#include "Code/Component/Transform/Transform.h"
#include "Code/Component/Polygon/EditorCameraController.h"
#include "Code/Utility/WorldInitializer.h"
#include "Code/Component/PostProcessComponent.h"

namespace EngineManager {

	GameManager* GameManager::m_Instance = nullptr;

	GameManager::GameManager() {
		m_Instance = this;
		_ObjectManager = EngineCore::Manager::ObjectManager::GetInstance();
	}

	GameManager::~GameManager() {
		m_Instance = nullptr;
	}

	void GameManager::Initialize() {
		m_ImGuiController.Initialize();
		_WindowManager.Initialize();

		// Create EditorCamera automatically at startup
		auto editorCameraObj = _ObjectManager->CreateObject();
		editorCameraObj->SetName("EditorCamera");
		editorCameraObj->AddComponent<EngineCore::General::Transform>();
		editorCameraObj->AddComponent<EngineCore::General::Camera>();
		editorCameraObj->AddComponent<EngineCore::General::EditorCameraController>();

		// Position editor camera at a nice viewing spot
		auto transform = editorCameraObj->GetComponent<EngineCore::General::Transform>();
		if (transform) {
			transform->SetPosition(Vector3(0.0f, 3.0f, -8.0f));
			transform->SetRotation(Vector3(0.2f, 0.0f, 0.0f)); // Look down slightly
		}

		// Initialize default world objects (plane, light, camera)
		EngineCore::Utility::InitializeWorld(_ObjectManager);

		// Create PostProcessManager GameObject automatically
		auto postProcessObj = _ObjectManager->CreateObject();
		postProcessObj->SetName("PostProcessManager");
		postProcessObj->AddComponent<EngineCore::General::Transform>();
		postProcessObj->AddComponent<EngineCore::General::PostProcessComponent>();
	}

	void GameManager::Update() {
		// Update scene objects
		_ObjectManager->UpdateObjects();
	}

	EngineCore::General::Camera* GameManager::FindGameCamera() {
		auto& objects = _ObjectManager->GetObjects();
		for (const auto& entry : objects) {
			if (entry.object && entry.object->IsActive() && entry.object->GetName() != "EditorCamera") {
				auto camera = entry.object->GetComponent<EngineCore::General::Camera>();
				if (camera) {
					return camera;
				}
			}
		}
		return nullptr;
	}

	EngineCore::General::Camera* GameManager::FindEditorCamera() {
		auto& objects = _ObjectManager->GetObjects();
		for (const auto& entry : objects) {
			if (entry.object && entry.object->IsActive() && entry.object->GetName() == "EditorCamera") {
				auto camera = entry.object->GetComponent<EngineCore::General::Camera>();
				if (camera) {
					return camera;
				}
			}
		}
		return nullptr;
	}

	void GameManager::Draw() {
		// Handle any safely deferred render target resizes
		m_RenderManger.ApplyPendingResizes();

		// --- 1. Game View Render Pass ---
		auto gameCamera = FindGameCamera();
		if (gameCamera) {
			EngineCore::General::Camera::SetActiveCamera(gameCamera);
			m_RenderManger.SetCurrentTarget(EngineCore::Render::RenderManager::RENDER_TARGET_TYPE::GAME_VIEW);
			m_RenderManger.DrawBegin();
			_ObjectManager->DrawObjects();
			m_RenderManger.DrawEnd();
		}
		else {
			// Clear Game View to black if no active game camera exists
			m_RenderManger.SetCurrentTarget(EngineCore::Render::RenderManager::RENDER_TARGET_TYPE::GAME_VIEW);
			m_RenderManger.DrawBegin();
			m_RenderManger.DrawEnd();
		}

		// --- 2. Scene View Render Pass ---
		auto editorCamera = FindEditorCamera();
		if (editorCamera) {
			EngineCore::General::Camera::SetActiveCamera(editorCamera);
			m_RenderManger.SetCurrentTarget(EngineCore::Render::RenderManager::RENDER_TARGET_TYPE::SCENE_VIEW);
			m_RenderManger.DrawBegin();
			_ObjectManager->DrawObjects();
			m_RenderManger.DrawEnd();
		}
		else {
			// Clear Scene View to black if no active editor camera exists
			m_RenderManger.SetCurrentTarget(EngineCore::Render::RenderManager::RENDER_TARGET_TYPE::SCENE_VIEW);
			m_RenderManger.DrawBegin();
			m_RenderManger.DrawEnd();
		}

		// --- 3. ImGui Pass (To Back Buffer) ---
		m_RenderManger.SetCurrentTarget(EngineCore::Render::RenderManager::RENDER_TARGET_TYPE::BACK_BUFFER);
		m_RenderManger.DrawBegin();

		m_ImGuiController.BeginFrame();

		_WindowManager.Draw();
		_WindowManager.DrawWindows();

		m_RenderManger.DrawEnd();
		m_ImGuiController.EndFrame();
		m_RenderManger.FrameEnd();
	}

	void GameManager::EngineManagerMenuBarCommand(WPARAM wParam) {
		switch (LOWORD(wParam)) {
		case ID_WINDOW_NEWWINDOW:
			_WindowManager.NewWindow<GUIController::Window::DefaultWindowController>()->Initialize();
			break;
		}
	}

}
