#include "MeshRenderer.h"
#include "MeshFilter.h"
#include "../Transform/Transform.h"
#include "RenderManager.h"
#include "../../GameObject/GameObject.h"

using namespace EngineCore::General;
using namespace Render::RenderStructure;

void MeshRenderer::Draw() {
	auto meshFilter = GetOwner()->GetComponent<MeshFilter>();
	if (!meshFilter) return;

	auto vertexBuffer = meshFilter->GetVertexBuffer();
	auto vertexData = meshFilter->GetVertexData();
	if (!vertexBuffer || !vertexData) return;

	auto renderManager = Render::RenderManager::GetInstance();
	if (!renderManager) return;

	auto transform = GetOwner()->GetComponent<Transform>();
	if (!transform) return;

	// Calculate and send World Matrix
	{
		XMMATRIX world = XMMatrixIdentity();
		world *= XMMatrixScaling(transform->GetScale().x, transform->GetScale().y, transform->GetScale().z);
		world *= XMMatrixRotationRollPitchYaw(transform->GetRotation().x, transform->GetRotation().y, transform->GetRotation().z);
		world *= XMMatrixTranslation(transform->GetPosition().x, transform->GetPosition().y, transform->GetPosition().z);
		OBJECT_CONSTANT objectConstant{};
		XMStoreFloat4x4(&objectConstant.World, XMMatrixTranspose(world));

		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::OBJECT, &objectConstant, sizeof(objectConstant));
	}

	// Bind Vertex Buffer and Topology
	renderManager->SetVertexBuffer(vertexBuffer);
	
	auto indexBuffer = meshFilter->GetIndexBuffer();
	if (indexBuffer) {
		renderManager->SetIndexBuffer(indexBuffer);
	}

	renderManager->GetGraphicsCommandList()->IASetPrimitiveTopology(meshFilter->GetPrimitiveTopology());
	renderManager->SetPipelineState("Geometry");

	// Bind Material Textures
	for (auto& texture : vertexData->GetTextures()) {
		renderManager->SetTexture(Render::RenderManager::TEXTURE_TYPE::BASE_COLOR, texture.get());
	}

	// Issue Draw command
	if (indexBuffer && !vertexData->GetIndices().empty()) {
		renderManager->GetGraphicsCommandList()->DrawIndexedInstanced(
			static_cast<UINT>(vertexData->GetIndices().size()), 1, 0, 0, 0);
	} else {
		renderManager->GetGraphicsCommandList()->DrawInstanced(
			static_cast<UINT>(vertexData->GetVertices().size()), 1, 0, 0);
	}
}

void MeshRenderer::Inspector() {}
