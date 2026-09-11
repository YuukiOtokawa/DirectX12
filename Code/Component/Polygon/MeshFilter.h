#pragma once

#include "Component.h"

// GPU バッファは unique_ptr 越しにしか触らないので、<d3d12.h> / RenderManager.h は include しない。
// （VERTEX_BUFFER / INDEX_BUFFER の前方宣言は RenderTypes.h にある）
// 完全型が要るメンバ関数は MeshFilter.cpp に出してある。
#include "../../Render/RenderTypes.h"
#include "../../Utility/VertexData.h"

namespace EngineCore::General {

	class MeshFilter :
		public Component {
		REGISTER_COMPONENT(MeshFilter)

		std::unique_ptr<EngineCore::Render::Types::VERTEX_BUFFER> m_VertexBuffer;
		std::unique_ptr<EngineCore::Render::Types::INDEX_BUFFER> m_IndexBuffer;

		EngineCore::Render::PrimitiveTopology m_PrimitiveTopology = EngineCore::Render::PrimitiveTopology::TriangleStrip;

		VertexData* m_VertexData = nullptr;
	public:
		MeshFilter();
		~MeshFilter() override;

		void Update() override;

		void Inspector() override;

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
