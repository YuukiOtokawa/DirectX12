#pragma once

#include "Engine/Public/EngineAPI.h"
#include "Component.h"

#include "Engine/Source/Render/VertexTypes.h"
#include "Engine/Source/Utility/VertexData.h"

#include <memory>
#include <string>
#include <vector>

namespace EngineCore::General {

	class ENGINE_API MeshFilter :
		public Component {
		REGISTER_COMPONENT(MeshFilter)

		std::unique_ptr<EngineCore::Render::Types::VERTEX_BUFFER> m_VertexBuffer;
		std::unique_ptr<EngineCore::Render::Types::INDEX_BUFFER> m_IndexBuffer;

		EngineCore::Render::PrimitiveTopology m_PrimitiveTopology = EngineCore::Render::PrimitiveTopology::TriangleStrip;

		VertexData* m_VertexData = nullptr;
	public:
		// VERTEX_BUFFER / INDEX_BUFFER を不完全型のまま扱うため、生成・破棄は MeshFilter.cpp で定義する
		MeshFilter();
		~MeshFilter() override;
		void Update() override;

		// FBX / OBJ を拡張子で判別して読み込み、頂点データとして設定する
		void LoadModel(const std::string& filePath);

		void SetVertexBuffer(EngineCore::Render::Types::VERTEX_BUFFER* pVertexBuffer);
		void SetIndexBuffer(EngineCore::Render::Types::INDEX_BUFFER* pIndexBuffer);
		void SetVertexData(VertexData* vertexData) {
			m_VertexData = vertexData;
			SetVertexData(vertexData->GetFilePath().c_str(), vertexData->GetVertices(), vertexData->GetIndices());
			SetPrimitiveTopology(vertexData->GetPrimitiveTopology());
		}
		void SetVertexData(const char* FilePath, std::vector<EngineCore::Render::Types::VERTEX> vertices, std::vector<unsigned int> indices = {});
		void SetPrimitiveTopology(EngineCore::Render::PrimitiveTopology topology) { m_PrimitiveTopology = topology; }

		EngineCore::Render::Types::VERTEX_BUFFER* GetVertexBuffer() const { return m_VertexBuffer.get(); }
		EngineCore::Render::Types::INDEX_BUFFER* GetIndexBuffer() const { return m_IndexBuffer.get(); }
		VertexData* GetVertexData() const { return m_VertexData; }
		EngineCore::Render::PrimitiveTopology GetPrimitiveTopology() const { return m_PrimitiveTopology; }
	};
}
