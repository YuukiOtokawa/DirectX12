#include "VertexData.h"

#include "Code/Render/RenderManager.h"

VertexData::VertexData() = default;

VertexData::VertexData(const char* FilePath, std::vector<EngineCore::Render::Types::VERTEX> vertices, std::vector<unsigned int> indices) {
	m_FilePath = FilePath;
	m_Vertices = vertices;
	m_Indices = indices;
}

VertexData::~VertexData() = default;

void VertexData::SetTextures(std::vector<std::unique_ptr<EngineCore::Render::Types::TEXTURE>>&& textures) {
	m_Textures = std::move(textures);
}
