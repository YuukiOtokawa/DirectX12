#include "Camera.h"

#include "RenderManager.h"
using namespace Render::RenderStructure;

Camera::Camera() {
	m_Position = { 0.0f, 0.5f, -5.0f };
	m_Target = { 0.0f, 0.0f, 0.0f };
}

void Camera::Update() {}

void Camera::Draw() {
	auto renderManager = Render::RenderManager::GetInstance();



	{
		XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };
		XMMATRIX view = XMMatrixLookAtLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&m_Target), XMLoadFloat3(&up));
		
		XMMATRIX projection;

		float aspect = (float)renderManager->GetBackBufferWidth() / (float)renderManager->GetBackBufferHeight();
		projection = XMMatrixPerspectiveFovLH(1.0f, aspect, 0.1f, 100.0f);

		CAMERA_CONSTANT cameraConstant{};
		XMStoreFloat4x4(&cameraConstant.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cameraConstant.Projection, XMMatrixTranspose(projection));

		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::CAMERA, &cameraConstant, sizeof(cameraConstant));
	}
}
