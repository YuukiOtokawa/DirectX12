#pragma once

// スクリプト（将来の GameScripts.dll）から見えるエンジンの公開ヘッダ
// スクリプトはこのヘッダ（と Engine/Public/ 配下）だけを include する
//
// ここから <Windows.h> / <d3d12.h> に到達させないこと。1本混ざるだけでスクリプト1本の再コンパイル時間が桁で変わる
// ヘッダを足したら、推移的閉包に重量級のシステムヘッダが現れないことを確認する（仕様書「進め方」Phase 3）

#include "Engine/Source/Component/Component.h"
#include "Engine/Source/GameObject/GameObject.h"
#include "Engine/Source/Component/Transform/Transform.h"
#include "Engine/Source/Component/Camera/Camera.h"
#include "Engine/Source/Component/Polygon/Light.h"
#include "Engine/Source/Component/Polygon/MeshRenderer.h"
#include "Engine/Source/Component/Polygon/MeshFilter.h"
