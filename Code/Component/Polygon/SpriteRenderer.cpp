#include "SpriteRenderer.h"
#include <Windows.h>

#include <d3d12.h>
#include <DirectXMath.h>
using namespace DirectX;

using namespace EngineCore::Object::Component::Renderer;

SpriteRenderer::SpriteRenderer() {
	auto renderManager = Render::RenderManager::GetInstance();

	_VertexBuffer = renderManager->CreateVertexBuffer(sizeof(VERTEX_3D), 4);

	VERTEX_3D* buffer{};
	HRESULT hr = _VertexBuffer->Resource->Map(0, nullptr, (void**)&buffer);

	buffer[0].Position = { 0.0f,0.0f,0.0f };
	buffer[1].Position = { 200.0f,0.0f,0.0f };
	buffer[2].Position = { 0.0f,200.0f,0.0f };
	buffer[3].Position = { 200.0f,200.0f,0.0f };

	buffer[0].Color = { 1.0f,1.0f,1.0f,1.0f };
	buffer[1].Color = { 1.0f,1.0f,1.0f,1.0f };
	buffer[2].Color = { 1.0f,1.0f,1.0f,1.0f };
	buffer[3].Color = { 1.0f,1.0f,1.0f,1.0f };

	buffer[0].Normal = { 0.0f,1.0f,0.0f };
	buffer[1].Normal = { 0.0f,1.0f,0.0f };
	buffer[2].Normal = { 0.0f,1.0f,0.0f };
	buffer[3].Normal = { 0.0f,1.0f,0.0f };

	buffer[0].TexCoord = { 0.0f,0.0f };
	buffer[1].TexCoord = { 1.0f,0.0f };
	buffer[2].TexCoord = { 0.0f,1.0f };
	buffer[3].TexCoord = { 1.0f,1.0f };

	_VertexBuffer->Resource->Unmap(0, nullptr);
}

void SpriteRenderer::Update() {
}

void SpriteRenderer::Draw() {
	auto renderManager = Render::RenderManager::GetInstance();
	
	//マトリクス設定
	{
		XMMATRIX world = XMMatrixIdentity();
		OBJECT_CONSTANT objectConstant{};
		XMStoreFloat4x4(&objectConstant.World, XMMatrixTranspose(world));

		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::OBJECT, &objectConstant, sizeof(objectConstant));
	}

	{
		XMMATRIX view = XMMatrixIdentity();
		XMMATRIX projection = XMMatrixOrthographicOffCenterLH(0.0f,
															  renderManager->GetBackBufferWidth(),
															  renderManager->GetBackBufferHeight(),
															  0.0f,
															  0.1f,
															  100.0f);

		CAMERA_CONSTANT cameraConstant{};
		XMStoreFloat4x4(&cameraConstant.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cameraConstant.Projection, XMMatrixTranspose(projection));

		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::CAMERA, &cameraConstant, sizeof(cameraConstant));
	}

	renderManager->SetVertexBuffer(_VertexBuffer.get());
	renderManager->GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderManager->SetPipelineState("unlit");

	renderManager->GetGraphicsCommandList()->DrawInstanced(4, 1, 0, 0);
}
