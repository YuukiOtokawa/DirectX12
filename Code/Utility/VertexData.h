#pragma once

#include <vector>
#include <string>

#include <d3d12.h>

#include "../Render/RenderManager.h"

#include <memory>

class VertexData {
	std::vector<Render::Types::VERTEX> _Vertices;
	std::vector<unsigned int> _Indices;
	std::string _FilePath;
	D3D12_PRIMITIVE_TOPOLOGY _PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	std::vector<std::unique_ptr<Render::Types::TEXTURE>> _Textures;
public:
	VertexData() = default;
	VertexData(const char* FilePath, std::vector<Render::Types::VERTEX> vertices, std::vector<unsigned int> indices = {}) {
		_FilePath = FilePath;
		_Vertices = vertices;
		_Indices = indices;
	}
	void LoadFromFile(const char* FilePath);

	const std::vector<Render::Types::VERTEX>& GetVertices() const { return _Vertices; }
	const std::vector<unsigned int>& GetIndices() const { return _Indices; }
	const std::string& GetFilePath() const { return _FilePath; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return _PrimitiveTopology; }

	//Get Textures
	const std::vector<std::unique_ptr<Render::Types::TEXTURE>>& GetTextures() const { return _Textures; }

	void SetVertices(const std::vector<Render::Types::VERTEX>& vertices) { _Vertices = vertices; }
	void SetIndices(const std::vector<unsigned int>& indices) { _Indices = indices; }
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) { _PrimitiveTopology = topology; }
	void SetFilePath(const char* FilePath) { _FilePath = FilePath; }
	void SetTextures(std::vector<std::unique_ptr<Render::Types::TEXTURE>>&& textures) { _Textures = std::move(textures); }
};
