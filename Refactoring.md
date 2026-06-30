# リファクタリング項目一覧（Code/ 以下）

`Code/` 以下の自作ソース（`assimp` / `ImGui` などの外部ライブラリは除く）を一通り読んで、
リファクタリングでやれそうなことを洗い出した。優先度順（上が高い）。

---

## 1. 命名規則の統一（最優先・影響範囲が広い）

メンバ変数の接頭辞が **ファイル・クラスどころか同じクラス内でも混在**している。

| パターン | 例 | 場所 |
|---|---|---|
| `m_` + PascalCase | `m_Components`, `m_WindowHandle`, `m_Name` | GameObject, RenderManager, Material |
| `_` + PascalCase | `_IsActive`, `_Name`, `_Objects`, `_SelectedObject`, `_CurrentTargetType` | GameObject, ObjectManager, RenderManager |
| `_` + camelCase | `_instanceID` | Object |

- `GameObject.h` は `_IsStarted` / `_IsActive` / `_Name` と `m_Components` が同居（[GameObject.h:16-21](Code/GameObject/GameObject.h)）
- `RenderManager.h` も `m_*` が大半なのに `_CurrentTargetType` だけ `_`（[RenderManager.h:202](Code/Render/RenderManager.h)）

**やること**: 接頭辞・大文字小文字のルールを1つ決めて全体を機械置換。関数の引数も `Width` / `Height` / `Resource` のように PascalCase になっており（C++ では珍しい）、ローカル変数のルールも合わせて決めたい。

---

## 2. C スタイルの ALL_CAPS 構造体を正式なクラスへ移行

`RenderManager.h` の `namespace Types` に、コメントで「○○クラスに移動」と書かれたまま放置されている構造体群がある（[RenderManager.h:12-100](Code/Render/RenderManager.h)）。

```cpp
struct VERTEX        // → VertexData クラスに移動（コメント済み）
struct MATERIAL      // → Material クラスに移動
struct ENV_CONSTANT  // → Light クラスに移動
struct CAMERA_CONSTANT, OBJECT_CONSTANT, SUBSET_CONSTANT, TEXTURE,
       CONSTANT_BUFFER, RENDER_TARGET, VERTEX_BUFFER, INDEX_BUFFER
```

- ALL_CAPS は本来マクロ用の命名で、型名としては不適切。
- 移行が途中で止まっており「正規版クラス」と「旧 struct」が二重に存在している。
- さらに `using Types::VERTEX;` 等を **namespace スコープで丸ごと再公開**しており（[RenderManager.h:106-116](Code/Render/RenderManager.h)）、`Render` 名前空間が汚染されている。

**やること**: 各構造体を責務を持つクラスへ移し切る。GPU リソース保持系（`RENDER_TARGET` / `VERTEX_BUFFER` / `TEXTURE` など）は `Render` 配下のリソースクラスに整理。

---

## 3. 神クラス RenderManager の分割

`RenderManager.cpp` が **1903 行**で全描画処理を抱えている（[RenderManager.cpp](Code/Render/RenderManager.cpp)）。1クラスに以下が混在：

- デバイス／スワップチェーン／フェンス初期化（低レベル）
- ディスクリプタヒープのアロケータ（SRV/RTV プール管理）
- 定数バッファのリングバッファ管理
- レンダーターゲット生成・リサイズ（pending 機構）
- テクスチャ読み込み（`LoadTexture`）
- PSO 生成・登録・遅延解放
- **Deferred ライティング解決 / ポストプロセス / Forward パス**（高レベルのレンダーグラフ）

低レベル（GPU リソース確保）と高レベル（パスの組み立て）が同居しているのが一番の問題。

**やること（分割案）**:
- `GraphicsDevice` … デバイス・キュー・スワップチェーン・フェンス
- `DescriptorAllocator` … SRV/RTV プール（`CreateShaderResourceView` 等）
- `ConstantBufferRing` … 定数バッファのリング管理
- `RenderTargetManager` … RT 生成・リサイズ・pending
- `RenderPipeline` / `RenderGraph` … Deferred/Forward/PostProcess のパス制御
- `TextureLoader` … `LoadTexture`

---

## 4. 文字コード崩れ（mojibake）コメントの修正 ✅ 対応済み

調査の結果、化けには2種類あった。

1. **生 Shift-JIS のまま残っていたファイル**（バイト情報が残っており無損失で復元可）
   - `Main.cpp` / `SpriteRenderer.cpp` / `Resource/resource.h`
   - → CP932 でデコードして UTF-8 で保存し直した。
2. **UTF-8 として焼き付いた化け**（元バイトが失われ機械復元不可、文脈から手書き）
   - `RenderManager.h` / `RenderManager.cpp` / `OBJLoader.cpp`
   - U+FFFD 化け（例: `//�萔�o�b�t�@`）と、高ビット落ちで純 ASCII 化けたもの（例: `//\``→`//描画`、`//uhXe[g`→`//ブレンドステート`）の両方を、周辺コードから推定して書き直した。

**BOM について**: 当初「UTF-8(BOM付き)」を想定していたが、実ビルド構成（x64 Debug/Release）には既に `/utf-8` が付いていて UTF-8 として正しく読まれることが判明。他 183 ファイルも BOM 無し UTF-8 なので、**BOM は付けず BOM 無し UTF-8 に統一**した（差分最小・既存と整合）。

**残りの推奨（任意）**: 死んでいる Win32 構成にも `/utf-8` を追加しておくと、構成を切り替えても再発しない（[DirectX12.vcxproj:85,100](DirectX12.vcxproj)）。x64 構成は対応済み。

---

## 5. 既知のバグ・デッドコードの除去

リファクタリングのついでに直すべき明確な不具合：

- **`Object::operator==` が壊れている**: 引数 `other` を一切使わず自分の `IsValid()` を返すだけ（[Object.h:13-15](Code/Object.h)）。意図通りに比較できていない。
- **return 後の到達不能コード**: `GetSRVDescriptorCPUHandle()` が `return` の後に `m_SRVDescriptorPool.pop_front();` を書いていて永遠に実行されない（[RenderManager.h:357-360](Code/Render/RenderManager.h)）。
- **呼ばれない Start**: `GameObject::ExecUpdate` で `//Start();` がコメントアウトされたまま `_IsStarted` だけ立てている（[GameObject.h:28-36](Code/GameObject/GameObject.h)）。Component に `Start()` がある以上、ライフサイクルとして繋ぐべき。
- **デストラクタの巨大コメントアウト**: `~RenderManager` の ReportLiveDeviceObjects ブロック（[RenderManager.cpp:34-46](Code/Render/RenderManager.cpp)）。`#if _DEBUG` で残すか消すか決める。

---

## 6. Material の二重管理の解消

`Material` が「動的プロパティバッファ `m_PropertyBuffer`」と「互換用の個別メンバ `m_BaseColor` など」を**両方持ち、手動で同期**している（[Material.h:39-48](Code/Render/Material.h), `UpdateBufferFromLegacyMembers` / `UpdateLegacyMembersFromBuffer`）。

- 真実の源（source of truth）が2つあり、同期忘れによるバグの温床。

**やること**: プロパティバッファ一本に寄せ、個別 getter/setter はバッファ参照に置き換える。`MaterialConstant`（POD）と `Material`（ロジック）の役割分担も整理。

---

## 7. シングルトン乱立とグローバル状態

`RenderManager` / `ObjectManager` / `ComponentFactory` がいずれも `GetInstance()` シングルトン。しかも実装パターンが2種類：

- ctor で `m_Instance = this`（RenderManager、生成タイミング外部依存）
- 遅延 `new`（ObjectManager: [ObjectManager.h:37-42](Code/Manager/ObjectManager.h)）

どちらも `delete` されずリーク放置。

**やること**: 最低限パターンを統一。将来的には明示的な所有（`Engine` クラスが各 Manager を保持して参照を配る）への移行を検討。

---

## 8. 所有権・生ポインタの整理

- `WorldInitializer` で `new VertexData()` を生ポインタで作って `SetVertexData` に渡している（[WorldInitializer.cpp:50, 93](Code/Utility/WorldInitializer.cpp)）。所有者が誰か不明瞭でリークの懸念。
- `GameObject::AddComponent` が `make_unique<T>` 直後に `dynamic_cast<T*>` している（[GameObject.h:60-66](Code/GameObject/GameObject.h)）。型は確定しているので `static_cast` か `.get()` で十分。

**やること**: 所有は `unique_ptr`/`shared_ptr` で表現し、受け渡し API を `&&` か参照で統一。

---

## 9. 名前空間の不統一

- 自作コードは `EngineCore::General` / `EngineCore::Manager` / `EngineCore::Utility` / `GUIController::Gui` に整理されているのに、描画系だけ `Render`（`EngineCore` の外）になっている（[Material.h:7](Code/Render/Material.h), [RenderManager.h:10](Code/Render/RenderManager.h)）。

**やること**: `EngineCore::Render` に寄せるなど、トップレベル名前空間を1本化。

---

## 10. ヘッダの依存・include 衛生

- `RenderManager.h` が `Main.h`（Windows/D3D の重いヘッダ群）を include しており、これを include する全 TU に伝播する（[RenderManager.h:3](Code/Render/RenderManager.h)）。
- ヘッダ内 `using namespace General;`（[ObjectManager.h:16](Code/Manager/ObjectManager.h)）— ヘッダでの `using namespace` は名前衝突の原因。
- include が `../../` の相対パス頼り（[ImGuiController.cpp:3-7](Code/GUIController/ImGuiController.cpp)）。インクルードディレクトリ設定でルート相対にしたい。
- ヘッダに巨大な inline 実装（`GetActiveTargetSize` 等、[RenderManager.h:273-290](Code/Render/RenderManager.h)）。.cpp へ移すかコンパクトに。

**やること**: 前方宣言＋ pimpl/最小 include でビルド時間と結合度を下げる。

---

## 11. シーン構築のハードコード

`WorldInitializer.cpp` がシーン内容（頂点・パス・シェーダー名・座標）を全部ベタ書き（[WorldInitializer.cpp:38-133](Code/Utility/WorldInitializer.cpp)）。

- `"Assets/field004.dds"`, `"Code/Shader/Geometry.hlsl"`, `"Geometry"`, `"Unlit"` などのマジック文字列が散在。

**やること**: TODO の「シーンの保存・読み込み」と直結する。まずはアセットパス／シェーダー名を定数化 or リソース定義に外出しし、最終的にシーンファイルから生成する形へ。

---

## 補足：着手順のおすすめ

1. **#4 文字コード**と**#5 バグ/デッドコード** … 機械的・低リスク・差分が読みやすい
2. **#1 命名規則**と**#9 名前空間** … 一括置換系、早めにやると後続が楽
3. **#2 構造体のクラス化** … #1 と連動
4. **#6 Material 二重管理** … 局所的で効果大
5. **#3 RenderManager 分割** … 最大の山。上記が片付いてから腰を据えて
6. **#7/#8 所有権・シングルトン** と **#11 シーン外部化** … 設計寄り。TODO の他項目（シーン機能・スクリプト）と合わせて進める
