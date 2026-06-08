#include "MeshRenderer.h"
#include "MeshFilter.h"
#include "../Transform/Transform.h"
#include "RenderManager.h"
#include "../../GameObject/GameObject.h"
#include "../../../ImGui/Code/imgui.h"

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

	// Bind Material Constant Buffer
	{
		Render::MaterialConstant constData = m_Material.GetConstantData();
		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::SUBSET, &constData, sizeof(Render::MaterialConstant));
	}

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

void MeshRenderer::Inspector() {
	// Base Color
	Vector4 baseColor = m_Material.GetBaseColor();
	float color[4] = { baseColor.x, baseColor.y, baseColor.z, baseColor.w };
	if (ImGui::ColorEdit4("Base Color", color)) {
		m_Material.SetBaseColor(Vector4(color[0], color[1], color[2], color[3]));
	}

	// Emission Color
	Vector4 emissionColor = m_Material.GetEmissionColor();
	float emissive[4] = { emissionColor.x, emissionColor.y, emissionColor.z, emissionColor.w };
	if (ImGui::ColorEdit4("Emission Color", emissive)) {
		m_Material.SetEmissionColor(Vector4(emissive[0], emissive[1], emissive[2], emissive[3]));
	}

	// Metallic
	float metallic = m_Material.GetMetallic();
	if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
		m_Material.SetMetallic(metallic);
	}

	// Roughness
	float roughness = m_Material.GetRoughness();
	if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
		m_Material.SetRoughness(roughness);
	}

	// Specular
	float specular = m_Material.GetSpecular();
	if (ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f)) {
		m_Material.SetSpecular(specular);
	}
}
