#pragma once

// レンダリング関連のうち、GPU リソース（ComPtr<ID3D12*>）を持たない
// 「純粋なデータ」だけを集めた軽量ヘッダ。
//
// 目的: スクリプト・コンポーネントから見える公開ヘッダが <d3d12.h> / <Windows.h> を
//       引きずり込まないようにすること。これらは前処理後に数十万行規模になるため、
//       1つ混ざるだけでスクリプト1本の再コンパイル時間が大きく伸びる。
//       （詳細は Split_Project.md 2-1）
//
// GPU リソースを保持する型（TEXTURE / RENDER_TARGET / VERTEX_BUFFER / INDEX_BUFFER）は
// ここでは前方宣言だけ置き、定義は RenderManager.h に残す。
// ポインタ・参照越しにしか触らない場所ではこのヘッダだけで足りる。

#include "../Utility/VectorClass.h"

namespace EngineCore::Render {

	// D3D の D3D_PRIMITIVE_TOPOLOGY をエンジン側で持ち直したもの。
	// D3D の値への変換は RenderManager.h の ToD3DPrimitiveTopology() で行う。
	enum class PrimitiveTopology {
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

		// --- GPU リソース保持型（定義は RenderManager.h）---
		struct TEXTURE;
		struct RENDER_TARGET;
		struct VERTEX_BUFFER;
		struct INDEX_BUFFER;

	}
}
