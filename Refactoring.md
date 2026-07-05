# リファクタリング項目一覧（Code/ 以下）

`Code/` 以下の自作ソース（`assimp` / `ImGui` などの外部ライブラリは除く）を一通り読んで、
リファクタリングでやれそうなことを洗い出した。優先度順（上が高い）。

> **進捗（最終更新: 2026-07-06）**
> - ✅ **#4 文字コード崩れ … 対応済み**。自作189ファイル全てが「化け0・有効UTF-8」になったことを確認。
> - 🔄 commit `e88c42a`（テクスチャ差し替え時クラッシュ修正）を取り込み済み。RenderManager / Material に
>   テクスチャの**遅延解放機構**が追加され、本書の #3 / #6 に反映した。
> - 🆕 TODO.md に高優先の新項目が追加された（`assertを避ける` / `シェーダーテクスチャのエディタ設定` /
>   `PositionBuffer→深度バッファ化`）。リファクタリングと絡む点は末尾「TODO との関連」に追記。
> - 🔍 **#2 を調査**：「○○クラスに移動」とコメントされた構造体のうち、実際にクラス側へ完全移行して
>   `struct` が死んでいたのは `SUBSET_CONSTANT`（Rendererへ移動と記載）と `CONSTANT_BUFFER`（コメント無し）の
>   **2つのみ**と判明、削除した（Debug x64 ビルド通過）。他の構造体（`VERTEX`/`TEXTURE`/`RENDER_TARGET`/
>   `VERTEX_BUFFER`/`INDEX_BUFFER`/`ENV_CONSTANT`/`CAMERA_CONSTANT`/`OBJECT_CONSTANT`）は「移動先」とされる
>   クラス（`VertexData`/`Light`/`Camera`/`Renderer`/`MeshFilter`等）が実在していても、実際にはそのクラスが
>   構造体を**メンバとして保持**するか、GPU転送用に構造体を**その場で組み立てて使う**だけで、構造体自体は
>   現役。コメントは「移行済み」という誤った印象を与えるだけだったので、本項の内容そのものを見直す必要がある
>   （詳細は #2 節を参照）。
> - ✅ **#3 に着手（Stage 0〜5 完了、一区切り）**：`RenderManager`（当初約1900行）から
>   `DeferredReleaseQueue`/`DescriptorAllocator`/`ConstantBufferRing`/`TextureLoader`/
>   `RenderTargetFactory`/`GraphicsDevice` の6クラスを段階的に切り出し、**1714行まで縮小**。
>   全段階で外部呼び出し元（MeshRenderer/SpriteRenderer/Light/Camera等）は無改修、各段階でDebug x64
>   ビルド通過＋実行確認済み。**G-Buffer所有権・バックバッファ/深度バッファ・`ApplyPendingResizes`
>   （リサイズ機構）は最もリスクが高い部分としてあえて未着手**（詳細は #3 節参照）。

---

## 1. 命名規則の統一（最優先・影響範囲が広い） ✅ メンバ変数は対応済み

メンバ変数の接頭辞が **ファイル・クラスどころか同じクラス内でも混在**していた。

| パターン | 例 | 場所 |
|---|---|---|
| `m_` + PascalCase | `m_Components`, `m_WindowHandle`, `m_Name` | GameObject, RenderManager, Material |
| `_` + PascalCase | `_IsActive`, `_Name`, `_Objects`, `_SelectedObject`, `_CurrentTargetType` | GameObject, ObjectManager, RenderManager |
| `_` + camelCase | `_instanceID`, `_isActive` | Object, ImGuiWindowController |

**対応**: 統一先を **`m_PascalCase`** に決定（既存多数派＝差分最小、かつ `_` + 大文字始まりは C++ 標準が実装に予約する命名なので是正の意味も大きい）。
`_`始まりのメンバ 41 種を `m_PascalCase` に機械置換した（**35 ファイル・261 箇所**）。`_instanceID`→`m_InstanceID`、`_isActive`→`m_IsActive`、`_creators`→`m_Creators`、`_pVertexData`→`m_VertexData` のように camelCase/ハンガリアンは PascalCase 化。Debug x64 ビルド通過を確認（新規の警告・エラーなし）。

**残り（このパスでは未対応・別パス）**:
- `Transform` の **public 無接頭辞メンバ** `Position` / `Rotation` / `Scale` / `Quaternion` — public データメンバに `m_` を付けるかは別の判断（private 化＋アクセサ化とセットで検討）。
- **関数引数・ローカル変数** が `Width` / `Height` / `Resource` のように PascalCase（C++ では珍しい）。規則を別途決めて統一したい。
- **static メンバの接頭辞** — `s_ActiveCamera`（Camera）等の `s_` を残すか `m_` に寄せるか。今回は `s_` を温存。

---

## 2. C スタイルの ALL_CAPS 構造体を正式なクラスへ移行 ✅ 死んでいた2件は削除済み

`RenderManager.h` の `namespace Types` に、コメントで「○○クラスに移動」と書かれたまま放置されている構造体群があった（[RenderManager.h:12-100](Code/Render/RenderManager.h)）。

```cpp
struct VERTEX        // → VertexData クラスに移動（コメント済み）
struct MATERIAL      // → Material クラスに移動
struct ENV_CONSTANT  // → Light クラスに移動
struct CAMERA_CONSTANT, OBJECT_CONSTANT, TEXTURE,
       RENDER_TARGET, VERTEX_BUFFER, INDEX_BUFFER
```

- ALL_CAPS は本来マクロ用の命名で、型名としては不適切。
- さらに `using Types::VERTEX;` 等を **namespace スコープで丸ごと再公開**しており（[RenderManager.h:106-116](Code/Render/RenderManager.h)）、`Render` 名前空間が汚染されている。

**調査（2026-07-04）**: 「コメントで書かれた移動先クラスが実際にその役割を果たしていて構造体が不要になっていないか」を1つずつ確認した。結果、コメントの「移動」は**ほぼ実現していなかった**：

- ✅ **削除済み（本当に死んでいた）**
  - `SUBSET_CONSTANT`（「Rendererクラスに移動」と記載）— `Renderer` 側にも移行された形跡がなく、宣言・`using` 以外に使用箇所ゼロ。
  - `CONSTANT_BUFFER`（移行コメント無し）— 宣言・デストラクタ定義・`using` 以外に使用箇所ゼロ（デストラクタも誰からも呼ばれていなかった）。
  - → 両方を `RenderManager.h`/`.cpp` から削除。Debug x64 ビルド通過を確認。
- ⚠️ **現役（コメントは誤り・移行未完了）**
  - `VERTEX` — `VertexData` クラスは存在するが、`std::vector<VERTEX>` を**保持するだけ**で置き換えてはいない。`OBJLoader`/`FBXLoader`/`MeshFilter`/`SpriteRenderer` 等で構造体自体が広く使われている。
  - `ENV_CONSTANT` / `CAMERA_CONSTANT` / `OBJECT_CONSTANT` — `Light`/`Camera`/`Renderer` クラスは存在するが、各クラスの `Draw()` 内で**その場で構造体を組み立てて GPU に送る**用途に使っており、構造体は GPU 定数バッファのレイアウトとして現役。
  - `TEXTURE` — 「`Texture` クラスに移動」とコメントされているが、**`Texture` クラス自体がコードベースに存在しない**（コメントが完全に誤り）。`Material`/`RenderManager`/`OBJLoader`/`FBXLoader` 等で広範に使用。
  - `VERTEX_BUFFER` / `INDEX_BUFFER` — 「`MeshFilter` クラスに移動」とコメントされているが、`MeshFilter` は `std::unique_ptr<VERTEX_BUFFER>` を**保持するだけ**。`RenderManager` の `CreateVertexBuffer`/`SetVertexBuffer` 等でも現役。
  - `RENDER_TARGET` — 移行コメント無し。G-Buffer 各種（`m_ColorBuffer` 等）の実体として現役。

**やること（残り）**: 上記「現役」組はまだ削除できないが、コメントの「移動」は誤解を招くので、いずれ本気で移行するか（#3 の `RenderTargetManager`/`TextureLoader` 等への分割と合わせて）、コメント自体を実態に合わせて書き直すか判断する。GPU リソース保持系（`RENDER_TARGET` / `VERTEX_BUFFER` / `TEXTURE` など）は `Render` 配下のリソースクラスに整理していく方向は変わらず。

---

## 3. 神クラス RenderManager の分割 🔄 Stage 0〜5 完了・一区切り

`RenderManager.cpp` が **1903 行**で全描画処理を抱えている（[RenderManager.cpp](Code/Render/RenderManager.cpp)）。1クラスに以下が混在：

- デバイス／スワップチェーン／フェンス初期化（低レベル）
- ディスクリプタヒープのアロケータ（SRV/RTV プール管理）
- 定数バッファのリングバッファ管理
- レンダーターゲット生成・リサイズ（pending 機構）
- テクスチャ読み込み（`LoadTexture`）
- PSO 生成・登録・遅延解放（`m_PendingReleasePSOs`）
- **テクスチャの遅延解放**（`m_PendingReleaseTextures` / `DeferReleaseTexture`、commit `e88c42a` で追加）
- **Deferred ライティング解決 / ポストプロセス / Forward パス**（高レベルのレンダーグラフ）

低レベル（GPU リソース確保）と高レベル（パスの組み立て）が同居しているのが一番の問題。
さらに「フェンス通過まで保持してから解放する」遅延解放が **PSO 用・テクスチャ用と別々に2系統**生えてきており
（今後バッファ等でも増えそう）、`DeferredReleaseQueue` のような**汎用の遅延解放キュー1本**に寄せる余地がある。

**分割方針**: 外部40箇所超の呼び出し元（`RenderManager::GetInstance()->X()`）への影響を避けるため、
`RenderManager` を薄いファサードとして残し、既存 public API のシグネチャは変えずに内部実装だけ新クラスへ
委譲する形で、段階的に・都度ビルド確認しながら進めた（自動テストが無くDX12ネイティブウィンドウの
描画結果は目視でしか確認できないため、各段階でユーザーが実行して確認）。

**進捗（2026-07-06 時点、Stage 0〜5 完了）**:
- ✅ `DeferredReleaseQueue<T>`（[DeferredReleaseQueue.h](Code/Render/DeferredReleaseQueue.h)）… PSO/テクスチャの遅延解放を1本化
- ✅ `DescriptorAllocator`（[DescriptorAllocator.h](Code/Render/DescriptorAllocator.h)/[.cpp](Code/Render/DescriptorAllocator.cpp)）… SRV/RTVヒープ生成＋フリーリスト（SRV用・RTV用で2インスタンス）
- ✅ `ConstantBufferRing`（[ConstantBufferRing.h](Code/Render/ConstantBufferRing.h)/[.cpp](Code/Render/ConstantBufferRing.cpp)）… 2フレーム分の定数バッファリング
- ✅ `TextureLoader`（[TextureLoader.h](Code/Render/TextureLoader.h)/[.cpp](Code/Render/TextureLoader.cpp)）… `LoadTexture`のDDS読込・SRV作成ロジック（`TEXTURE`構造体自体は外部8ファイルがフル修飾参照しているため`Types`名前空間に残置）
- ✅ `RenderTargetFactory`（[RenderTargetFactory.h](Code/Render/RenderTargetFactory.h)/[.cpp](Code/Render/RenderTargetFactory.cpp)）… `CreateRenderTarget(width,height,format,...)`の生成ロジック（`RENDER_TARGET`構造体自体は外部から型名で参照されていないと確認済みだが、所有権はまだRenderManager側）
- ✅ `GraphicsDevice`（[GraphicsDevice.h](Code/Render/GraphicsDevice.h)/[.cpp](Code/Render/GraphicsDevice.cpp)）… Factory/Adapter/Device/CommandQueue/Fence/CommandAllocator×2/CommandList/SwapChainの生成。`RenderManager`側の`m_Device`等は型を変えずInit直後にコピーするだけにして、他の全既存コードを無改修に保った
- → `RenderManager.cpp` は **1903行→1714行** に縮小。全段階でDebug x64ビルド通過・実行確認済み。

**あえて未着手（最もリスクが高い部分）**:
- **G-Buffer（Color/Normal/Position/Material/Emission/LightedColor/PostProcess）・バックバッファ・深度バッファの所有権**
- **`ApplyPendingResizes()`**（スワップチェーンリサイズ/ゲームビュー・シーンビューリサイズのpending機構。コマンドリストのClose/Reset・`WaitGPU`・コマンドアロケータリセットと密結合していて、単純な「生成ロジックの切り出し」パターンが通用しない）
- **`DrawBegin`/`DrawEnd`/`ResolveDeferredLighting`/`BeginForwardPass`/`ApplyPostProcess`**（高レベルのレンダーグラフ）

これらは描画ループの中核で、壊れた場合に画面が黒くなる・G-Bufferが化ける等、目視確認でしか検知できない
バグを埋め込むリスクが高い。ユーザーと相談の上、**このパスでは着手を見送り**、`RenderTargetManager`
（本格版）と`RenderPipeline`/`RenderGraph`は別パスとして仕切り直す判断とした。

**残タスク（次回以降）**:
- `RenderTargetManager` … G-Buffer/バックバッファ/深度バッファの所有権、`ApplyPendingResizes`の移行
- `RenderPipeline` / `RenderGraph` … Deferred/Forward/PostProcess のパス制御

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

## 5. 既知のバグ・デッドコードの除去 ✅ ほぼ対応済み

調査の結果、「バグ」の一部は**半実装サブシステムの入口**だった。Debug x64 ビルド通過を確認済み。

- ✅ **ハンドル/ID 検証システムを完成**（当初「`Object::operator==` が壊れている」と記載していた件）。
  - 実態: `_instanceID` を**どこも代入しておらず**（`CreateID` 未使用）、`GetID()`/`Object::IsValid()`/`operator==` が未初期化値を触る半実装だった。さらに `ObjectManager::IsValid` は条件が**反転**（世代一致で `false`）。
  - 対応: `ObjectManager::AddObject` で `SetID(CreateID(index, generation))` を代入（[ObjectManager.cpp](Code/Manager/ObjectManager.cpp)）。`IsValid` の反転を修正。`operator==` を `GetID()` 比較に。`SetID` は friend で `ObjectManager` のみに限定（[Object.h](Code/Object.h)）。→ 世代ベースの dangling 検出が実際に機能するように。
- ✅ **return 後の到達不能コード**: `GetSRVDescriptorCPUHandle()` の `m_SRVDescriptorPool.pop_front();` を削除（[RenderManager.h](Code/Render/RenderManager.h)）。
- ⏸ **コンポーネント Start ライフサイクル（半実装・後回し）**: `GameObject::ExecUpdate` の `//Start();` がコメントアウト、`GameObject::Update()` も空（[GameObject.cpp:14](Code/GameObject/GameObject.cpp)）。Component の `Start()`/`Update()` ディスパッチが未配線。これはオブジェクト管理／ライフサイクルの設計とセットなので、TODO「シーン機能」等の実装時にまとめて対応する。
- ⏭ **`~RenderManager` の ReportLiveDeviceObjects ブロック**: D3D12 リーク検出の有用なデバッグツール（コメントアウト中）。**そのまま残す**判断（必要時に `#if _DEBUG` で有効化）。

---

## 6. Material の二重管理の解消

`Material` がプロパティ／テクスチャを**複数の表現で二重〜三重に保持**しており、手動同期が必要になっている（[Material.h:40-57](Code/Render/Material.h)）。

- **スカラー系**: 動的バッファ `m_PropertyBuffer` ＋ 互換用個別メンバ `m_BaseColor` 等を両方持ち、`UpdateBufferFromLegacyMembers` / `UpdateLegacyMembersFromBuffer` で手動同期。
- **テクスチャ系**: 旧 API の `m_TextureBaseColor`（単体）と、新 API の `m_Textures`（名前→テクスチャの map, register space1）が並存。さらに `m_TextureBlock`（RAII）でディスクリプタブロックを遅延確保。
- source of truth が複数あり、同期忘れ・解放タイミングのバグ温床（実際 `e88c42a` でテクスチャ差し替え時のクラッシュを遅延解放で塞いだばかり）。

**やること**: スカラーはプロパティバッファ一本に寄せ、個別 getter/setter はバッファ参照に。テクスチャも `m_Textures` の map に一本化し、`m_TextureBaseColor` は名前付きスロット（例 `"BaseColor"`）へ吸収。`MaterialConstant`（POD）と `Material`（ロジック）の役割分担も整理。

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

## 9. 名前空間の不統一 ✅ Render は対応済み

- 自作コードは `EngineCore::General` / `EngineCore::Manager` / `EngineCore::Utility` に整理されているのに、描画系だけ `Render`（`EngineCore` の外）になっていた。

**対応**: `Render` を **`EngineCore::Render`** に移動（定義5箇所 + `using namespace Render*` 7箇所を修正）。EngineCore 外（GUIController・グローバルの VertexData/OBJLoader 等）からの参照は `EngineCore::Render::` に**完全修飾**して統一（**35ファイル・136箇所**、Debug x64 ビルド通過）。否定先読みで二重修飾を回避。

**残り（別パス）**:
- `GUIController::Window` / `GUIController::Gui` を `EngineCore::GUIController::*` に寄せるか。
- グローバル名前空間に置かれたクラス（`VertexData`、`MODEL`/OBJLoader 系、`Model`、`WindowManager` 等）を `EngineCore` 配下へ。
- ヘッダ内 `using namespace`（`ObjectManager.h` の `General`、`FBXLoader.h` の `EngineCore::Render::Types` 等）の除去は #10 と合わせて。

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

0. ~~**#4 文字コード**~~ … ✅ 対応済み
1. ~~**#5 バグ/デッドコード**~~ … ✅ ほぼ対応済み
2. ~~**#1 命名規則**~~ / ~~**#9 名前空間**~~ … ✅ メンバ変数・Render名前空間は対応済み
3. **#2 構造体のクラス化** … 🔄 死んでいた2件は削除済み。残りは #3 の続きと連動
4. **#6 Material 二重管理** … 局所的で効果大（未着手）
5. **#3 RenderManager 分割** … 🔄 Stage 0〜5 完了・一区切り。G-Buffer所有権/`ApplyPendingResizes`/
   `RenderPipeline`が残タスク（最大の山の本体はここから）
6. **#7/#8 所有権・シングルトン** と **#11 シーン外部化** … 設計寄り。TODO の他項目（シーン機能・スクリプト）と合わせて進める

---

## TODO との関連（新規項目の取り込み）

TODO.md に追加された高優先項目は、本書のリファクタリング項目と次のように噛み合う。

- **assertを避ける（クラッシュさせない／エディタ動作中にエラー表示して直す）**
  → #5・#7 と直結。今 `assert(...)` で落としている箇所（例 [ImGuiController.cpp:19,50,67](Code/GUIController/ImGuiController.cpp)）を、
  戻り値／エラー状態 + ログ出力（TODO「ログ出力ウィンドウ」）に置き換える設計が要る。例外 or `Result` 型の方針を先に決めると後が楽。
- **シェーダーテクスチャをエディタから設定**
  → #6 Material のテクスチャ一本化（`m_Textures` map 集約）と同じ土俵。先に Material を整理しておくと実装が乗せやすい。
- **PositionBuffer を深度バッファに切り替える**
  → #3 RenderManager の G-Buffer 周り（`m_PositionBuffer` 等）に直接手を入れる話。分割（`RenderTargetManager` / `RenderPipeline`）と同時に進めると衝突が減る。
