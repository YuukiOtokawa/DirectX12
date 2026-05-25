#pragma once

#include "Component.h"
#include <d3d12.h>

#include "RenderManager.h"

#include "../../Utility/VertexData.h"

namespace EngineCore::General {

	class MeshFilter :
		public Component {
		REGISTER_COMPONENT(MeshFilter)

		std::unique_ptr<Render::Types::VERTEX_BUFFER> _VertexBuffer;
		std::unique_ptr<Render::Types::INDEX_BUFFER> _IndexBuffer;

		D3D12_PRIMITIVE_TOPOLOGY _PrimitiveTopology;

		VertexData* _pVertexData;
	public:
		MeshFilter() = default;
		void Update() override;

		void Draw() override;

		void Inspector() override;

		void SetVertexBuffer(Render::Types::VERTEX_BUFFER* pVertexBuffer) {
			_VertexBuffer.reset(pVertexBuffer);
		}
		void SetIndexBuffer(Render::Types::INDEX_BUFFER* pIndexBuffer) {
			_IndexBuffer.reset(pIndexBuffer);
		}
		void SetVertexData(VertexData* vertexData) {
			_pVertexData = vertexData;
			SetVertexData(vertexData->GetFilePath().c_str(), vertexData->GetVertices(), vertexData->GetIndices());
			SetPrimitiveTopology(vertexData->GetPrimitiveTopology());
		}
		void SetVertexData(const char* FilePath, std::vector<Render::Types::VERTEX> vertices, std::vector<unsigned int> indices = {});
		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) { _PrimitiveTopology = topology; }

		Render::Types::VERTEX_BUFFER* GetVertexBuffer() const { return _VertexBuffer.get(); }
		Render::Types::INDEX_BUFFER* GetIndexBuffer() const { return _IndexBuffer.get(); }
		VertexData* GetVertexData() const { return _pVertexData; }
	};
}