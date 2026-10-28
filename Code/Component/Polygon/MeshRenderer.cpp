#include "MeshRenderer.h"
#include "MeshFilter.h"
#include "Code/Component/Transform/Transform.h"
#include "RenderManager.h"
#include "Code/GameObject/GameObject.h"
#include "Code/Utility/ResourcePath.h"

using namespace EngineCore::General;
using namespace EngineCore::Render::RenderStructure;

void MeshRenderer::Draw() {
	auto* owner = GetOwner();
	if (!owner) return;

	auto* transform = owner->GetComponent<Transform>();
	if (!transform) return;

	const auto scale = transform->GetScale();
	const auto rotation = transform->GetRotation();
	const auto position = transform->GetPosition();
	const XMMATRIX world =
		XMMatrixScaling(scale.x, scale.y, scale.z) *
		XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
		XMMatrixTranslation(position.x, position.y, position.z);

	DrawMesh(world);
}

void MeshRenderer::DrawMesh(const DirectX::XMMATRIX& world, bool bindMaterial) {
	if (!GetOwner()) return;
	auto meshFilter = GetOwner()->GetComponent<MeshFilter>();
	if (!meshFilter) return;

	auto vertexBuffer = meshFilter->GetVertexBuffer();
	auto vertexData = meshFilter->GetVertexData();
	if (!vertexBuffer || !vertexData) return;

	auto renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return;

	// 影を落とさないオブジェクトはシャドウパスでは描かない
	if (renderManager->IsShadowPass() && !m_CastShadows) return;

	// Send the caller's World Matrix.
	{
		OBJECT_CONSTANT objectConstant{};
		XMStoreFloat4x4(&objectConstant.World, XMMatrixTranspose(world));

		renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::OBJECT, &objectConstant, sizeof(objectConstant));
	}

	// Bind Vertex Buffer and Topology
	renderManager->SetVertexBuffer(vertexBuffer);
	
	auto indexBuffer = meshFilter->GetIndexBuffer();
	if (indexBuffer) {
		renderManager->SetIndexBuffer(indexBuffer);
	}

	renderManager->GetGraphicsCommandList()->IASetPrimitiveTopology(EngineCore::Render::ToD3D12PrimitiveTopology(meshFilter->GetPrimitiveTopology()));

	// Bind Pipeline State
    if (renderManager->IsShadowPass()) {
        renderManager->SetPipelineState("Shadow");
    } else {
		// シェーダーファイル×パス種別でPSOを引く（初回のみコンパイル）。未設定なら Geometry
		const std::string& shaderFile = m_Material.GetShaderFilePath();
		ID3D12PipelineState* pipeline = renderManager->GetOrCreatePipeline(
			shaderFile.empty() ? SHADER_DIR "Geometry.hlsl" : shaderFile,
			m_Material.GetRenderPassType());
		if (!pipeline) return; // コンパイル失敗中は描かない
		renderManager->SetPipelineState(pipeline);
	}

	// Reuse the material constant across consecutive branches of the same tree.
	if (bindMaterial) {
		if (renderManager->IsShadowPass() || !m_Material.GetShaderName().empty()) {
			renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::SUBSET, m_Material.GetBufferData(), static_cast<unsigned int>(m_Material.GetBufferSize()));
		} else {
			EngineCore::Render::MaterialConstant constData = m_Material.GetConstantData();
			renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::SUBSET, &constData, sizeof(EngineCore::Render::MaterialConstant));
		}
	}

	// Bind Material Textures
	if (m_Material.GetTextureBaseColor()) {
		renderManager->SetTexture(EngineCore::Render::RenderManager::TEXTURE_TYPE::BASE_COLOR, m_Material.GetTextureBaseColor());
	} else {
		for (auto& texture : vertexData->GetTextures()) {
			renderManager->SetTexture(EngineCore::Render::RenderManager::TEXTURE_TYPE::BASE_COLOR, texture.get());
		}
	}

	// マテリアルテクスチャ(space1)テーブルをバインド
	if (m_Material.HasTextureBlock()) {
		renderManager->SetMaterialTextureTable(m_Material.GetTextureBlock());
	} else {
		renderManager->SetMaterialTextureTable(renderManager->GetDefaultMaterialBlock());
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
