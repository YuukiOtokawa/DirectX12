#pragma once

#include <vector>
#include <string>
#include <memory>

// GPU リソースを持たない型だけを使うので、<d3d12.h> / RenderManager.h は include しない。
// TEXTURE は unique_ptr 越しにしか触らないため前方宣言で足りる（実体の定義は RenderManager.h）。
// そのため特殊メンバ関数は VertexData.cpp に出してある。
#include "../Render/RenderTypes.h"

class VertexData {
	std::vector<EngineCore::Render::Types::VERTEX> m_Vertices;
	std::vector<unsigned int> m_Indices;
	std::string m_FilePath;
	EngineCore::Render::PrimitiveTopology m_PrimitiveTopology = EngineCore::Render::PrimitiveTopology::TriangleStrip;

	std::vector<std::unique_ptr<EngineCore::Render::Types::TEXTURE>> m_Textures;
public:
	VertexData();
	VertexData(const char* FilePath, std::vector<EngineCore::Render::Types::VERTEX> vertices, std::vector<unsigned int> indices = {});
	~VertexData();

	const std::vector<EngineCore::Render::Types::VERTEX>& GetVertices() const { return m_Vertices; }
	const std::vector<unsigned int>& GetIndices() const { return m_Indices; }
	const std::string& GetFilePath() const { return m_FilePath; }
	EngineCore::Render::PrimitiveTopology GetPrimitiveTopology() const { return m_PrimitiveTopology; }

	//Get Textures
	const std::vector<std::unique_ptr<EngineCore::Render::Types::TEXTURE>>& GetTextures() const { return m_Textures; }

	void SetVertices(const std::vector<EngineCore::Render::Types::VERTEX>& vertices) { m_Vertices = vertices; }
	void SetIndices(const std::vector<unsigned int>& indices) { m_Indices = indices; }
	void SetPrimitiveTopology(EngineCore::Render::PrimitiveTopology topology) { m_PrimitiveTopology = topology; }
	void SetFilePath(const char* FilePath) { m_FilePath = FilePath; }
	void SetTextures(std::vector<std::unique_ptr<EngineCore::Render::Types::TEXTURE>>&& textures);
};
