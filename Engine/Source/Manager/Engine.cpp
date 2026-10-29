#include "Engine.h"

#include "Engine/Source/Render/RenderManager.h"
#include "Engine/Source/Manager/ObjectManager.h"
#include "Engine/Source/GameObject/GameObject.h"
#include "Engine/Source/Component/Camera/Camera.h"
#include "Engine/Source/Component/Polygon/Light.h"
#include "Engine/Source/Component/Transform/Transform.h"
#include "Engine/Source/Component/PostProcessComponent.h"
#include "Engine/Source/Utility/WorldInitializer.h"

namespace EngineCore {

	Engine::Engine() {
		m_ObjectManager = Manager::ObjectManager::GetInstance();
	}

	Engine::~Engine() = default;

	void Engine::Initialize(HWND hWnd, ResourcePath::Layout layout) {
		ResourcePath::SetLayout(layout);
		m_RenderManager = std::make_unique<Render::RenderManager>(hWnd);
	}

	void Engine::CreateDefaultScene() {
		// Initialize default world objects (plane, light, camera)
		Utility::InitializeWorld(m_ObjectManager);

		// Create PostProcessManager GameObject automatically
		auto postProcessObj = m_ObjectManager->CreateObject();
		postProcessObj->SetName("PostProcessManager");
		postProcessObj->AddComponent<General::Transform>();
		postProcessObj->AddComponent<General::PostProcessComponent>();
	}

	void Engine::Update() {
		// 前フレームのGUIで要求された削除を、コマンド記録前に処理する
		m_ObjectManager->FlushRemovedObjects();

		// Update scene objects
		m_ObjectManager->UpdateObjects();
	}

	void Engine::BeginFrame() {
		// Handle any safely deferred render target resizes
		m_RenderManager->ApplyPendingResizes();
	}

	void Engine::RenderView(General::Camera* camera, Render::RenderTargetType target) {
		if (!camera) {
			// Clear the target to black if no active camera exists
			m_RenderManager->SetCurrentTarget(target);
			m_RenderManager->DrawGeometryBegin();
			m_RenderManager->DrawEnd();
			return;
		}

		// シャドウの箱がこのビューのカメラを追従できるよう、シャドウパスの前に設定する
		General::Camera::SetActiveCamera(camera);
		m_RenderManager->SetCurrentTarget(Render::RenderTargetType::SHADOW_BUFFER);
		for (auto light : FindLights()) {
			if (light) {
				m_RenderManager->DrawGeometryBegin();
				for (int cascade = 0; cascade < General::Light::CASCADE_COUNT; cascade++) {
					m_RenderManager->BeginShadowCascade(cascade);  // アトラス上の枠(ビューポート)を移す
					light->SetCurrentCascade(cascade);           // LightProjectionをこの段の射影に
					light->Draw();
					m_ObjectManager->DrawObjects();
				}
				m_RenderManager->DrawEnd();
			}
		}
		m_RenderManager->SetCurrentTarget(target);
		m_RenderManager->DrawGeometryBegin();
		m_ObjectManager->DrawObjects();
		m_RenderManager->DrawEnd();
	}

	void Engine::EndFrame() {
		m_RenderManager->FrameEnd();
	}

	General::Camera* Engine::FindGameCamera(const std::optional<std::uint64_t>& excludeObjectID) {
		auto& objects = m_ObjectManager->GetObjects();
		for (const auto& entry : objects) {
			if (entry.object && entry.object->IsActive() &&
				(!excludeObjectID || entry.object->GetID() != *excludeObjectID)) {
				auto camera = entry.object->GetComponent<General::Camera>();
				if (camera) {
					return camera;
				}
			}
		}
		return nullptr;
	}

	std::vector<General::Light*> Engine::FindLights() {
		auto& objects = m_ObjectManager->GetObjects();
		std::vector<General::Light*> lights;
		for (const auto& entry : objects) {
			if (entry.object && entry.object->IsActive() &&
				entry.object->GetComponent<General::Light>()) {
				lights.push_back(entry.object->GetComponent<General::Light>());
			}
		}
		return lights;
	}
}
