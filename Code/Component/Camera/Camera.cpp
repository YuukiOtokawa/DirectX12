#include "Camera.h"

#include "../Component.h"
#include "RenderManager.h"

#include "../Transform/Transform.h"

using namespace EngineCore::General;
using namespace Render::RenderStructure;

Camera::Camera() {
	_ProjectionMatrix = XMMatrixIdentity();
	_ViewMatrix = XMMatrixIdentity();
}

void Camera::RenderCamera() {
	auto renderManager = Render::RenderManager::GetInstance();

	Vector2 clientSize = Vector2(renderManager->GetBackBufferWidth(), renderManager->GetBackBufferHeight());

	_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(_Fov), clientSize.x / clientSize.y, _Near, _Far);

	// TODO: Œã‚Å‘O•û•ûŒü‚É‚¸‚ç‚·‚æ‚¤‚É•ÏX
	if (GetComponent<Transform>()->Position == _TargetPosition)
		_TargetPosition.z += 0.1f;

	XMVECTOR eyev = XMLoadFloat3(_TargetPosition.ToXMFloat3());
	XMVECTOR pos = XMLoadFloat3(GetComponent<Transform>()->Position.ToXMFloat3());
	XMVECTOR up = XMLoadFloat3(_UpVector.ToXMFloat3());
	_ViewMatrix = XMMatrixLookAtLH(pos, eyev, up);

	CAMERA_CONSTANT cameraConstant{};
	XMStoreFloat4x4(&cameraConstant.View, XMMatrixTranspose(_ViewMatrix));
	XMStoreFloat4x4(&cameraConstant.Projection, XMMatrixTranspose(_ProjectionMatrix));

	renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::CAMERA, &cameraConstant, sizeof(cameraConstant));
}


void Camera::DrawInspector() {}
