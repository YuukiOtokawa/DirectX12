#pragma once

#include <vector>
#include <string>

#include <d3d12.h>

#include "../Render/RenderManager.h"

#include <memory>

class VertexData {
	std::vector<Render::Types::VERTEX> m_Vertices;
	std::vector<unsigned int> m_Indices;
	std::string m_FilePath;
	D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	std::vector<std::unique_ptr<Render::Types::TEXTURE>> m_Textures;
public:
	VertexData() = default;
	VertexData(const char* FilePath, std::vector<Render::Types::VERTEX> vertices, std::vector<unsigned int> indices = {}) {
		m_FilePath = FilePath;
		m_Vertices = vertices;
		m_Indices = indices;
	}
	void LoadFromFile(const char* FilePath);

	const std::vector<Render::Types::VERTEX>& GetVertices() const { return m_Vertices; }
	const std::vector<unsigned int>& GetIndices() const { return m_Indices; }
	const std::string& GetFilePath() const { return m_FilePath; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	//Get Textures
	const std::vector<std::unique_ptr<Render::Types::TEXTURE>>& GetTextures() const { return m_Textures; }

	void SetVertices(const std::vector<Render::Types::VERTEX>& vertices) { m_Vertices = vertices; }
	void SetIndices(const std::vector<unsigned int>& indices) { m_Indices = indices; }
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) { m_PrimitiveTopology = topology; }
	void SetFilePath(const char* FilePath) { m_FilePath = FilePath; }
	void SetTextures(std::vector<std::unique_ptr<Render::Types::TEXTURE>>&& textures) { m_Textures = std::move(textures); }
};
