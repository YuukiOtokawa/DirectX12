#include "MeshRenderer.h"
#include "MeshFilter.h"
#include "../Transform/Transform.h"
#include "RenderManager.h"
#include "../../GameObject/GameObject.h"
#include "../../../ImGui/Code/imgui.h"
#include "../../Utility/FilePicker.h"

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

	// Bind Pipeline State (Dynamic or Legacy)
	if (!m_Material.GetShaderName().empty()) {
		renderManager->SetPipelineState(m_Material.GetShaderName().c_str());
	} else {
		renderManager->SetPipelineState("Geometry");
	}

	// Bind Material Constant Buffer (Dynamic or Legacy)
	if (!m_Material.GetShaderName().empty()) {
		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::SUBSET, m_Material.GetBufferData(), static_cast<unsigned int>(m_Material.GetBufferSize()));
	} else {
		Render::MaterialConstant constData = m_Material.GetConstantData();
		renderManager->SetConstant(Render::RenderManager::CONSTANT_TYPE::SUBSET, &constData, sizeof(Render::MaterialConstant));
	}

	// Bind Material Textures
	if (m_Material.GetTextureBaseColor()) {
		renderManager->SetTexture(Render::RenderManager::TEXTURE_TYPE::BASE_COLOR, m_Material.GetTextureBaseColor());
	} else {
		for (auto& texture : vertexData->GetTextures()) {
			renderManager->SetTexture(Render::RenderManager::TEXTURE_TYPE::BASE_COLOR, texture.get());
		}
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
	auto renderManager = Render::RenderManager::GetInstance();
	
	// --- Shader Picker ---
	ImGui::Text("Shader (HLSL)");
	ImGui::SameLine();
	if (ImGui::Button("Select Shader")) {
		COMDLG_FILTERSPEC shaderFilter[] = { { L"HLSL Files (*.hlsl)", L"*.hlsl" }, { L"All Files", L"*.*" } };
		std::string path = OpenFileDialog(shaderFilter, _countof(shaderFilter));
		if (!path.empty() && renderManager) {
			std::string shaderName = path.substr(path.find_last_of("\\/") + 1);
			shaderName = shaderName.substr(0, shaderName.find_last_of("."));
			
			// Compiling with Geometry Pass RTV Format (5 G-Buffer layers)
			DXGI_FORMAT formats[] = {
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_R16G16B16A16_FLOAT
			};
			
			ComPtr<ID3D12PipelineState> pso = renderManager->CreatePipeline(path.c_str(), formats, _countof(formats));
			if (pso) {
				renderManager->RegisterPipelineState(shaderName, pso);
				m_Material.SetShader(shaderName);
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
			std::shared_ptr<Render::Types::TEXTURE> tex = renderManager->LoadTexture(path.c_str());
			if (tex) {
				m_Material.SetTextureBaseColor(std::move(tex));
			}
		}
	}

	ImGui::Separator();

	// --- Dynamic Properties UI ---
	const Render::ShaderMetadata* meta = nullptr;
	if (renderManager && !m_Material.GetShaderName().empty()) {
		meta = renderManager->GetShaderMetadata(m_Material.GetShaderName());
	}

	if (meta) {
		ImGui::Text("Shader Properties (%s)", m_Material.GetShaderName().c_str());
		for (auto& prop : meta->properties) {
			if (prop.type == "float4") {
				Vector4 val = m_Material.GetVector(prop.name);
				float color[4] = { val.x, val.y, val.z, val.w };
				if (ImGui::ColorEdit4(prop.name.c_str(), color)) {
					m_Material.SetVector(prop.name, Vector4(color[0], color[1], color[2], color[3]));
				}
			}
			else if (prop.type == "float") {
				float val = m_Material.GetFloat(prop.name);
				// Customize slider for common material properties
				if (prop.name == "Metallic" || prop.name == "Roughness" || prop.name == "Specular" || prop.name == "NormalWeight") {
					if (ImGui::SliderFloat(prop.name.c_str(), &val, 0.0f, 1.0f)) {
						m_Material.SetFloat(prop.name, val);
					}
				} else {
					if (ImGui::DragFloat(prop.name.c_str(), &val, 0.01f)) {
						m_Material.SetFloat(prop.name, val);
					}
				}
			}
		}
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
