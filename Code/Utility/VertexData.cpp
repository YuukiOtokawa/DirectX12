#include "VertexData.h"

// VertexData.h は TEXTURE を前方宣言だけで扱っているため、
// unique_ptr<TEXTURE> の破棄には完全型が要る特殊メンバ関数をここに置く。
// （TEXTURE の定義は RenderManager.h にある）
#include "../Render/RenderManager.h"

VertexData::VertexData() = default;

VertexData::VertexData(const char* FilePath, std::vector<EngineCore::Render::Types::VERTEX> vertices, std::vector<unsigned int> indices) {
	m_FilePath = FilePath;
	m_Vertices = std::move(vertices);
	m_Indices = std::move(indices);
}

VertexData::~VertexData() = default;

void VertexData::SetTextures(std::vector<std::unique_ptr<EngineCore::Render::Types::TEXTURE>>&& textures) {
	m_Textures = std::move(textures);
}
