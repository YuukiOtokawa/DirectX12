#include "MeshRenderer.h"
#include "MeshFilter.h"
#include "Code/Component/Transform/Transform.h"
#include "RenderManager.h"
#include "Code/GameObject/GameObject.h"
#include "ImGui/Code/imgui.h"
#include "Code/Utility/FilePicker.h"
#include "Code/Utility/ResourcePath.h"
#include "Code/Component/MaterialPropertyInspector.h"

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

	renderManager->GetGraphicsCommandList()->IASetPrimitiveTopology(meshFilter->GetPrimitiveTopology());

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

void MeshRenderer::Inspector() {
	auto renderManager = EngineCore::Render::RenderManager::GetInstance();

	ImGui::Checkbox("Cast Shadows", &m_CastShadows);

	// --- Render Pass Picker ---
	ImGui::Text("Render Pass");
	ImGui::SameLine();
	int currentPassType = static_cast<int>(m_Material.GetRenderPassType());
	const char* passTypeNames[] = { "Deferred Opaque", "Forward Opaque", "Forward Transparent" };
	if (ImGui::Combo("##RenderPass", &currentPassType, passTypeNames, _countof(passTypeNames))) {
		// PSOは描画時にシェーダー×パス種別で引き直されるので、ここでは種別を変えるだけ
		m_Material.SetRenderPassType(static_cast<EngineCore::Render::RenderPassType>(currentPassType));
	}

	// --- Shader Picker ---
	ImGui::Text("Shader (HLSL)");
	ImGui::SameLine();
	if (ImGui::Button("Select Shader")) {
		COMDLG_FILTERSPEC shaderFilter[] = { { L"HLSL Files (*.hlsl)", L"*.hlsl" }, { L"All Files", L"*.*" } };
		std::string path = OpenFileDialog(shaderFilter, _countof(shaderFilter));
		// 同じファイルを選び直したときも再コンパイルされる（ホットリロード）
		if (!path.empty() && renderManager &&
			renderManager->ReloadPipeline(path, m_Material.GetRenderPassType())) {
			m_Material.SetShaderFile(path);
		}
	}

	// --- Texture Picker ---
	ImGui::Text("Base Color Texture");
	if (m_Material.GetTextureBaseColor() && renderManager) {
		auto handle = renderManager->GetShaderResourceViewHandle(m_Material.GetTextureBaseColor()->SRVIndex);
		ImGui::Image((void*)handle.ptr, ImVec2(64.0f, 64.0f));
		ImGui::SameLine();
	}
	if (ImGui::Button("Select Texture")) {
		COMDLG_FILTERSPEC texFilter[] = { { L"Texture Files", L"*.png;*.jpg;*.tga;*.dds" }, { L"All Files", L"*.*" } };
		std::string path = OpenFileDialog(texFilter, _countof(texFilter));
		if (!path.empty() && renderManager) {
			std::shared_ptr<EngineCore::Render::Types::TEXTURE> tex = renderManager->LoadTexture(path.c_str());
			if (tex) {
				m_Material.SetTextureBaseColor(std::move(tex));
			}
		}
	}

	ImGui::Separator();

	// --- Dynamic Properties UI ---
	const EngineCore::Render::ShaderMetadata* meta = nullptr;
	if (renderManager && !m_Material.GetShaderName().empty()) {
		meta = renderManager->GetShaderMetadata(m_Material.GetShaderName());
	}

	if (meta) {
		ImGui::Text("Shader Properties (%s)", m_Material.GetShaderName().c_str());
		GUIHelper::DrawMaterialProperties(m_Material, meta);
	} else {
		// Legacy Properties UI
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
}
