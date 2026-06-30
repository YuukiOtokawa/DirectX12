#include "MeshFilter.h"
#include "../../../ImGui/Code/imgui.h"

#include "../../Utility/FilePicker.h"
#include "../../Utility/FBXLoader.h"
#include "../../Utility/OBJLoader.h"
#include <algorithm>

#include "../../GameObject/GameObject.h"
#include "../Transform/Transform.h"

void EngineCore::General::MeshFilter::Update() {}

void EngineCore::General::MeshFilter::Inspector() {
	if (ImGui::Button("Open Model")) {
		auto filePath = OpenFileDialog();
		if (filePath.empty())
			return;

		if (!m_VertexData) {
			m_VertexData = new VertexData();
		}

		// Determine file extension
		std::string pathStr(filePath);
		std::string ext = pathStr.substr(pathStr.find_last_of(".") + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == "fbx") {
			auto fbxData = LoadFBX(filePath.c_str(), m_VertexData);
			if (fbxData.m_Scene) {
				SetVertexData(fbxData.vertexData[0]);
				SetPrimitiveTopology(fbxData.vertexData[0]->GetPrimitiveTopology());
			}
		}
		else if (ext == "obj") {
			LoadObjToVertexData(filePath.c_str(), m_VertexData);
			SetVertexData(m_VertexData->GetFilePath().c_str(), m_VertexData->GetVertices(), m_VertexData->GetIndices());
			SetPrimitiveTopology(m_VertexData->GetPrimitiveTopology());
		}
	}
}

void EngineCore::General::MeshFilter::SetVertexData(const char* FilePath, std::vector<EngineCore::Render::Types::VERTEX> vertices, std::vector<unsigned int> indices) {
	HRESULT hr;
	{
		auto render = EngineCore::Render::RenderManager::GetInstance();

		m_VertexBuffer = render->CreateVertexBuffer(sizeof(EngineCore::Render::Types::VERTEX), (unsigned int)vertices.size());
		EngineCore::Render::Types::VERTEX* buffer{};
		hr = m_VertexBuffer->Resource->Map(0, nullptr, (void**)&buffer);

		memcpy(buffer, vertices.data(), sizeof(EngineCore::Render::Types::VERTEX) * vertices.size());
		m_VertexBuffer->Resource->Unmap(0, nullptr);
	}

	if (!indices.empty()) {
		auto render = EngineCore::Render::RenderManager::GetInstance();
		m_IndexBuffer = render->CreateIndexBuffer((unsigned int)indices.size());
		unsigned int* buffer{};
		hr = m_IndexBuffer->Resource->Map(0, nullptr, (void**)&buffer);
		memcpy(buffer, indices.data(), sizeof(unsigned int) * indices.size());
		m_IndexBuffer->Resource->Unmap(0, nullptr);
	}

	// 描画システムが m_VertexData を必要とするため、設定されていない場合は生成・設定する
	if (!m_VertexData) {
		m_VertexData = new VertexData(FilePath, vertices, indices);
	} else {
		m_VertexData->SetFilePath(FilePath);
		m_VertexData->SetVertices(vertices);
		m_VertexData->SetIndices(indices);
	}
}
