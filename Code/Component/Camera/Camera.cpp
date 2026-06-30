#include "Camera.h"
#include <cmath>

#include "../Component.h"
#include "RenderManager.h"

#include "../../GameObject/GameObject.h"
#include "../Transform/Transform.h"

// Include ImGui header
#include "../../../ImGui/Code/imgui.h"

using namespace EngineCore::General;
using namespace Render::RenderStructure;

// Static member definition
Camera* Camera::s_ActiveCamera = nullptr;

Camera::Camera() {
	m_ProjectionMatrix = XMMatrixIdentity();
	m_ViewMatrix = XMMatrixIdentity();
	
	m_Fov = 60.0f;
	m_Near = 0.1f;
	m_Far = 1000.0f;
	m_UpVector = Vector3(0.0f, 1.0f, 0.0f);
	m_TargetPosition = Vector3(0.0f, 0.0f, 0.0f);

	m_LastPosition = Vector3(0.0f, 0.0f, 0.0f);
	m_LastRotation = Vector3(0.0f, 0.0f, 0.0f);
	m_LastTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
	m_TargetDistance = 10.0f;
	m_IsInitialized = false;

	if (s_ActiveCamera == nullptr) {
		s_ActiveCamera = this;
	}
}

void Camera::Draw() {
	if (s_ActiveCamera != this) {
		return;
	}

	auto renderManager = Render::RenderManager::GetInstance();
	if (!renderManager) return;

	unsigned int width = 0;
	unsigned int height = 0;
	renderManager->GetActiveTargetSize(width, height);
	if (width <= 0 || height <= 0) return;

	// If rendering into the G-Buffer (GameView or SceneView), the G-Buffer is strictly allocated at a fixed 1920x1080 resolution.
	// We must use a constant 1920x1080 to calculate the aspect ratio for G-Buffer pass rendering.
	if (renderManager->GetCurrentTarget() != Render::RenderManager::RENDER_TARGET_TYPE::BACK_BUFFER) {
		width = 1920;
		height = 1080;
	}

	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_Fov), (float)width / (float)height, m_Near, m_Far);

	auto transform = GetOwner()->GetComponent<Transform>();
	if (transform) {
		if (!m_IsInitialized) {
			m_LastPosition = transform->Position;
			m_LastRotation = transform->Rotation;

			Vector3 toTarget = m_TargetPosition - transform->Position;
			float len = toTarget.Length();
			if (len > 0.001f) {
				m_TargetDistance = len;
			}
			else {
				m_TargetDistance = 10.0f;
			}

			Vector3 forward = transform->GetForward();
			m_TargetPosition = transform->Position + forward * m_TargetDistance;
			m_LastTargetPosition = m_TargetPosition;
			m_IsInitialized = true;
		}
		else {
			bool posChanged = !(transform->Position == m_LastPosition);
			bool rotChanged = !(transform->Rotation == m_LastRotation);
			bool targetChanged = !(m_TargetPosition == m_LastTargetPosition);

			if (posChanged || rotChanged) {
				Vector3 forward = transform->GetForward();
				m_TargetPosition = transform->Position + forward * m_TargetDistance;
			}
			else if (targetChanged) {
				Vector3 toTarget = m_TargetPosition - transform->Position;
				float len = toTarget.Length();
				if (len > 0.001f) {
					m_TargetDistance = len;
					Vector3 dir = toTarget.Normalize();
					float pitch = std::asin(dir.y);
					float yaw = std::atan2(dir.z, dir.x);
					transform->Rotation = Vector3(pitch, yaw, 0.0f);
				}
				else {
					m_TargetDistance = 1.0f;
				}
			}
			else {
				if (transform->Position == m_TargetPosition) {
					Vector3 forward = transform->GetForward();
					m_TargetPosition = transform->Position + forward * m_TargetDistance;
				}
			}
		}

		m_LastPosition = transform->Position;
		m_LastRotation = transform->Rotation;
		m_LastTargetPosition = m_TargetPosition;

		XMFLOAT3 posF3 = { transform->Position.x, transform->Position.y, transform->Position.z };
		XMFLOAT3 targetF3 = { m_TargetPosition.x, m_TargetPosition.y, m_TargetPosition.z };
		XMFLOAT3 upF3 = { m_UpVector.x, m_UpVector.y, m_UpVector.z };

		XMVECTOR pos = XMLoadFloat3(&posF3);
		XMVECTOR eyev = XMLoadFloat3(&targetF3);
		XMVECTOR up = XMLoadFloat3(&upF3);
		m_ViewMatrix = XMMatrixLookAtLH(pos, eyev, up);
	}

	CAMERA_CONSTANT cameraConstant{};
	XMStoreFloat4x4(&cameraConstant.View, XMMatrixTranspose(m_ViewMatrix));
	XMStoreFloat4x4(&cameraConstant.Projection, XMMatrixTranspose(m_ProjectionMatrix));
    cameraConstant.Position =
        transform ? Vector4(transform->Position.x, transform->Position.y, transform->Position.z, 1.0f) : Vector4(0.0f, 0.0f, 0.0f, 1.0f);

	renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::CAMERA, &cameraConstant, sizeof(cameraConstant));
}

void Camera::Inspector() {
	ImGui::DragFloat("FOV", &m_Fov, 0.5f, 1.0f, 180.0f, "%.1f");
	ImGui::DragFloat("Near", &m_Near, 0.05f, 0.01f, 10.0f, "%.2f");
	ImGui::DragFloat("Far", &m_Far, 1.0f, 10.0f, 10000.0f, "%.1f");
    ImGui::DragFloat3("Target Position", &m_TargetPosition.x, 0.1f, -1000.0f,
                      1000.0f, "%.1f");
}
