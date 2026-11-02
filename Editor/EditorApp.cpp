#include "EditorApp.h"

#include "Editor/Resource/resource.h"

#include "Engine/Source/Render/RenderManager.h"
#include "Engine/Source/Manager/ObjectManager.h"
#include "Engine/Source/GameObject/GameObject.h"
#include "Editor/GUIController/DefaultWindowController.h"
#include "Engine/Source/Component/Camera/Camera.h"
#include "Engine/Source/Component/Transform/Transform.h"
#include "Editor/Component/EditorCameraController.h"
#include "Engine/Source/Asset/MeshAsset.h"
#include "Tools/AssetPipeline/ModelImporter.h"

namespace Editor {

	EditorApp* EditorApp::m_Instance = nullptr;

	EditorApp::EditorApp(HWND hWnd) {
		m_Instance = this;
		// エディタは .obj / .fbx を AssetPipeline（assimp）で直接読む。製品は事前変換した .meshbin だけを読む
		EngineCore::Asset::SetMeshImporter(&AssetPipeline::ImportModel);
		// エディタはリポジトリ内のアセット・シェーダーを直接読む（シェーダーのホットリロードのため）
		m_Engine.Initialize(hWnd, EngineCore::ResourcePath::Layout::Repository);
	}

	EditorApp::~EditorApp() {
		m_Instance = nullptr;
	}

	void EditorApp::Initialize(HWND hWnd) {
		m_ImGuiController.Initialize(hWnd);
		m_WindowManager.Initialize();

		// Create EditorCamera automatically at startup
		auto editorCameraObj = m_Engine.GetObjectManager().CreateObject();
		m_EditorCameraID = editorCameraObj->GetID();
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

		m_Engine.CreateDefaultScene();
	}

	void EditorApp::Update() {
		m_Engine.Update();
	}

	EngineCore::General::Camera* EditorApp::FindEditorCamera() {
		if (!m_EditorCameraID) {
			return nullptr;
		}

		auto& objects = m_Engine.GetObjectManager().GetObjects();
		for (const auto& entry : objects) {
			if (entry.object && entry.object->IsActive() && entry.object->GetID() == *m_EditorCameraID) {
				auto camera = entry.object->GetComponent<EngineCore::General::Camera>();
				if (camera) {
					return camera;
				}
			}
		}
		return nullptr;
	}

	void EditorApp::Draw() {
		using EngineCore::Render::RenderTargetType;

		m_Engine.BeginFrame();

		// --- 1. Game View Render Pass ---
		m_Engine.RenderView(m_Engine.FindGameCamera(m_EditorCameraID), RenderTargetType::GAME_VIEW);

		// --- 2. Scene View Render Pass ---
		m_Engine.RenderView(FindEditorCamera(), RenderTargetType::SCENE_VIEW);

		// --- 3. ImGui Pass (To Back Buffer) ---
		auto& renderManager = m_Engine.GetRenderManager();
		renderManager.SetCurrentTarget(RenderTargetType::BACK_BUFFER);
		renderManager.DrawGeometryBegin();

		m_ImGuiController.BeginFrame();

		m_WindowManager.Draw();
		m_WindowManager.DrawWindows();

		renderManager.DrawEnd();
		m_ImGuiController.EndFrame();

		m_Engine.EndFrame();
	}

	void EditorApp::MenuBarCommand(WPARAM wParam) {
		switch (LOWORD(wParam)) {
		case ID_WINDOW_NEWWINDOW:
			m_WindowManager.NewWindow<GUIController::Window::DefaultWindowController>()->Initialize();
			break;
		}
	}

}
