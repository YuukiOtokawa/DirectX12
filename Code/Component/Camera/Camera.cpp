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
	_ProjectionMatrix = XMMatrixIdentity();
	_ViewMatrix = XMMatrixIdentity();
	
	_Fov = 60.0f;
	_Near = 0.1f;
	_Far = 1000.0f;
	_UpVector = Vector3(0.0f, 1.0f, 0.0f);
	_TargetPosition = Vector3(0.0f, 0.0f, 0.0f);

	_LastPosition = Vector3(0.0f, 0.0f, 0.0f);
	_LastRotation = Vector3(0.0f, 0.0f, 0.0f);
	_LastTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
	_TargetDistance = 10.0f;
	_IsInitialized = false;

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

	_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(_Fov), (float)width / (float)height, _Near, _Far);

	auto transform = GetOwner()->GetComponent<Transform>();
	if (transform) {
		if (!_IsInitialized) {
			_LastPosition = transform->Position;
			_LastRotation = transform->Rotation;

			Vector3 toTarget = _TargetPosition - transform->Position;
			float len = toTarget.Length();
			if (len > 0.001f) {
				_TargetDistance = len;
			}
			else {
				_TargetDistance = 10.0f;
			}

			Vector3 forward = transform->GetForward();
			_TargetPosition = transform->Position + forward * _TargetDistance;
			_LastTargetPosition = _TargetPosition;
			_IsInitialized = true;
		}
		else {
			bool posChanged = !(transform->Position == _LastPosition);
			bool rotChanged = !(transform->Rotation == _LastRotation);
			bool targetChanged = !(_TargetPosition == _LastTargetPosition);

			if (posChanged || rotChanged) {
				Vector3 forward = transform->GetForward();
				_TargetPosition = transform->Position + forward * _TargetDistance;
			}
			else if (targetChanged) {
				Vector3 toTarget = _TargetPosition - transform->Position;
				float len = toTarget.Length();
				if (len > 0.001f) {
					_TargetDistance = len;
					Vector3 dir = toTarget.Normalize();
					float pitch = std::asin(dir.y);
					float yaw = std::atan2(dir.z, dir.x);
					transform->Rotation = Vector3(pitch, yaw, 0.0f);
				}
				else {
					_TargetDistance = 1.0f;
				}
			}
			else {
				if (transform->Position == _TargetPosition) {
					Vector3 forward = transform->GetForward();
					_TargetPosition = transform->Position + forward * _TargetDistance;
				}
			}
		}

		_LastPosition = transform->Position;
		_LastRotation = transform->Rotation;
		_LastTargetPosition = _TargetPosition;

		XMFLOAT3 posF3 = { transform->Position.x, transform->Position.y, transform->Position.z };
		XMFLOAT3 targetF3 = { _TargetPosition.x, _TargetPosition.y, _TargetPosition.z };
		XMFLOAT3 upF3 = { _UpVector.x, _UpVector.y, _UpVector.z };

		XMVECTOR pos = XMLoadFloat3(&posF3);
		XMVECTOR eyev = XMLoadFloat3(&targetF3);
		XMVECTOR up = XMLoadFloat3(&upF3);
		_ViewMatrix = XMMatrixLookAtLH(pos, eyev, up);
	}

	CAMERA_CONSTANT cameraConstant{};
	XMStoreFloat4x4(&cameraConstant.View, XMMatrixTranspose(_ViewMatrix));
	XMStoreFloat4x4(&cameraConstant.Projection, XMMatrixTranspose(_ProjectionMatrix));
    cameraConstant.Position =
        transform ? Vector4(transform->Position.x, transform->Position.y, transform->Position.z, 1.0f) : Vector4(0.0f, 0.0f, 0.0f, 1.0f);

	renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::CAMERA, &cameraConstant, sizeof(cameraConstant));
}

void Camera::Inspector() {
	ImGui::DragFloat("FOV", &_Fov, 0.5f, 1.0f, 180.0f, "%.1f");
	ImGui::DragFloat("Near", &_Near, 0.05f, 0.01f, 10.0f, "%.2f");
	ImGui::DragFloat("Far", &_Far, 1.0f, 10.0f, 10000.0f, "%.1f");
    ImGui::DragFloat3("Target Position", &_TargetPosition.x, 0.1f, -1000.0f,
                      1000.0f, "%.1f");
}
