#pragma once

// 頂点データとプリミティブの型
// Windows.h / d3d12.h に依存させないこと（コンポーネントの公開ヘッダから include されるため）

#include "Engine/Source/Utility/VectorClass.h"

namespace EngineCore::Render {

	// プリミティブトポロジー。D3D12 の値への変換は RenderManager.h の ToD3D12PrimitiveTopology() で行う
	enum class PrimitiveTopology {
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
	};

	namespace Types {

		// 頂点データ
		struct VERTEX
		{
			Vector3 Position;
			Vector3 Normal;
			Vector2 TexCoord;
			Vector4 Color;
		};

		// GPU リソースを持つ型。定義は RenderManager.h
		struct TEXTURE;
		struct VERTEX_BUFFER;
		struct INDEX_BUFFER;
	}
}
