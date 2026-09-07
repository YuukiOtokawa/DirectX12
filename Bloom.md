# ブルーム実装メモ

> 実装方針を決めるまでの検討をまとめたもの。実装しながら参照する用。
> 作成: 2026-09-07 / 対象ブランチ: `Bloom`

> **実装状況（2026-09-07）: 一通り実装済み。ビルド未確認。**
> 新規 .cpp なしで完了。`BloomComponent` は作らず、パラメータは `RenderManager::BloomSettings` に持たせ、
> Graphics Debug Window から調整できるようにした。

---

## 0. 決定事項

| 項目 | 決定 |
|---|---|
| 実装方式 | **案X: 専用メソッド `RenderManager::ApplyBloom()` にハードコード** |
| アルゴリズム | ミップピラミッド + 各段で分離ガウス（URP方式） |
| シェーダ構成 | `Bloom.hlsl` 1ファイル / 5エントリポイント |
| 元絵の参照 | SRVスロット **t7** に `SCENE_COLOR` を新設 |
| パラメータ | 当面 `RenderManager` のメンバ。最後に `BloomComponent` 化 |
| 退避バッファ | **不要**（2-2 参照） |

### なぜ案Xか

汎用ポストプロセスチェーン（`PostProcessComponent` → `ApplyPostProcess()`）は
「1パス = 入力1枚 + 出力1枚、全部フル解像度」という前提で作られている。
ブルームはこの前提を2つとも破る。

1. 合成パスが「元のシーン」と「ブラー済み輝度」の2枚を同時に要求する
2. ミップごとに解像度が変わる

これを汎用チェーン側で吸収しようとすると、入出力をリソース名で解決する
ミニレンダーグラフ（案Y）が必要になり、実装量が桁違いになる。

**Unity も同じ判断をしている。** URP のブルームは `PostProcessPass.cs` の
`SetupBloom()` にベタ書きで、汎用パスの一要素ではない。HDRP はさらにハードコード度が高い。
ユーザー拡張は「任意のパスをグラフで繋ぐ」のではなく
「決められた injection point に自分のパスを差す」形で提供している。

なお **Unity 6 の RenderGraph は案Yではない**。あれはリソースの寿命管理と
バリア自動挿入のための内部機構で、RenderGraph 化後もブルームは C# にベタ書きのまま。
「レンダーグラフを入れれば宣言的に書ける」わけではない、という点は誤解しやすいので注意。

---

## 1. 先に潰しておく地雷

### 1-1. SRVスロット t7 が空いている（好都合）

ルートシグネチャは `rootParameters[4..11]` を t0〜t7 に割り当てているが、
`TEXTURE_TYPE` は `BASE_COLOR`(t0) 〜 `SHADOW`(t6) までしか使っていない。

**ルートシグネチャを一切変更せずに参照テクスチャを1枚増やせる。**
合成パスの最大の障害だった「2枚同時参照」が enum 1行 + hlsli 1行で解決する。

```cpp
// RenderManager.h
enum class TEXTURE_TYPE {
    BASE_COLOR = (int)CONSTANT_TYPE::SUBSET + 1,
    NORMAL, POSITION, MATERIAL, EMISSION, ENVIRONMENT, SHADOW,
    SCENE_COLOR,   // ← 追加。t7、ルートパラメータ11に対応済み
};
```

```hlsl
// Common.hlsli
Texture2D<float4> TextureSceneColor : register(t7);
```

`SetTexture()` はスロットごとに独立した `SetGraphicsRootDescriptorTable` を叩くだけなので、
既存パスへの副作用はゼロ。

> Unity も `Blit` が入力1枚しか渡せない同じ制約を抱えていて、
> `cmd.SetGlobalTexture(_SourceTexLowMip, lowMip)` という同じ解き方をしている。

### 1-2. `RENDER_TARGET` が幅・高さを持っていない

```cpp
struct RENDER_TARGET {
    ComPtr<ID3D12Resource>  Resource;
    unsigned int            SRVIndex;
    unsigned int            RTVIndex;
    D3D12_GPU_DESCRIPTOR_HANDLE SRVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;
};
```

ミップごとのビューポート設定にも、シェーダへ渡すテクセルサイズにも要る。
`RenderTargetFactory::Create()` は既に `width` / `height` を引数で受け取っているので、
**メンバを2つ足して代入するだけ**。既存コードへの影響なし。**最初にやる。**

### 1-3. 1つの .hlsl = 1つの PSO → エントリポイント引数化で解決

`CreatePipeline()` はエントリポイントを決め打ちでコンパイルしている。

```cpp
compileShader(ShaderFile, "vtx", "vs_5_1", &vsBlob);
compileShader(ShaderFile, "pix", "ps_5_1", &psBlob);
```

デフォルト引数を足せば**既存の8箇所の呼び出しも `RegisterDynamicPostProcess` も無変更**で済む。

```cpp
ComPtr<ID3D12PipelineState> CreatePipeline(
    const char* ShaderFile,
    const DXGI_FORMAT* RTVFormats,
    unsigned int NumRenderTargets,
    RenderPassType passType = RenderPassType::DeferredOpaque,
    const char* vsEntry = "vtx",     // ← 追加
    const char* psEntry = "pix");    // ← 追加
```

関数本体も置換2箇所だけ。

**コストはほぼゼロ:**

- 実行時: PSO は出自のファイルを知らない。4パス = 4 PSO はどちらでも同じ。完全にゼロ
- 起動時: むしろ軽くなる。4ファイル案だと `D3DCompileFromFile` 8回 +
  **同一の `vtx` を4回コンパイルする無駄**が出る。1ファイル案なら vtx 1回 + pix 4回

**既知の副作用（今は無害）:** メタデータのキーはファイル名由来、PSO のキーは PSO 名。

```cpp
ShaderMetadata meta = ParseShaderMetadata(ShaderFile);
m_ShaderMetadataMap[meta.shaderName] = meta;   // キーは "Bloom" ひとつ
```

1ファイルから `"BloomPrefilter"` など複数 PSO を作ると、Inspector の
`GetShaderMetadata(passName)` がヒットしない。
ただし**案Xではブルームのメタデータを使わない**（PSO は `Deferred` 等と同じ固定登録、
パラメータは `BloomComponent` が自前で持つ）ので問題にならない。
将来ユーザー側の動的ポストプロセスで多エントリを使いたくなったら、
`m_ShaderMetadataMap[psoName] = meta;` にするか PSO名→ファイル名の対応表を持つ。

**`pDefines` も空いている。** `D3DCompileFromFile` の第2引数が `nullptr` のまま。
Unity の `multi_compile` 相当をやりたくなったらここ。
使い分けは「中身が別物のパス = エントリポイント分け」「品質バリアント = defines」。
ブルームに必要なのは前者だけ。

### 1-4. ★ `SHADER_DIR` が構成で切り替わる

```cpp
// Code/Utility/ResourcePath.h
#ifdef _DEBUG
#define SHADER_DIR "Code/Shader/"      // リポジトリ内を直接参照
#else
#define SHADER_DIR "Resource/Shader/"  // 配布用
#endif
```

**シェーダを編集したら必ず両方に反映すること。**
Debug で開発している限り気づかず、Release で
`D3DCompileFromFile` がファイルを見つけられず `MessageBoxA` が出る。

2026-09-07 時点で既にズレている:

```
Only in Code/Shader: Bloom.hlsl
Files Code/Shader/CustomMaterial.hlsl and Resource/Shader/CustomMaterial.hlsl differ
Files Code/Shader/PostColorAdjust.hlsl and Resource/Shader/PostColorAdjust.hlsl differ
```

`Common.hlsli` に t7 を足すときも両方に入れないと Release で壊れる。
**将来的にはビルド後イベントで `Code/Shader/` → `Resource/Shader/` をコピーする形にすべき。**
手作業コピー運用だとこの手のズレは必ずまた起きる。

### 1-5. バリアの待機状態規約

既存 `ApplyPostProcess()` は
**「ping-pong する2枚は常に `PIXEL_SHADER_RESOURCE` で待機し、
描く直前だけ `RENDER_TARGET` に遷移して戻す」**という規約で回っている。
ブルーム用バッファも同じ規約に揃える。崩すとデバッグレイヤーが黙って荒れる。

### 1-6. `ApplyBloom()` の入口/出口契約

`ApplyPostProcess()` と同じにする。

- **入口**: `LightedColorBuffer` が `RENDER_TARGET` 状態で、合成済みの絵を持っている
- **出口**: 同じく `RENDER_TARGET` 状態で、ブルーム適用後の絵を持っている

`GetCurrentTarget() == BACK_BUFFER` の early return も同様に必要。

---

## 2. 設計

### 2-1. 呼び出し位置

`Code/Render/RenderSystem.cpp` の 7. の直前に1行挟むだけ。

```cpp
// 7. Apply Post-Process
if (renderManager) {
    renderManager->ApplyBloom();          // ← 追加
    renderManager->ApplyPostProcess();
}
```

### 2-2. ★ 退避バッファ（SceneColorCopy）は不要

案Xに決めたことで前提が変わった。
**ブルームチェーンは `LightedColorBuffer` に一度も書き込まない。**
プリフィルタで読むだけ、以降は専用バッファ内で完結、合成時にもう一度読むだけ。
つまり元絵は最後まで無傷で残っている。退避用の16MBと `CopyResource` が丸ごと不要。

代わりに必要なのは**合成の出力先**。
`LightedColorBuffer` を読みながら同じバッファには書けないので、
`PostProcessBuffer1` に書いてから戻す。

```
Composite: LightedColor(t0) + MipUp[0](t7) → PostProcessBuffer1
           → CopyResource で LightedColor へ戻す
```

フルスクリーンの `CopyResource` 1回（1920x1080 RGBA16F ≒ 読み書き32MB）は
最近のGPUなら0.1msも掛からない。まずこれで組んで、プロファイルに出てきたら考える。

### 2-3. バッファ構成 — チェーンは2本

ブラーが H と V で ping-pong するため `MipUp[]` / `MipDown[]` の**2本**が要る
（URP も同じ）。既存の `m_BloomBuffer`（1/8固定・完全未使用）は破棄する。

```cpp
static constexpr int BLOOM_MAX_MIPS = 5;
std::unique_ptr<RENDER_TARGET> m_BloomMipUp[BLOOM_MAX_MIPS];
std::unique_ptr<RENDER_TARGET> m_BloomMipDown[BLOOM_MAX_MIPS];
```

サイズは半解像度スタートで半分ずつ。
1920x1080 → 960x540 / 480x270 / 240x135 / 120x67 / 60x33。
2本合わせて約11MB。

ミップ数は URP 方式で実行時に決める（解像度をハードコードしない）。

```cpp
int maxSize    = max(width, height);
int iterations = (int)floor(log2((float)maxSize)) - 1;
int mipCount   = clamp(iterations, 1, BLOOM_MAX_MIPS);
```

> **注意:** 整数除算で奇数が出る（135/2 = 67 で1行落ちる）。これは正常。
> ただし**テクセルサイズは実際に確保したサイズから計算すること。**
> 「2の累乗で割った想定値」を使うとズレる。だから 1-2 が必要になる。
> `max(1, size >> 1)` のガードも忘れずに。

### 2-4. パス表

これが `ApplyBloom()` の全体。

| # | PSO | 入力 t0 | 入力 t7 | 出力 | サイズ変化 |
|---|---|---|---|---|---|
| 1 | `BloomPrefilter` | LightedColor | — | MipDown[0] | 1/1 → 1/2 |
| 2 | `BloomBlurH` | MipDown[i-1] | — | MipUp[i] | **半分に落ちる** |
| 3 | `BloomBlurV` | MipUp[i] | — | MipDown[i] | 等倍 |
| 4 | `BloomUpsample` | MipDown[i] | 低ミップ側 ※ | MipUp[i] | 拡大 |
| 5 | `BloomComposite` | LightedColor | MipUp[0] | PostProcessBuffer1 | 1/2 → 1/1 |

※ 最下段のみ `MipDown[i+1]`、それ以外は `MipUp[i+1]`

- パス2・3 を `i = 1 .. mipCount-1` で回す
- パス4 を `i = mipCount-2 .. 0` で逆順に回す
- **ダウンサンプルはブラーHが兼ねている。**専用のダウンサンプルパスは無い

### 2-5. 定数バッファ

`SetConstant(CONSTANT_TYPE::SUBSET, ...)` で b3 に流す。
パスごと・ミップごとに値が変わるので、Inspector の Material 経由では渡せない。
`ApplyBloom()` が自前で組み立てる。float4 境界ぴったりの32バイト。

```cpp
struct BloomConstants {
    float Threshold;
    float Knee;
    float Intensity;
    float Scatter;
    float InputTexel[2];    // 1/入力幅, 1/入力高さ
    float LowMipTexel[2];   // アップサンプル時の低ミップ側
};
```

> **タップのオフセットは「入力テクスチャ」のテクセル基準。**出力側ではない。
> 間違えても絵は出てしまう（少しボケ方がおかしいだけ）ので気づきにくい。

---

### 2-6. ApplyBloom は ping-pong ではない

`ApplyPostProcess()` の `currentInput` / `currentOutput` を差し替えた形にはならない。
あちらは **2枚固定の ping-pong** で、成立している前提が3つある。
入力は常に「直前の出力」1枚、サイズは全パス同じ、バッファは2枚。
ブルームはこの3つを全部破る。

| | ApplyPostProcess | ApplyBloom |
|---|---|---|
| バッファ数 | 2枚 | 11枚（MipUp 5 + MipDown 5 + PostProcessBuffer1） |
| 入出力の決まり方 | `swap` で機械的 | ミップ番号と向き（下り/上り）で決まる |
| サイズ | 全パス同じ | 毎パス変わる |
| 入力枚数 | 常に1枚 | Upsample と Composite は2枚 |
| ループ | 1本 | 下り1本 + 上り1本（逆順） |

共通なのは次の3つだけ。

1. 入口/出口の契約（1-6）… メソッドの外から見た振る舞いの話で、中身の話ではない
2. バリアの作法（1-5）
3. **1回の描画の定型** … ここだけは完全に同じ

なので流用すべきはループではなく定型のほうで、`RenderManager::DrawFullScreenPass()` に切り出した。

```cpp
void DrawFullScreenPass(const char* psoName,
                        const RENDER_TARGET* input,      // t0
                        const RENDER_TARGET* inputLow,   // t7、nullptr可
                        RENDER_TARGET* output,
                        const void* constantData = nullptr,
                        unsigned int constantSize = 0);
```

バリア（PSR→RT→PSR）、ビューポート（`output->Size` から）、クリア、PSO、テクスチャ、定数、Draw をまとめて面倒みる。
これで `ApplyBloom()` は 2-4 のパス表をそのまま並べるだけになり、バリアの書き忘れも起きない。
`ApplyPostProcess()` も後でこれを使う形にリファクタできる。

### 2-7. LightedColorBuffer の状態遷移

```
入口                      LightedColor = RENDER_TARGET

1. barrier                LightedColor: RT -> PSR
2. Prefilter              LightedColor を読む -> MipDown[0]
3. 下り / 上り連鎖         ブルームバッファ内で完結
4. Composite              LightedColor(t0) + MipUp[0](t7) -> PostProcessBuffer1
5. barrier                LightedColor:       PSR -> COPY_DEST
                          PostProcessBuffer1: PSR -> COPY_SOURCE
6. CopyResource           PostProcessBuffer1 -> LightedColor
7. barrier                LightedColor:       COPY_DEST   -> RENDER_TARGET
                          PostProcessBuffer1: COPY_SOURCE -> PSR

出口                      LightedColor = RENDER_TARGET  OK
                          PostProcessBuffer1 = PSR       OK（次の ApplyPostProcess の前提）
```

**ステップ1で PSR に落としたら Composite まで上げないこと。**Prefilter と Composite の2回読むため。

---

## 3. シェーダ仕様

### 3-1. ファイル構成

```hlsl
#define CUSTOM_MATERIAL_CBUFFER   // 共有マテリアル定義を殺して b3 を自前で使う
#include "Common.hlsli"

cbuffer BloomConstantBuffer : register(b3) { ... };

PS_INPUT  vtx(VS_INPUT input) { ... }        // 5パス共通

PS_OUTPUT pixPrefilter(PS_INPUT input) { ... }
PS_OUTPUT pixBlurH    (PS_INPUT input) { ... }
PS_OUTPUT pixBlurV    (PS_INPUT input) { ... }
PS_OUTPUT pixUpsample (PS_INPUT input) { ... }
PS_OUTPUT pixComposite(PS_INPUT input) { ... }
```

`CUSTOM_MATERIAL_CBUFFER` は `Common.hlsli` が既に用意している仕組み。

登録側:

```cpp
DXGI_FORMAT fmt[] = { DXGI_FORMAT_R16G16B16A16_FLOAT };
const char* f = SHADER_DIR "Bloom.hlsl";
m_PipelineState["BloomPrefilter"] = CreatePipeline(f, fmt, 1, RenderPassType::PostProcess, "vtx", "pixPrefilter");
m_PipelineState["BloomBlurH"]     = CreatePipeline(f, fmt, 1, RenderPassType::PostProcess, "vtx", "pixBlurH");
m_PipelineState["BloomBlurV"]     = CreatePipeline(f, fmt, 1, RenderPassType::PostProcess, "vtx", "pixBlurV");
m_PipelineState["BloomUpsample"]  = CreatePipeline(f, fmt, 1, RenderPassType::PostProcess, "vtx", "pixUpsample");
m_PipelineState["BloomComposite"] = CreatePipeline(f, fmt, 1, RenderPassType::PostProcess, "vtx", "pixComposite");
```

### 3-2. サンプラー

**`SamplerClamp`(s1) を使う。**
確認済みで `MIN_MAG_MIP_LINEAR` + `ADDRESS_MODE_CLAMP`、ブルームの条件を満たす。
`Sampler`(s0) は `ANISOTROPIC` + `WRAP` なので、フルスクリーンのダウンサンプルでは
画面端が反対側から滲む。

### 3-3. Prefilter

RT が全て `R16G16B16A16_FLOAT` なので 1.0 超えの HDR 値が保持されている。
閾値抽出がちゃんと意味を持つ環境。

**輝度は `Max3(r, g, b)` を使う（相対輝度の dot ではない）。**
`dot(color, float3(0.2126, 0.7152, 0.0722))` だと純青のネオンのような
彩度の高い色を拾い漏らす。URP も `Max3` を使っている。

```hlsl
half brightness = Max3(color.r, color.g, color.b);
half softness = clamp(brightness - Threshold + Knee, 0.0, 2.0 * Knee);
softness = (softness * softness) / (4.0 * Knee + 1e-4);
half multiplier = max(brightness - Threshold, softness) / max(brightness, 1e-4);
color *= multiplier;
```

ハードな閾値ではなくソフトニー（Karis）。

### 3-4. Blur

**H は9タップ、V は5タップ。**URP の重みをそのまま使える（対称なので片側のみ記載、総和1.0）。

```
H (中心から外へ): 0.22702703, 0.19459459, 0.12162162, 0.05405405, 0.01621622
V (中心から外へ): 0.22702703, 0.31621622, 0.07027027
```

H が広いのは、ここで同時に半分に縮めるため。
オフセットはバイリニアの半テクセル位置を使い、タップ数の割に広い範囲を拾う。

### 3-5. Upsample

単純な加算 `high + low` でも動く。`Scatter` を入れると調整の効きがよくなる。

```hlsl
return lerp(highMip, lowMip, Scatter);
```

**加算は PSO のブレンドではなくシェーダ内でやる。**
`TextureBaseColor`(高解像度側) と `TextureSceneColor`(低解像度側) の2枚を読んで足せば、
`RenderPassType` に加算ブレンド用の型を増やさずに済む。t7 がここでも効く。

### 3-6. Composite

```hlsl
return scene + bloom * Intensity;
```

> ブルームは**トーンマップの前**に適用する。
> `Exposure` が b0 にあるが、トーンマップがどこで掛かっているか
> （Deferred内かポスト側か）は要確認。順序を間違えると強度の調整が効かなくなる。

---

## 4. 実装手順

**絵が見える単位で刻む。**ブルームは途中経過が見えないとどこで壊れているか分からない。

| # | やること | 見て確認すること |
|---|---|---|
| 0 | ✅ `RENDER_TARGET` に `Size`(Vector2) を追加、Factory で代入 | ビルドが通る |
| 1 | ✅ `CreatePipeline` にエントリポイント引数 | 既存シェーダが全部今までどおり動く |
| 2 | ✅ `TEXTURE_TYPE::SCENE_COLOR`(t7) + `TextureSceneColor` | **合成が真っ黒ならここを疑う** |
| 3 | ✅ Graphics Debug Window にミップ表示 + Debug View | 各ミップがサムネイルで見える |
| 4 | ✅ ミップチェーン確保（MipUp/MipDown 各5段） | デバッグ表示で各ミップが出る |
| 5 | ✅ Prefilter（Karis average + ソフトニー） | 明るい所だけ抜ける、閾値が効く |
| 6 | ✅ BlurH(9タップ) / BlurV(5タップ) 連鎖 | 最小ミップがボケた輝点の塊になる |
| 7 | ✅ Upsample 連鎖（9タップ tent） | MipUp[0] が滑らかに広がる |
| 8 | ✅ Composite + CopyResource で戻す | 完成 |
| 9 | ⬜ `BloomComponent` で Inspector 化（任意） | シーンごとに保存したくなったら |

**ステップ2で必ず止まって確認すること。**
t7 が本当に届いているかをここで確かめないと、
後で「合成が真っ黒」になったときに原因の切り分けができなくなる。

**ステップ3のデバッグ表示を先に作ること。**
ステップ4〜7は全部「中間バッファが正しいか」の確認なので、これがないと総当たりになる。
既存の `SetPipelineState("PostProcess")` + `SetTexture(BASE_COLOR, 任意のRT)` +
`DrawScreen()` の組み合わせでそのまま作れる。

### パラメータは最後に Component 化する

ステップ8まではこれで十分。

```cpp
// RenderManager.h に一旦ベタ書き
float m_BloomThreshold = 1.0f;
float m_BloomKnee      = 0.5f;
float m_BloomIntensity = 1.0f;
float m_BloomScatter   = 0.7f;
```

絵が出て値の効き方が分かってから Component 化する。
先に作ると要らないパラメータまで生やしがち。

> Unity も `Bloom : VolumeComponent` はパラメータを持つだけで描画コードを一切持たない。
> **パラメータのオーサリングとパスの実行構造を完全に分離している。**
> 現状の `PostProcessComponent` は両方を担っているので、ブルームはこの分離に倣う。

---

## 5. 触るファイル一覧

### 新規作成

- `Code/Shader/Bloom.hlsl` ✅ 実装済み（5エントリポイント）
- `Resource/Shader/Bloom.hlsl` ✅ 同期済み（1-4 のため必須）
- `Code/Component/BloomComponent.h` / `.cpp` ← ステップ9のみ。**それまで不要**

**ステップ8（合成完成）まで、新規の .cpp はゼロで到達できる。**

### 既存の修正

| ファイル | 内容 |
|---|---|
| `Code/Shader/Common.hlsli` + `Resource/Shader/Common.hlsli` | `TextureSceneColor : register(t7)` を1行（**両方**） |
| `Render/RenderManager.h` | `RENDER_TARGET` に `Width`/`Height`、`TEXTURE_TYPE::SCENE_COLOR`、`CreatePipeline` の引数追加、`ApplyBloom()` 宣言 |
| `Render/RenderManager.cpp` | `CreatePipeline` 本体、PSO登録5つ、`ApplyBloom()` 本体 |
| `Render/RenderTargetFactory.cpp` | `Width`/`Height` の代入（実質1〜2行） |
| `Render/RenderTargetManager.h/.cpp` | `m_BloomBuffer` 削除、ミップチェーン確保、ゲッター |
| `Render/RenderSystem.cpp` | `ApplyBloom()` の呼び出し1行 |
| `GUIController/GraphicsDebugWindowController.h/.cpp` | ブルームのパラメータUIとミップのサムネイル表示 |

### vcxproj

`Bloom.hlsl` の `FxCompile` エントリは既にあり、両構成とも `ExcludedFromBuild`。
VS がビルド時に fxc で勝手にコンパイルして失敗する事故は起きないので触らなくてよい。
`.cpp` を新規追加するとき（ステップ9）は VS の「追加 > 新しい項目」でやれば自動で入る。

---

## 6. 付録: Unity(URP) との対応

参考にした実装との対応表。

| 項目 | URP | 本実装 |
|---|---|---|
| 実装場所 | `PostProcessPass.cs` の `SetupBloom()` | `RenderManager::ApplyBloom()` |
| ミップ配列 | `m_BloomMipUp[]` / `m_BloomMipDown[]`、`k_MaxPyramidSize` | 同じ、`BLOOM_MAX_MIPS = 5` |
| ミップ数 | `Mathf.FloorToInt(Mathf.Log(maxSize, 2f) - 1)` を clamp | 同じ |
| 解像度解決 | RT のディスクリプタから `Blit` が自動設定 | `RENDER_TARGET::Width/Height` から手動設定 |
| 2枚目の入力 | `cmd.SetGlobalTexture(_SourceTexLowMip, ...)` | `SetTexture(SCENE_COLOR, ...)` (t7) |
| シェーダパス | 1ファイル4パス（`Bloom.shader`） | 1ファイル5エントリポイント |
| パラメータ | `Bloom : VolumeComponent`（描画コードを持たない） | `BloomComponent`（ステップ9） |
| リサイズ追従 | `RTHandle` が解像度倍率で自動確保 | 手動（将来 `Create(scale)` を検討） |

**借りるべきだった設計は3つ:**

1. 解像度をハードコードせず、倍率と log2 で出す
2. パラメータ（Component）と実行（RenderManager）を分離する
3. 2枚目のテクスチャはグローバルバインドで渡す（= t7 案）

---

## 7. デバッグの当たり

| 症状 | 疑うところ |
|---|---|
| 合成が真っ黒 | t7 が届いていない（ステップ2の検証をやり直す） |
| 全体が白飛び | Threshold が効いていない / Intensity 過大 / 合成で二重加算 |
| 滲まない・広がらない | テクセルサイズが出力側になっている / ミップ数が1 |
| 画面端が反対側から滲む | サンプラーが s0（WRAP）になっている |
| 動かすとチラつく（ファイアフライ） | Prefilter に Karis average が無い |
| ボケ方が微妙にズレる | 奇数解像度を想定値で計算している（2-3 の注意） |
| Release だけ起動時にエラーダイアログ | `Resource/Shader/` へのコピー忘れ（1-4） |

---

## 8. 特定のオブジェクトを光らせる（`[HDR]`）

ブルームは 1.0 を超えた輝度に反応する。BaseColor は**アルベド（反射率）＝比率**なので 0..1 が上限で、
1.0 を超えさせるとエネルギー保存が破れて IBL や金属反射が発散する。光らせる役目は `EmissionColor` の方。

```hlsl
// Deferred.hlsl : emission だけ照明計算を通らず素通しで足される（= 放射輝度そのもの）
output.Color.xyz = diffuse + specular * shadowFactor + emission.xyz;
```

ところが `ShaderMetadata.cpp` の `isColor = (prop.type == "float4")` により `EmissionColor` は
`ImGui::ColorEdit4` になり、**0..1 にクランプされて 1.0 超えを入力できなかった**。

そこで Unity と同じ `[HDR]` アノテーションを新設した（2026-09-07）。

| ファイル | 変更 |
|---|---|
| `Render/ShaderMetadata.h` | `ShaderProperty::isHDR` を追加 |
| `Render/ShaderMetadata.cpp` | `ParsePropertyAttributes` で `[HDR]` を解析（`isColor` も true にする） |
| `Component/MaterialPropertyInspector.h` | `isHDR` なら `ImGuiColorEditFlags_HDR \| _Float` を渡す |
| `Shader/Common.hlsli` | `EmissionColor` に `// [HDR] "Emission Color"` |
| `Shader/CustomMaterial.hlsl` | `_Emission` を `[Color]` → `[HDR]` |

`ImGuiColorEditFlags_HDR` は単体だと表示が 0..255 のままなので、`_Float` とセットで指定する必要がある
（imgui.h のコメントにもそう書いてある）。

使い方: 光らせたいマテリアルの Emission Color に 2.0〜5.0 くらいを入れる。
Threshold を超えた分がブルームになる。

### エミッシブ以外の経路

`LightColor` と IBL（`* 10`）も 1.0 を超える。強い光が白い面に当たれば diffuse は普通に 1.0 を超えるので、
エミッシブを設定しなくてもブルームは出る。動作確認だけなら Threshold を 0.3 くらいまで下げるのが早い。

---

## 8. 参考

- [Bloom.shader - Unity-Technologies/Graphics](https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.universal/Shaders/PostProcessing/Bloom.shader)
- [PostProcessPass.cs - Unity-Technologies/Graphics](https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.universal/Runtime/Passes/PostProcessPass.cs)
- [Custom rendering and post-processing in URP - Unity 6 Manual](https://docs.unity3d.com/6000.0/Documentation/Manual/urp/customizing-urp.html)
