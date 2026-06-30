#pragma once

#include "Component.h"
#include <d3d12.h>

#include "RenderManager.h"

#include "../../Utility/VertexData.h"

namespace EngineCore::General {

	class MeshFilter :
		public Component {
		REGISTER_COMPONENT(MeshFilter)

		std::unique_ptr<EngineCore::Render::Types::VERTEX_BUFFER> m_VertexBuffer;
		std::unique_ptr<EngineCore::Render::Types::INDEX_BUFFER> m_IndexBuffer;

		D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;

		VertexData* m_VertexData = nullptr;
	public:
		MeshFilter() = default;
		~MeshFilter() override {
			if (m_VertexData) {
				delete m_VertexData;
				m_VertexData = nullptr;
			}
		}
		void Update() override;

		void Inspector() override;

		void SetVertexBuffer(EngineCore::Render::Types::VERTEX_BUFFER* pVertexBuffer) {
			m_VertexBuffer.reset(pVertexBuffer);
		}
		void SetIndexBuffer(EngineCore::Render::Types::INDEX_BUFFER* pIndexBuffer) {
			m_IndexBuffer.reset(pIndexBuffer);
		}
		void SetVertexData(VertexData* vertexData) {
			m_VertexData = vertexData;
			SetVertexData(vertexData->GetFilePath().c_str(), vertexData->GetVertices(), vertexData->GetIndices());
			SetPrimitiveTopology(vertexData->GetPrimitiveTopology());
		}
		void SetVertexData(const char* FilePath, std::vector<EngineCore::Render::Types::VERTEX> vertices, std::vector<unsigned int> indices = {});
		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) { m_PrimitiveTopology = topology; }

		EngineCore::Render::Types::VERTEX_BUFFER* GetVertexBuffer() const { return m_VertexBuffer.get(); }
		EngineCore::Render::Types::INDEX_BUFFER* GetIndexBuffer() const { return m_IndexBuffer.get(); }
		VertexData* GetVertexData() const { return m_VertexData; }
		D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
	};
}