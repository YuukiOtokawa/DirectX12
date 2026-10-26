#include "MeshRenderer.h"
#include "MeshFilter.h"
#include "../Transform/Transform.h"
#include "RenderManager.h"
#include "../../GameObject/GameObject.h"
#include "../../../ImGui/Code/imgui.h"
#include "../../Utility/FilePicker.h"
#include "../MaterialPropertyInspector.h"
#include "../../Utility/ResourcePath.h"

using namespace EngineCore::General;
using namespace EngineCore::Render::RenderStructure;

void MeshRenderer::Draw() {
	auto meshFilter = GetOwner()->GetComponent<MeshFilter>();
	if (!meshFilter) return;

	auto vertexBuffer = meshFilter->GetVertexBuffer();
	auto vertexData = meshFilter->GetVertexData();
	if (!vertexBuffer || !vertexData) return;

	auto renderManager = EngineCore::Render::RenderManager::GetInstance();
	if (!renderManager) return;

	// 影を落とさないオブジェクトはシャドウパスでは描かない
	if (renderManager->IsShadowPass() && !m_CastShadows) return;

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

		renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::OBJECT, &objectConstant, sizeof(objectConstant));
	}

	// Bind Vertex Buffer and Topology
	renderManager->SetVertexBuffer(vertexBuffer);
	
	auto indexBuffer = meshFilter->GetIndexBuffer();
	if (indexBuffer) {
		renderManager->SetIndexBuffer(indexBuffer);
	}

	renderManager->GetGraphicsCommandList()->IASetPrimitiveTopology(meshFilter->GetPrimitiveTopology());

	// Bind Pipeline State（マテリアルのシェーダーは初回使用時に生成される）
    if (renderManager->IsShadowPass()) {
        renderManager->SetPipelineState("Shadow");
    } else {
		// シェーダー未設定なら Geometry。コンパイルに失敗したシェーダーは描かない
		const bool hasShader = !m_Material.GetShaderFilePath().empty();
		const std::string shaderFile = hasShader ? m_Material.GetShaderFilePath() : SHADER_DIR "Geometry.hlsl";
		const auto passType = hasShader ? m_Material.GetRenderPassType() : EngineCore::Render::RenderPassType::DeferredOpaque;
		if (!renderManager->SetPipelineState(shaderFile, passType)) return;
	}

	// Bind Material Constant Buffer (Dynamic or Legacy)
    if (renderManager->IsShadowPass() || !m_Material.GetShaderName().empty()) {
		renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::SUBSET, m_Material.GetBufferData(), static_cast<unsigned int>(m_Material.GetBufferSize()));
	} else {
		EngineCore::Render::MaterialConstant constData = m_Material.GetConstantData();
		renderManager->SetConstant(EngineCore::Render::RenderManager::CONSTANT_TYPE::SUBSET, &constData, sizeof(EngineCore::Render::MaterialConstant));
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
		// PSO はパス種別ごとにキャッシュされるので、ここでは種別を変えるだけでよい
		m_Material.SetRenderPassType(static_cast<EngineCore::Render::RenderPassType>(currentPassType));
	}

	// --- Shader Picker ---
	ImGui::Text("Shader (HLSL)");
	ImGui::SameLine();
	if (ImGui::Button("Select Shader")) {
		COMDLG_FILTERSPEC shaderFilter[] = { { L"HLSL Files (*.hlsl)", L"*.hlsl" }, { L"All Files", L"*.*" } };
		std::string path = OpenFileDialog(shaderFilter, _countof(shaderFilter));
		if (!path.empty() && renderManager) {
			// 選び直しでファイルの変更を反映させるため読み直す。コンパイルできたときだけ差し替える
			if (renderManager->ReloadShader(path, m_Material.GetRenderPassType())) {
				m_Material.SetShaderFile(path);
			}
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
