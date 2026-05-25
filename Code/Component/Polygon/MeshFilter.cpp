#include "MeshFilter.h"
#include "../../../ImGui/Code/imgui.h"

#include "../../Utility/FilePicker.h"
#include "../../Utility/ModelLoader.h"

#include "../Transform/Transform.h"

void EngineCore::General::MeshFilter::Update() {}

void EngineCore::General::MeshFilter::Draw() {
	if (!_VertexBuffer)
		return;

	auto renderManager = Render::RenderManager::GetInstance();
	
	auto transform = GetComponent<Transform>();
	if (!transform)
		return;

	{
			//マトリクス設定
		{
			XMMATRIX world = XMMatrixIdentity();
			world *= XMMatrixScaling(transform->GetScale().x, transform->GetScale().y, transform->GetScale().z);
			world *= XMMatrixRotationRollPitchYaw(transform->GetRotation().x, transform->GetRotation().y, transform->GetRotation().z);
			world *= XMMatrixTranslation(transform->GetPosition().x, transform->GetPosition().y, transform->GetPosition().z);
			OBJECT_CONSTANT objectConstant{};
			XMStoreFloat4x4(&objectConstant.World, XMMatrixTranspose(world));

			renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::OBJECT, &objectConstant, sizeof(objectConstant));
		}

		{
			XMMATRIX view = XMMatrixLookAtLH(
				XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f),
				XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
				XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f)
			);
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

		renderManager->SetPipelineState("Deferred");

		for (auto& texture : _pVertexData->GetTextures()) {
			renderManager->SetTexture(Render::RenderManager::TEXTURE_TYPE::BASE_COLOR, texture.get());
		}

		renderManager->GetGraphicsCommandList()->DrawInstanced(_pVertexData->GetVertices().size(), 1, 0, 0);

	}
}

void EngineCore::General::MeshFilter::Inspector() {
	if (ImGui::Button("Open Model")) {
		auto filePath = OpenFileDialog();
		if (filePath.empty())
			return;


		auto fbxData = LoadFBX(filePath.c_str(), _pVertexData);

		if (fbxData._Scene) {
			SetVertexData(fbxData.vertexData[0]);
			SetPrimitiveTopology(fbxData.vertexData[0]->GetPrimitiveTopology());
		}
	}
}

void EngineCore::General::MeshFilter::SetVertexData(const char* FilePath, std::vector<Render::Types::VERTEX> vertices, std::vector<unsigned int> indices) {
	HRESULT hr;
	{
		auto render = Render::RenderManager::GetInstance();

		_VertexBuffer = render->CreateVertexBuffer(sizeof(Render::Types::VERTEX), (unsigned int)vertices.size());
		Render::Types::VERTEX* buffer{};
		hr = _VertexBuffer->Resource->Map(0, nullptr, (void**)&buffer);

		memcpy(buffer, vertices.data(), sizeof(Render::Types::VERTEX) * vertices.size());
		_VertexBuffer->Resource->Unmap(0, nullptr);
	}

	if (!indices.empty()) {
		auto render = Render::RenderManager::GetInstance();
		_IndexBuffer = render->CreateIndexBuffer((unsigned int)indices.size());
		unsigned int* buffer{};
		hr = _IndexBuffer->Resource->Map(0, nullptr, (void**)&buffer);
		memcpy(buffer, indices.data(), sizeof(unsigned int) * indices.size());
		_IndexBuffer->Resource->Unmap(0, nullptr);
	}
}
