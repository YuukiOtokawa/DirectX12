#include "MeshFilter.h"

#include "Engine/Source/Asset/MeshAsset.h"
#include <algorithm>

#include "Engine/Source/GameObject/GameObject.h"
#include "Engine/Source/Component/Transform/Transform.h"
#include "Engine/Source/Render/RenderManager.h"

EngineCore::General::MeshFilter::MeshFilter() = default;

EngineCore::General::MeshFilter::~MeshFilter() {
	if (m_VertexData) {
		delete m_VertexData;
		m_VertexData = nullptr;
	}
}

void EngineCore::General::MeshFilter::SetVertexBuffer(EngineCore::Render::Types::VERTEX_BUFFER* pVertexBuffer) {
	m_VertexBuffer.reset(pVertexBuffer);
}

void EngineCore::General::MeshFilter::SetIndexBuffer(EngineCore::Render::Types::INDEX_BUFFER* pIndexBuffer) {
	m_IndexBuffer.reset(pIndexBuffer);
}

void EngineCore::General::MeshFilter::Update() {}

void EngineCore::General::MeshFilter::LoadModel(const std::string& filePath) {
	if (filePath.empty())
		return;

	VertexData* vertexData = EngineCore::Asset::LoadMesh(filePath);
	if (!vertexData)
		return;

	delete m_VertexData;
	SetVertexData(vertexData);
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
