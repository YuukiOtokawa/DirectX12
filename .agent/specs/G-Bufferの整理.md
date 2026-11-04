# G-Bufferの整理

- 状態: 段階1 実装確認済み（コミット `09700e0`） / 段階2 実装確認済み（未コミット） / 段階3 提案
- 更新日: 2026-11-04
- 動作検証: 段階1・2は実施済み（条件と結果は「受け入れ条件と検証」）。段階3は未実施
- 関連資料: [TODO.md](../../TODO.md)「PositionBufferを深度バッファに切り替える・G-Bufferの整理」

## 進捗

| 段階 | 内容 | 状態 |
| --- | --- | --- |
| 1 | PositionBufferを廃止し、Deferredで深度からワールド座標を復元する | 実装確認済み（コミット `09700e0`） |
| 2 | G-Buffer 4枚のフォーマットを見直してメモリと帯域を減らす | 実装確認済み（未コミット） |
| 2.5 | `EmissionColor` の既定値を (0,0,0,1) にする（地面の白飛びの修正） | 実装済み・ユーザー確認待ち（未コミット） |
| 3 | GeometryのARM（AO/Roughness/Metallic）書き込みのずれと、float プロパティの既定値を直す | 提案 |

段階2の作業項目:

- [x] `RenderTargetManager` の4枚のフォーマット変更。`GBUFFER_FORMATS` にまとめ、初期化とリサイズの生成処理を `CreateGBuffers` に共通化
- [x] `RenderManager::CreatePipelineForPass` のRTVフォーマットを `GBUFFER_FORMATS` から取る
- [x] `Common.hlsli` に八面体エンコード・デコード関数を追加（fxcで通るよう `select()` は使わない）
- [x] `Geometry.hlsl` / `ProcedualTexture.hlsl` の書き込み（法線の正規化とエンコード）
- [x] `Deferred.hlsl`（法線デコード、深度による背景判定、IBLを正規化法線で計算、シルエットのフェード削除）
- [x] ビルド、実行時シェーダー、デバッグレイヤーの確認
- [x] 画面の見比べ、影、リサイズ（ユーザー確認済み）

## 目的と対象範囲

Deferredのジオメトリパス（G-Buffer）を整理し、GPUメモリと帯域を減らす。

- シェーダー内で計算できる情報（ワールド座標）はG-Bufferに持たない。
- 各バッファのフォーマットを、保存する値の範囲と精度に合わせる。

対象外:

- Forward描画、Bloom、ポストプロセスのバッファ（LightedColorBuffer、BloomMip、PostProcessBufferなど）
- シャドウマップ
- リサイズ中に描画が引き伸ばされる挙動（`ApplyPendingResizes` 側の問題として別に扱う）

## 期待する挙動

- 見た目は原則として変えない。変わる場合は理由を本書に記録し、ユーザーの確認を取る。
- Game View / Scene View、影、リサイズ後の描画が崩れない。
- D3D12デバッグレイヤーでERRORやCORRUPTIONが出ない。

## 構成

### 段階1の後（現在）

1920x1080固定、すべて `R16G16B16A16_FLOAT`。1枚約15.8MB、合計約63MB。

| RT | 名前 | 書き込み | Deferredで使うもの |
| --- | --- | --- | --- |
| 0 | Color | ベースカラー | rgb（aは未使用） |
| 1 | Normal | `法線 × 補間後の長さ`、w=1 | xyz、長さ（背景判定・シルエットのフェード） |
| 2 | Material | ARMマップ × パラメータ | r=AO（未使用）、g=Roughness、b=Metallic |
| 3 | Emission | EmissionColor | rgb |
| t2 | 深度 | メイン深度バッファ（`R32_TYPELESS`、SRVは `R32_FLOAT`） | ワールド座標の復元 |

### 段階2の案（合意済み）

| RT | 名前 | フォーマット | 中身 |
| --- | --- | --- | --- |
| 0 | Color | `R8G8B8A8_UNORM` | rgb=ベースカラー、a=空き |
| 1 | Normal | `R16G16_SNORM` | 正規化した法線の八面体エンコード |
| 2 | Material | `R8G8B8A8_UNORM` | r=AO、g=Roughness、b=Metallic、a=空き |
| 3 | Emission | `R11G11B10_FLOAT` | rgb=HDRの発光（負の値とアルファは持たない） |

1枚約8.3MB、合計約33MB。

- Color: テクスチャはsRGB変換なしの8bitで読み込んでいる（`TextureLoader.cpp`）ので、8bitで保存しても精度は落ちない。
- Emission: EmissionColor は `[HDR]` 指定なので浮動小数のまま小さくする。
- 背景判定は法線の長さから深度（`depth >= 1.0`、クリア値1.0）に置き換える。
- シルエットのフェード（`smoothstep(0.1, 0.9, normalLength)`）は削除する。理由は「現在の実装と差異」を参照。

TODOにある「float2個分の余り」は、この案では残らない（空くのはColorとMaterialのaチャンネル8bitのみ）。
ベロシティなどfloat2が必要になったら、専用の `RG16F` を追加する。Normalを `RGBA16F` のままにしてba成分を空ける案もあるが、使い道が決まるまでは約8MB多く使う。

### 段階3の案（提案）

Geometryの書き込みを、Deferredの読み方（r=AO、g=Roughness、b=Metallic）に合わせる。

```hlsl
float4 arm = _ArmMap.Sample(Sampler, input.TexCoord);
output.Material = float4(arm.r, arm.g * Material.Roughness, arm.b * Material.Metallic, 0.0f);
```

Specular と NormalWeight（法線強度として使用中）は書き込みに含めない。見た目が大きく変わるので、段階2と分けて確認する。

## 公開API・編集項目

- `RenderTargetManager::GetPositionBuffer()` / `RenderManager::GetPositionBuffer()` を削除し、`GetDepthTexture()` を追加した（段階1）。
- `RenderManager::TEXTURE_TYPE::POSITION` を `DEPTH` に改名した（t2のまま。ルートシグネチャは変更なし）。
- `CAMERA_CONSTANT` と `CameraConstantBuffer` に `InverseViewProjection` を追加した（段階1）。
- `PS_OUTPUT_GEOMETRY` は Color / Normal / Material / Emission の4出力（SV_TARGET0〜3）。G-Bufferへ書く独自シェーダーはこれに合わせる。
- Inspector項目、保存形式への影響はない。

## 処理の流れと所有権

- `RenderTargetManager` がG-Buffer、メイン深度バッファ、深度のSRV（`m_DepthTexture`）を所有する。
  - 深度バッファは `InitBackBufferAndDepth` で1920x1080で作る（G-Bufferと同じサイズにする必要がある）。
  - 深度のSRVは `InitGBuffers` で作り、スワップチェーンのリサイズ時（`ApplyPendingResizes`）に作り直す。呼び出し元がGPUの完了を待ってから実行する。
- 1フレームの深度の状態:
  1. G-Bufferパス（`DrawGeometryBegin`）: `DEPTH_WRITE` でクリアして書き込む。
  2. `ResolveDeferredLighting`: `DEPTH_WRITE → PIXEL_SHADER_RESOURCE` にしてDeferredで読み、パスの最後に `DEPTH_WRITE` に戻す。
  3. `BeginForwardPass`: `DEPTH_WRITE` のまま深度テストに使う。
- ワールド座標の復元（`Deferred.hlsl`）: 深度を `Load` で読み、UVをNDCに変換して `InverseViewProjection` を掛け、wで割る。

## 現在の実装と差異

### 段階1で変更したファイル（コミット `09700e0`）

- `Engine/Source/Render/RenderTargetManager.h` / `.cpp`: `m_PositionBuffer` を削除、`CreateDepthTexture` を追加、初期の深度バッファを1920x1080に変更
- `Engine/Source/Render/RenderManager.h` / `.cpp`: MRT 4枚化、深度のバリア、`TEXTURE_TYPE::DEPTH`
- `Engine/Source/Component/Camera/Camera.cpp`: `InverseViewProjection` の書き込み
- `Engine/Shader/Common.hlsli` / `Deferred.hlsl` / `Geometry.hlsl` / `ProcedualTexture.hlsl`
- `Editor/GUIController/GraphicsDebugWindowController.cpp`: Position表示を削除

### 既存の問題（段階2・3の判断材料）

- 法線の長さはシルエットを表していない。`mul(float4(normal, 0), World)` にワールド行列のスケールが入るため、スケール5のGroundPlaneは長さ5になる。スケール0.9未満の物体ではスペキュラが弱まる。
- IBLが正規化していない法線で `acos(normal.y)` を計算していた（`Deferred.hlsl`）。スケール5の地面では `acos(5)` となりNaNになる。段階2で正規化した法線を使うよう修正した。
  - 段階2の後も地面は白飛びしたままだった。原因は既定の EmissionColor が (1,1,1,1) になっていること（ユーザー確認: (0,0,0,1) にすると本来のテクスチャが見える）。
  - 原因の箇所: `Common.hlsli` の `float4 EmissionColor; // [HDR] "Emission Color"` に `[Default(...)]` がない。`ShaderMetadata.cpp` の `ParseDefaultValue` は、既定値の指定がない float4 を (1,1,1,1) にする。`Material.cpp` / `Material.h` の既定値 (0,0,0,1) は、シェーダーのメタデータが使われる経路では効いていない。
  - 段階2.5で修正: `Common.hlsli` のコメントに `[Default(0,0,0,1)]` を足した（`CustomMaterial.hlsl` の `_Emission` と同じ書き方）。
  - 同じ理由で、`Metallic` / `Specular` / `Roughness` / `NormalWeight`（float、既定値の指定なし）は既定値が0になる。`Material::SetShader` はメタデータの既定値でバッファを埋めてから `UpdateLegacyMembersFromBuffer()` で `Material.cpp` の値（0 / 0.5 / 1 / 1）を上書きするので、実際に使われるのは0（コードで確認）。
  - この結果、NormalWeight=0 なので法線マップの強さが既定で0（フラット）になり、Roughness=0 になる。
- ARMの掛け算のずれ: Geometryは ARM × (Metallic, Specular, Roughness, NormalWeight) を書き、Deferredは r=AO、g=Roughness、b=Metallic として読む。
  - 実際の既定値（すべて0、上記）では Material が (0,0,0,0) になり、Deferredから見ると AO=0、Roughness=0.01（下限）、Metallic=0。
  - 以前「既定で Metallic=1 として描かれている」と書いたのは `Material.cpp` の値を前提にした誤り。
  - float の既定値だけを `Material.cpp` に合わせると、ずれたままの読み方で Roughness=0.5、Metallic=1 になり全物体が金属になる。既定値の修正はARMの修正と同時に行う（段階3）。
  - AOはDeferredで未使用。
- `SpriteRenderer` はMeshRendererではないのでDeferred側のリストに入り、正射影のカメラ定数で上書きする（`InverseViewProjection` は0になる）。Deferredより前に描かれると座標の復元が崩れる。段階1以前から `CameraPosition` も同じ影響を受けていた。未対応。
- 深度はImGui描画時点で `DEPTH_WRITE` 状態なので、デバッグ表示はしていない。表示するにはフレーム末で `PIXEL_SHADER_RESOURCE` に遷移させる仕組みが必要。

## 受け入れ条件と検証

### 段階1（2026-11-04）

- 静的確認: 変更した10ファイルがCRLFのまま。旧名（PositionBuffer、TexturePosition、`TEXTURE_TYPE::POSITION`）への参照なし。
- C++ビルド: 成功。VS 18 Community の `vcvars64.bat` で初期化し、`msbuild DirectX12.slnx /t:Build /p:Configuration=Debug /p:Platform=x64 /m`。エラー0件。警告は変更していない行のみ。
- シェーダー: Playerビルドのオフラインコンパイルで全シェーダーが成功。エディター起動時の実行時コンパイルでもLogウィンドウにエラーなし。
- 画面・操作: Debugの `Build\x64\Debug\AsteroidEditor.exe` をリポジトリルートからcdb経由で起動。
  - D3D12デバッグレイヤーは有効（`D3D12SDKLayers.dll` を読み込み）。起動から終了までERRORやCORRUPTIONなし。
  - ユーザー確認: 見た目は変わっていない。影も問題なし。リサイズ後に描画は崩れない。
  - 終了時の `Live Object` 警告（42個）は以前から出ている（ユーザー確認）。

### 段階2（2026-11-04）

- 静的確認: 旧方式（法線の長さ、`origLen`、`normalLength`）への参照なし。変更ファイルはCRLFのまま。
- C++ビルド: 成功（段階1と同じ構成・コマンド）。エラー0件。HLSLの警告は以前からある行のみ。
- シェーダー: オフラインコンパイル成功。エディター起動時の実行時コンパイル（fxc）でもLogウィンドウにエラーなし。
- 画面・操作（Claudeが確認した範囲）:
  - Debug版をcdb経由で起動。D3D12デバッグレイヤー有効でERROR / WARNING / CORRUPTIONなし。
  - Scene Viewで地面にライティングがかかり、Debug表示のColor Buffer（8bit）にテクスチャが正しく入っている。
  - 地面は白飛びしたまま（上記「既存の問題」）。
- ユーザー確認（2026-11-04）: ビルド、起動、見た目、影、リサイズとも問題なし。
  - ファイルピッカー表示中に `wil::ResultException` の初回例外が出るが、以前から出ており停止はしない（G-Bufferとは無関係）。

### 段階3

未実施。

## 未決定事項

- 空きチャンネル（Color.a、Material.a）の使い道。
- 段階3（ARMの修正）を行うか、行う時期。
