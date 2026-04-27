#include "PolygonRenderer.h"
#include <Windows.h>

#include <d3d12.h>
#include <DirectXMath.h>
using namespace DirectX;

using namespace EngineCore::Object::Component::Renderer;

PolygonRenderer::PolygonRenderer() {
	auto renderManager = Render::RenderManager::GetInstance();

	_VertexBuffer = renderManager->CreateVertexBuffer(sizeof(VERTEX_3D), 4);

	VERTEX_3D* buffer{};
	HRESULT hr = _VertexBuffer->Resource->Map(0, nullptr, (void**)&buffer);

	buffer[0].Position = { -10.0f,0.0f,10.0f };
	buffer[1].Position = { 10.0f,0.0f,10.0f };
	buffer[2].Position = { -10.0f,0.0f,-10.0f };
	buffer[3].Position = { 10.0f,0.0f,-10.0f };

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

	_Texture = renderManager->LoadTexture("Assets/field004.dds");
}

void PolygonRenderer::Update() {}

void PolygonRenderer::Draw() {
	auto renderManager = Render::RenderManager::GetInstance();

	//マトリクス設定
	{
		XMMATRIX world = XMMatrixIdentity();
		OBJECT_CONSTANT objectConstant{};
		XMStoreFloat4x4(&objectConstant.World, XMMatrixTranspose(world));

		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::OBJECT, &objectConstant, sizeof(objectConstant));
	}

	renderManager->SetVertexBuffer(_VertexBuffer.get());

	renderManager->SetTexture(Render::RenderManager::TEXTURE_TYPE::BASE_COLOR, _Texture.get());

	renderManager->GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderManager->SetPipelineState("Unlit");

	renderManager->GetGraphicsCommandList()->DrawInstanced(4, 1, 0, 0);
}
