#include "MeshFilter.h"

#include "Engine/Source/Asset/MeshAsset.h"
#include <algorithm>

#include "Engine/Source/GameObject/GameObject.h"
#include "Engine/Source/Component/Transform/Transform.h"
#include "Engine/Source/Render/RenderManager.h"
#include "Engine/Source/Utility/Log.h"

namespace {
	// GPU が使い終わるまで破棄を遅らせる。同じフレームで記録済みの描画や、実行中の前のフレームが使っているため
	void DeferRelease(std::shared_ptr<void> object) {
		if (auto* renderManager = EngineCore::Render::RenderManager::GetInstance()) {
			renderManager->DeferRelease(std::move(object));
		}
	}

	// アップロードヒープのバッファへ CPU から書き込む
	bool WriteBuffer(ID3D12Resource* resource, const void* data, size_t size) {
		void* mapped = nullptr;
		HRESULT hr = resource->Map(0, nullptr, &mapped);
		if (FAILED(hr)) {
			EngineCore::Log::Write(EngineCore::Log::Level::Error, "Failed to map mesh buffer " + EngineCore::Log::FormatHresult(hr));
			return false;
		}
		memcpy(mapped, data, size);
		resource->Unmap(0, nullptr);
		return true;
	}
}

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

	// 古いメッシュはテクスチャを持つので、即 delete しない
	if (m_VertexData) {
		DeferRelease(std::shared_ptr<void>(m_VertexData, [](void* p) { delete static_cast<VertexData*>(p); }));
		m_VertexData = nullptr;
	}
	SetVertexData(vertexData);
}

void EngineCore::General::MeshFilter::SetVertexData(const char* FilePath, std::vector<EngineCore::Render::Types::VERTEX> vertices, std::vector<unsigned int> indices) {
	// 古いバッファは GPU が使い終わるまで残す。新しいバッファが作れなければ両方 null のままにし、
	// 頂点データとの食い違いを避ける（MeshRenderer は頂点バッファが null なら描かない。失敗はログに記録済み）
	DeferRelease(std::move(m_VertexBuffer));
	DeferRelease(std::move(m_IndexBuffer));

	if (auto render = EngineCore::Render::RenderManager::GetInstance()) {
		m_VertexBuffer = render->CreateVertexBuffer(sizeof(EngineCore::Render::Types::VERTEX), (unsigned int)vertices.size());
		if (m_VertexBuffer && !WriteBuffer(m_VertexBuffer->Resource.Get(), vertices.data(), sizeof(EngineCore::Render::Types::VERTEX) * vertices.size())) {
			m_VertexBuffer.reset(); // まだ GPU に使われていないので即解放してよい
		}

		if (m_VertexBuffer && !indices.empty()) {
			m_IndexBuffer = render->CreateIndexBuffer((unsigned int)indices.size());
			if (!m_IndexBuffer || !WriteBuffer(m_IndexBuffer->Resource.Get(), indices.data(), sizeof(unsigned int) * indices.size())) {
				m_IndexBuffer.reset();
				m_VertexBuffer.reset();
			}
		}
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
