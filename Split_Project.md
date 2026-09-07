# プロジェクト分割・複数exe運用 計画

> TODO.md「プロジェクト分割・複数exe運用」の設計メモ。
> 実装しながら参照する用。
> 作成: 2026-09-07 / 対象ブランチ: `claude/project-split-planning-jf2jnn`
>
> **状態: 計画のみ。コード変更は未着手。**

---

## 0. 決定事項

| 項目 | 決定 | 理由 |
|---|---|---|
| 分割の粒度 | **EngineCore / EngineEditor の2モジュール** | `Render` ⇄ `Component` に循環依存があり、Core をこれ以上割れない（→ 1-4） |
| `Component::Inspector()` の扱い | **Editor 側の Drawer レジストリへ分離**（Unity の `CustomEditor` 方式） | ImGui だけでなくファイルダイアログ・PSO再生成といった「明らかにエディタの処理」ごと Core から追い出せる |
| ライブラリ形態 | **まず静的lib → スクリプトDLL着手時に EngineCore を DLL 化** | export マクロと設計分離を同時にやらない。Phase 3 で設計を、Phase 6 でリンクを検証する |
| exe 構成 | `AsteroidEditor.exe` / `AsteroidPlayer.exe` / ツールexe 2本 | → 5章 |
| スクリプトDLL・ホットリロード | **今回のスコープ外**。ただし後で困らない制約だけ先に守る | → 4-3, 4-4, 7章 |
| ディレクトリ再配置 | **分割が動いてから、純粋な rename コミットとして実施**（Phase 5） | ファイル移動とビルド構成変更を同じコミットに混ぜると事故る |

---

## 1. 現状把握

### 1-1. 現在のビルド構成

```
DirectX12.slnx
├── DirectX12.vcxproj   … Application(exe) / 自作 .cpp 40本 全部入り
├── ImGui/ImGui.vcxproj … StaticLibrary
└── yaml-cpp/yaml-cpp.vcxproj … StaticLibrary
```

外部依存は他に **assimp**（`assimp-vc145-mtd.dll` をリポジトリ直下に配置、21MB）と
**DirectXTex**（`Code/DirectXTex/` にビルド済み .lib）。

`Core/Core.vcxproj` と `Editor/Editor.vcxproj` は**空の雛形として既に存在する**が、
ソースが1本も登録されておらず `.slnx` にも入っていない。今回はこれを土台にする
（ただし `ConfigurationType` が `Application` になっているので `StaticLibrary` に直す必要あり）。

自作 .cpp の内訳:

| ディレクトリ | .cpp 本数 |
|---|---|
| `Code/Render/` | 10 |
| `Code/Component/` | 10 |
| `Code/GUIController/` | 9 |
| `Code/Manager/` | 5 |
| `Code/Utility/` | 5 |
| `Code/GameObject/` | 1 |
| `Code/Object.cpp` | 1 |
| **計** | **40** |

> ルート直下の `Model.cpp` / `TestOBJClass.cpp` は `.vcxproj` に登録されておらず**ビルド対象外**（死んだファイル）。`OldShaders/` も同様。分割前に処遇を決める（→ Phase 0）。

### 1-2. 依存関係の実測

全 `#include` を集計してディレクトリ間の依存を出した結果（出現回数。同一ディレクトリ内と
`assimp` / `DirectXTex` は除外。ImGui 列は本物の `imgui*.h` のみで、`ImGuiController.h` は
`GUIController` 列に数えている）:

| from → to | Render | Component | GameObject | Manager | GUIController | Utility | ImGui |
|---|---|---|---|---|---|---|---|
| **Render** | — | 2 | 0 | 13 | **0** | 7 | **0** |
| **Component** | 11 | — | 6 | 2 | **0** | 11 | **9** |
| **GameObject** | 0 | 1 | — | 1 | **0** | 0 | **1** |
| **Manager** | 2 | 6 | 2 | — | **4** | 2 | **1** |
| **GUIController** | 4 | 0 | 2 | 3 | — | 0 | 8 |
| **Utility** | 5 | 5 | 1 | 1 | **0** | — | 0 |

（`Code/Object.h` は `Component.h` / `GameObject.h` から参照されるだけなので表からは省略）

**朗報**: `Render` / `Component` / `GameObject` / `Utility` から `GUIController` への依存は **全部ゼロ**。
GUI 側だけが一方的に Core を参照している。想像していたより境界は綺麗に引ける。

```
$ grep -rn "GUIController" Code/Render Code/Component Code/GameObject Code/Utility
  → 0件
```

### 1-3. 詰まっているのは実質3箇所だけ

上の表で問題なのは色を付けた列、つまり **Core 側から Editor 側への逆流**。3種類しかない。

| # | 問題 | 該当箇所 | 深刻度 |
|---|---|---|---|
| **A** | `Component::Inspector()` が ImGui を直呼び | `Component/` 内 9ファイル + `GameObject.cpp` | **最大**。これが解ければ8割終わり |
| **B** | `GameManager` が `ImGuiController` / `WindowManager` を**メンバとして所有** | `Manager/GameManager.h:5,6,23,25`, `.cpp`, `Manager/Main.cpp:7` | 大。ただし解体先は明確 |
| **C** | `FilePicker`（Win32 ファイルダイアログ）がコンポーネント内から呼ばれている | `MeshFilter.cpp:16`, `MeshRenderer.cpp:141,181`, `PostProcessComponent.cpp:92`, `MaterialPropertyInspector.h:99` | 中。A を解けば自動的に付いてくる |

おまけで、`RenderManager.h:177-178` に

```cpp
std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_ImGuiCPUDescHandles;
std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_ImGuiGPUDescHandles;
```

が**宣言だけあって使用箇所ゼロ**。消せば Render から ImGui の名前が完全に消える。

### 1-4. `Render` ⇄ `Component` の循環依存（Core をこれ以上割れない理由）

```
Render/RenderSystem.cpp  ──include──►  Component/Polygon/MeshRenderer.h
                                              │
Component/Polygon/MeshRenderer.h ──include──►  Render/Material.h
Component/Polygon/MeshRenderer.cpp ─include─►  Render/RenderManager.h
```

`RenderSystem::Render()` が `dynamic_cast<MeshRenderer*>` して
`GetMaterial().GetRenderPassType()` で Deferred / Forward に振り分けている
（`RenderSystem.cpp:24-34`）。**レンダラがゲームプレイ型を知っている**状態。

このため「Render」と「Gameplay」を別の .lib にすることは今はできない。
まとめて `EngineCore` として括り出す。

将来割りたくなったら、`Renderer` 側が

```cpp
struct DrawItem { PassType pass; DrawOrder order; ... };
renderSystem->Submit(DrawItem{...});
```

のように**描画要求を投げる**形にすれば循環は切れる。今回はやらない（TODO の「G-Buffer整理」や
「GPU使用率軽減」に着手するタイミングで一緒にやるのが自然）。

---

## 2. なぜ分割するのか

### 2-1. コンパイル時間（正直な見積もり）

**分割そのものの効果は控えめ**。MSBuild は既に .cpp 単位でコンパイルしていて、ImGui と yaml-cpp は
別 lib に出ているので、そこはもう分かれている。

分割で実際に効くのはこの3つ:

1. **製品ビルドで Editor の TU をそもそもコンパイルしない** — `GUIController` 10本 + Inspector 実装 + **ImGui 本体（約6万行）** が丸ごと消える。約 25% の TU 削減 + ImGui 分
2. **Editor の .cpp を触っても `EngineCore.lib` が再ビルドされない** — リンクだけになる。DLL 化すればリンクも分離される
3. **スクリプトDLL が入った後**、スクリプト変更でリンクされるのが DLL 1つだけになる（本命）

一方、**今のコンパイル時間の主犯は分割では消えない**:

- `Manager/Main.h` が事実上の PCH（`Windows.h` + `d3d12.h` + `DirectXMath.h` + STL 12種）で、これを **12ファイル**が include している
- `Render/RenderManager.h` が 385 行の全部入りヘッダで、これを Component / Utility / GUIController から広く include している

→ **PCH の正式導入とヘッダ整理を Phase 0 に入れる**（→ 6章）。分割の恩恵とは独立に効く。

### 2-2. ホットリロードでリンクエラーが出る仕組み

TODO の最終目標「スクリプト機能（ホットリロード付き）」を実現すると、こういう構成になる:

```
AsteroidEditor.exe ──► EngineCore ◄── GameScripts.dll （実行中に差し替える）
```

このとき、**今のコードのまま DLL 境界を跨ぐと以下が確実に壊れる**。分割時に潰しておく。

| # | 罠 | 今のコードの該当箇所 |
|---|---|---|
| 1 | **静的初期化がリンカに捨てられる** | `REGISTER_COMPONENT` マクロの `static inline bool registered = [](){...}()`（`Component.h:17-26`）。静的lib では、誰からも参照されていない .obj はリンカが丸ごと捨てるので、そのコンポーネントは**永久に登録されない**。今は全部1つの exe に入っているので偶然動いている |
| 2 | **シングルトンが二重化する** | `ComponentFactory::GetInstance()`（`ComponentFactory.h:23`）、`ObjectManager::GetInstance()`（`ObjectManager.h:37`）、`RenderSystem::GetInstance()`（`RenderSystem.h:12`）が**ヘッダ内 inline 関数のローカル static**。DLL 化してエクスポートしないと、exe と DLL がそれぞれ別のファクトリ・別のオブジェクト一覧を持つ |
| 3 | **vtable が消えたDLLを指す** | リロード時、既存の `Component` インスタンスの vptr が古い DLL のメモリを指したまま → 次の `Update()` で即死。リロード前に**シリアライズ→破棄→ロード→復元**が必須 |
| 4 | **確保と解放のモジュールがずれる** | `std::unique_ptr<Component>` を DLL 側で作って exe 側で捨てる、等。`/MD`（DLL CRT）で統一されていれば実質1ヒープなので事故りにくいが、方針として「確保した側が解放する」を守る |
| 5 | **PDB ロック** | リロード時にデバッガが .pdb を掴んでいてリンク失敗。出力名をユニークにする / PDB をコピーして使う |

**#3 が「プロパティ反射が結局要る」理由**。今回は作らないが、Drawer 分離（→ 4-1）を進めておくと
「コンポーネントの編集可能な値」がどこにあるかが Editor 側に集約されるので、後で反射を入れやすくなる。

### 2-3. デプロイ（TODO「デプロイ機能」への布石）

TODO にこう書いてある:

> ほぼすべてのコードにエディターとデプロイでコンパイルするコードを制御するようにしないといけない・・・？

**プロジェクトを分ければ、これは不要になる。**
`#ifdef EDITOR` を全コードに撒くのではなく、「Editor プロジェクトに入っているファイルは製品ビルドに存在しない」
という形で解決する。これが今回 `#ifdef` 案を採らなかった一番大きい理由。

---

## 3. 分割後の構成

### 3-1. モジュールと依存方向

```
        ┌──────────────────────┐      ┌──────────────────────┐
        │  AsteroidEditor.exe  │      │  AsteroidPlayer.exe  │
        │  (Win32/EditorApp)   │      │  (Win32/GameApp)     │
        └──────────┬───────────┘      └──────────┬───────────┘
                   │                             │
        ┌──────────▼───────────┐                 │
        │  EngineEditor.lib    │                 │
        │  GUIController /     │                 │
        │  Inspector Drawers / │                 │
        │  FilePicker          │                 │
        └──────────┬───────────┘                 │
                   │                             │
                   └───────────┬─────────────────┘
                               ▼
                    ┌──────────────────────┐
                    │   EngineCore.lib     │
                    │   Render / Component │
                    │   GameObject / Manager
                    │   Utility            │
                    └──────────┬───────────┘
                               ▼
        ┌──────────────┬───────────────┬──────────────┐
        │ yaml-cpp.lib │ DirectXTex    │ (assimp.dll) │
        └──────────────┴───────────────┴──────────────┘

        ImGui.lib ──► EngineEditor.lib のみ（Player には一切入らない）

        ─── ツール（別exe・エンジンとは別プロセス）───
        AssetImporter.exe   … assimp を使って FBX/OBJ → 独自バイナリ
        ShaderCompiler.exe  … HLSL → .cso（製品ビルド用）
```

**鉄則: 矢印は必ず下向き。`EngineCore` から `EngineEditor` への参照が1本でも生えたら分割は失敗している。**
これは grep で機械的に検証できる（→ 6章の各フェーズの完了条件）。

### 3-2. ファイル配置表

| ファイル | 行き先 | 備考 |
|---|---|---|
| `Code/Object.cpp/.h` | **EngineCore** | `DrawInspector()` 仮想関数を削除（→ 4-1） |
| `Code/GameObject/GameObject.cpp/.h` | **EngineCore** | `DrawInspector()` の中身を Editor へ |
| `Code/Component/*` (10 .cpp) | **EngineCore** | `Inspector()` の中身を Editor へ |
| `Code/Component/MaterialPropertyInspector.h` | **EngineEditor** | 中身が完全に ImGui。ファイル名の通りエディタのもの |
| `Code/Render/*` (10 .cpp) | **EngineCore** | 未使用の `m_ImGui*DescHandles` を削除 |
| `Code/Manager/ObjectManager.cpp` | **EngineCore** | |
| `Code/Manager/ComponentFactory.cpp` | **EngineCore** | `GetInstance()` を .cpp へ移す（→ 4-3） |
| `Code/Manager/YamlConfigLoader.cpp` | **EngineCore** | |
| `Code/Manager/GameManager.cpp/.h` | **解体** | → 4-2 |
| `Code/Manager/Main.cpp/.h` | **解体** | `Main.h` は Core の共通ヘッダ(PCH)へ、`Main.cpp` は各 exe へ |
| `Code/Utility/VectorClass.h` / `VertexData.h` / `ObjectIDManipulator.h` | **EngineCore** | |
| `Code/Utility/OBJLoader` / `FBXLoader` / `DDSTextureLoader12` | **EngineCore**（暫定） | 将来 AssetImporter.exe へ（→ 5-3） |
| `Code/Utility/WorldInitializer.cpp/.h` | **EngineCore**（暫定） | シーン読み込みが実装されたら消える |
| `Code/Utility/FilePicker.cpp/.h` | **EngineEditor** | Win32 ファイルダイアログはエディタ専用 |
| `Code/Utility/ResourcePath.h` | **EngineCore** | 分岐条件を `_DEBUG` からビルド種別へ（→ 4-6） |
| `Code/GUIController/*` (9 .cpp) | **EngineEditor** | |
| `Code/Shader/*.hlsl` | 共通アセット | |
| `Resource/Resource.rc`, `resource.h` | **AsteroidEditor.exe** | メニューバーはエディタのもの |
| **新規** `Editor/Inspector/*Drawer.cpp` (7本) | **EngineEditor** | → 4-1 |
| **新規** `Editor/Inspector/InspectorRegistry.cpp/.h` | **EngineEditor** | → 4-1 |
| **新規** `Editor/EditorApp.cpp/.h` | **AsteroidEditor.exe** | → 4-2 |
| **新規** `Player/GameApp.cpp/.h` | **AsteroidPlayer.exe** | → 4-2 |
| `Model.cpp/.h`, `TestOBJClass.cpp/.h`, `OldShaders/` | **削除**（要確認） | 現状ビルド対象外の死んだファイル |

集計すると **EngineCore = 29本 / EngineEditor = 10本 + 新規9本前後**。
`AsteroidPlayer.exe` は EngineCore の 29本 + 自前の2本だけで、**ImGui もエディタ UI も一切リンクされない**。

### 3-3. 静的lib か DLL か（決定と根拠）

**Phase 3 では静的lib、Phase 6 で EngineCore を DLL 化する。**

| | 静的lib | DLL |
|---|---|---|
| export マクロ | 不要 | `ENGINE_API` を全公開クラスに付与（重い） |
| 静的初期化の消失（2-2 #1） | **起きる** → `/WHOLEARCHIVE:EngineCore.lib` が要る | 起きない（DLL 内の .obj は全部リンクされる） |
| シングルトン二重化（2-2 #2） | 起きない（1つの exe に1つ） | エクスポートしないと起きる |
| スクリプトDLL からの利用 | 実質不可（exe のエクスポートlib経由になり、ビルド順が循環しがち） | 素直 |
| C4251（`std::vector` メンバの警告） | 出ない | 出る。全モジュールを同一コンパイラ・同一 `/MD` でビルドする前提で抑止する |

2段階にする理由は、**設計の分離（Phase 1-2）とリンクの分離（Phase 6）を同時に検証しないため**。
Phase 3 の時点で「Core から Editor への参照ゼロ」が達成できていれば、DLL 化は
export マクロを機械的に付ける作業に落ちる。逆に設計が分離できていないうちに DLL にすると、
リンクエラーの山を見て「設計の問題」なのか「export の付け忘れ」なのか切り分けられなくなる。

なお `/MD`（`MultiThreadedDLL` / `MultiThreadedDebugDLL`）は既に全プロジェクトで揃っている。
これは DLL 化にあたって好都合（→ ただし assimp の件は 4-7）。

### 3-4. 名前空間

現状の名前空間は分割の意図とだいたい一致している。**今回は変えない**（差分を増やさない）。

| 名前空間 | 行き先 |
|---|---|
| `EngineCore::General` / `::Render` / `::Manager` / `::RenderSystem` / `::Utility` | EngineCore |
| `GUIController::Gui` / `::Window` | EngineEditor |
| `EngineManager`（`GameManager` のみ） | 解体して消滅 |

将来 DLL 化するとき、`EngineCore` 直下に `ENGINE_API` を付けていくことになる。

---

## 4. 分割のために変えなければいけないもの

### 4-1. 【最重要】`Inspector()` を Core から追い出す

#### 現状

```cpp
// Object.h
class Object {
public:
    virtual void DrawInspector() {}   // ← Core の基底クラスに UI が生えている
};

// Component.h
class Component : public Object {
public:
    void DrawInspector() override;    // ImGui::Text(型名) してから
    virtual void Inspector();         // ← 各コンポーネントがここに ImGui を書く
};
```

`Inspector()` の実装は **7つだけ**（+ `Component::DrawInspector` と `GameObject::DrawInspector`）:

| 実装 | 場所 | 移設の難易度 | 必要なアクセサ |
|---|---|---|---|
| `Transform::Inspector` | `Transform.cpp:9` | ★ 3行 | **不要**（`Position`/`Rotation`/`Scale` が public） |
| `Light::Inspector` | `Light.cpp:103` | ★★ | `m_ShadowOrthoSize` / `m_ShadowFov` / `m_ShadowNear` / `m_ShadowFar` のアクセサ**要追加** |
| `Camera::Inspector` | `Camera.cpp:136` | ★★ | `m_Fov` / `m_Near` / `m_Far` / `m_TargetPosition` のアクセサ**要追加** |
| `MeshFilter::Inspector` | `MeshFilter.cpp:14` | ★★★ | 中身が「ファイルダイアログ → FBX/OBJ ロード → SetVertexData」。**丸ごとエディタの処理**。アクセサは揃っている |
| `MeshRenderer::Inspector` | `MeshRenderer.cpp:95` | ★★★★ | 中身が「シェーダ選択 → PSO 再生成 → RenderManager に登録」。**丸ごとエディタの処理**。`GetMaterial()` があるので追加は不要 |
| `PostProcessComponent::Inspector` | `PostProcessComponent.cpp:49` | ★★★★★ | `m_Passes` / `m_NewName[256]` / `m_NewShaderPath[256]` が private。**`m_NewName` は ImGui の `InputText` 用編集バッファ**で、ランタイムのコンポーネントが持っているのがそもそもおかしい |
| `EditorCameraController` | `EditorCameraController.h:16` | — | `Inspector()` は空。ただし `Draw()` が ImGui の入力APIを使っている（→ 後述） |

`PostProcessComponent` の `char m_NewName[256]` は、この分離が単なる「ImGui の隠蔽」ではないことの
いい例。**UI の編集途中の文字列**がランタイムのコンポーネントに住んでいて、製品ビルドにも乗る。
Drawer 側に移せば Drawer のメンバになり、製品ビルドから消える。

#### 変更後

```cpp
// ── EngineCore 側: UI の痕跡を消す ──────────────
// Object.h     : virtual void DrawInspector() を削除
// Component.h  : virtual void Inspector() / void DrawInspector() を削除
// GameObject.h : void DrawInspector() を削除
// 各 Component : Inspector() の実装と #include imgui.h を削除、代わりにアクセサを公開

// ── EngineEditor 側: 型名 → 描画関数のレジストリ ──
namespace EditorUI {
    using ComponentDrawer = std::function<void(EngineCore::General::Component&)>;

    class InspectorRegistry {
    public:
        static InspectorRegistry& Get();
        void Register(const std::string& typeName, ComponentDrawer drawer);
        // 登録が無ければ「(型名) : No custom inspector」とだけ描く
        void Draw(EngineCore::General::Component& component);
    };
}

#define REGISTER_INSPECTOR(Type, Fn) \
    static const bool s_reg_##Type = [] { \
        EditorUI::InspectorRegistry::Get().Register(#Type, Fn); return true; }();
```

```cpp
// Editor/Inspector/TransformDrawer.cpp
#include "InspectorRegistry.h"
#include "Component/Transform/Transform.h"
#include "imgui.h"

static void DrawTransform(EngineCore::General::Component& c) {
    auto& t = static_cast<EngineCore::General::Transform&>(c);
    ImGui::DragFloat3("Position", &t.Position.x, 0.1f);
    ImGui::DragFloat3("Rotation", &t.Rotation.x, 0.1f);
    ImGui::DragFloat3("Scale",    &t.Scale.x,    0.1f);
}
REGISTER_INSPECTOR(Transform, DrawTransform)
```

`InspectorWindowController` は `selectedObject->DrawInspector()` を呼ぶ代わりに、
GameObject 名 / Active / コンポーネント一覧 / Add Component ポップアップを自前で描き、
各コンポーネントについて `InspectorRegistry::Get().Draw(*component)` を呼ぶ。

#### 型名のキーをどうするか

`REGISTER_COMPONENT(Type)` が既に `GetClassName()` として `#Type` を持っているが **private**。
これを `virtual const char* GetTypeName() const` として公開するのが一番素直
（`ComponentFactory` の登録キーとも一致するので、後のシリアライズでもそのまま使える）。
`typeid().name()` は MSVC 依存の装飾名になるので使わない。

#### 検討したが採らなかった案

**案B: `Inspector(IInspectorUI& ui)` に抽象化**
差分は小さいが、`MeshRenderer` / `MeshFilter` / `PostProcessComponent` の Inspector は
「ウィジェット描画」ではなく **ファイルダイアログを開いてモデルをロードし PSO を作り直す処理**なので、
ウィジェットを抽象化しても本体は Core に残る。半分しか解決しない。

**案D: `#ifdef EDITOR`**
2-3 の通り、TODO で懸念していた「全コードに分岐を入れる」状態そのものになる。
さらに Core が Editor版/製品版の2ビルドに分岐して、ビルドマトリクスが倍になる。

#### `EditorCameraController` の扱い

`Inspector()` は空だが、`Draw()` の中で `ImGui::IsMouseDown` / `ImGui::GetIO()` / `ImGui::IsKeyDown` を
使っている（`EditorCameraController.cpp:11-62`）。これは Inspector ではなく**毎フレームの入力処理**なので、
Drawer には移せない。**コンポーネントごと `EngineEditor` に移す**のが正しい
（そもそもエディタカメラは製品ビルドに要らない）。

ただし `EngineEditor` 側でコンポーネントを定義すると `REGISTER_COMPONENT` が
`EngineCore` の `ComponentFactory` に登録することになる。これは**動く**（依存方向は Editor→Core で正しい）が、
「Core のファクトリに Editor のコンポーネントが入る」形になるので、
将来の Input 抽象化まではこの形で妥協する。

### 4-2. `GameManager` の解体

現状の `GameManager` はエディタ専用のループになっている（`GameManager.cpp:99-176`）:

1. GameView 用: シャドウパス（カスケード×ライト数）→ G-Buffer → GameView ターゲット
2. SceneView 用: **同じことをもう一度**、エディタカメラで
3. BackBuffer に ImGui を描画

製品ビルドに要るのは 1 だけで、しかも出力先は GameView ターゲットではなくバックバッファ。

**分解:**

| 新クラス | 場所 | 責務 |
|---|---|---|
| `EngineCore::Engine` | EngineCore | `RenderManager` と `ObjectManager` の所有。`Initialize()` / `Update()`。`FindGameCamera()` / `FindLights()` |
| `EngineCore::Engine::RenderView(Camera*, RenderTargetType)` | EngineCore | 「1つのカメラで1つのターゲットに、シャドウ込みで描く」。現 `Draw()` の 1 と 2 で完全に重複しているブロックをここに畳む |
| `EditorApp` | AsteroidEditor.exe | `ImGuiController` / `WindowManager` の所有。`RenderView(game)` → `RenderView(editor)` → ImGui パス |
| `GameApp` | AsteroidPlayer.exe | `RenderView(gameCamera, BACK_BUFFER)` のみ |

`GameManager::Draw()` の 1 と 2 は**ほぼコピペで重複している**（差はカメラと出力ターゲットだけ）ので、
`RenderView()` に畳むのはリファクタリングとしても素直に得。

**`Main.cpp` も分解する:**

- ウィンドウクラス登録 / メッセージループ → 各 exe が持つ（`EditorApp` 版と `GameApp` 版）
- `EngineCore::GetWindow()` というグローバル関数（`Main.h:55`、実体は `Main.cpp:27`）が
  **Core から exe 側のグローバル変数 `g_Window` を引っ張っている隠れた逆流**。
  `Engine::Initialize(HWND)` で受け取る形に直す
- `Main.h` の巨大 include 群は Core の PCH へ（→ 6章 Phase 0）
- メニューバー（`Resource.rc` / `IDR_MENU1` / `ID_WINDOW_NEWWINDOW`）はエディタのもの

### 4-3. シングルトンをモジュール跨ぎに耐える形にする

Phase 6（DLL 化）で必須。**Phase 3 の時点で先に直しておく**（静的lib のままでも壊れないので安全に先行できる）。

| 対象 | 現状 | 変更 |
|---|---|---|
| `ComponentFactory::GetInstance()` | `ComponentFactory.h:23` でヘッダ内 inline、ローカル static | 宣言だけヘッダに残し、**定義を `ComponentFactory.cpp` へ移す** |
| `ObjectManager::GetInstance()` | `ObjectManager.h:37` でヘッダ内 inline、`new` で生成 | 同上（`ObjectManager.cpp` へ） |
| `RenderSystem::GetInstance()` | `RenderSystem.h:12` でヘッダ内 inline、`new` で生成 | 同上（`RenderSystem.cpp` へ） |
| `RenderManager::GetInstance()` | static メンバを返すだけ。実体は `.cpp` | アクセサ自体が inline なので、DLL 化時に `ENGINE_API` 化 |
| `Camera::s_ActiveCamera` / `Light::s_ActiveLight` | static メンバ。実体は `.cpp` | 同上 |
| `GameManager::m_Instance` | | 解体して消滅 |

ヘッダ内 inline 関数のローカル static は、**エクスポートされていないと exe と DLL でそれぞれ別実体になる**。
`ComponentFactory` が二重化すると「スクリプトDLL で登録したコンポーネントがエディタから見えない」
という、原因の分かりにくいバグになる。

### 4-4. `REGISTER_COMPONENT` の静的初期化が消える問題

`Component.h:17-26`:

```cpp
#define REGISTER_COMPONENT(Type) \
private: \
    static const char* GetClassName() { return #Type; } \
    static inline bool registered = []() { ... }();
```

静的lib にすると、**そのコンポーネントの .obj を誰も参照していなければリンカが .obj ごと捨てる**ため、
`registered` の初期化が走らず `ComponentFactory` に登録されない
（＝ Inspector の「Add Component」に出てこない、シーンから復元できない）。

今は全部1つの exe に入っているので偶然全部リンクされているが、Phase 3 で確実に踏む。

**対策（Phase 3 では 1 を採る）:**

1. **`/WHOLEARCHIVE:EngineCore.lib`** をリンカオプションに追加（`Link > AdditionalOptions`）。1行で済む。サイズは増えるが、コンポーネントだけの lib ではないので許容
2. Phase 6 で DLL 化すれば自動的に解決（DLL 内の .obj は全部リンクされる）

**やってはいけない**: 「動いているから大丈夫」と放置すること。この不具合は
「新しく追加したコンポーネントだけ Add Component に出てこない」という形で、
分割から何週間も経ってから出る。**Phase 3 の完了条件に「Add Component ポップアップに
全コンポーネントが並ぶことを目視確認」を入れる。**

### 4-5. `FilePicker` を Editor へ

`OpenFileDialog()` は `IFileOpenDialog`（Win32 コモンダイアログ）のラッパで、
呼び出し元は 5箇所すべて Inspector 系（`MeshFilter` / `MeshRenderer`×2 / `PostProcessComponent` /
`MaterialPropertyInspector`）。**4-1 の Drawer 移設をやれば呼び出し元ごと Editor 側に移る**ので、
`FilePicker.cpp/.h` を `EngineEditor` に移すだけで完了する。

### 4-6. `ResourcePath.h` の分岐条件

```cpp
#ifdef _DEBUG
#define ASSET_DIR  "Assets\\"        // リポジトリ内を直接参照
#else
#define ASSET_DIR  "Resource\\Asset\\"  // 配布用
#endif
```

**判定軸が間違っている**。「Debug かどうか」ではなく「エディタか製品か」で決まるべき。
Debug ビルドの `AsteroidPlayer.exe` はリポジトリではなく配布レイアウトを見るべきだし、
Release ビルドのエディタはリポジトリを見たい。

→ プロジェクトのプリプロセッサ定義を `ASTEROID_EDITOR` / `ASTEROID_PLAYER` にして、そちらで分岐する。

### 4-7. 既存のビルド設定の不具合（分割前に潰す）

分割とは独立だが、リンク周りを触るタイミングでまとめて直す。

| # | 問題 | 該当 |
|---|---|---|
| 1 | **Release 構成が Debug 版 assimp をリンクしている** | `DirectX12.vcxproj:143` の `assimp-vc145-mtd.lib`。`mtd` は MultiThreaded **Debug**。しかもプロジェクト本体は `/MD` なので、静的デバッグCRT前提の lib と CRT 種別が食い違っている |
| 2 | **ビルド成果物がリポジトリにコミットされている** | `OutDir` が `$(SolutionDir)`（`DirectX12.vcxproj:74,77`）なので出力がルート直下に落ち、`.gitignore` の `x64/` に引っかからない。`DirectX12.exe`(1.1MB) / `.lib` / `.exp` / `assimp-vc145-mtd.dll`(**21MB**) がコミット済み |
| 3 | **相対 include 地獄** | `#include "../../../ImGui/Code/imgui.h"` のような相対パスが多数。**Phase 5 のディレクトリ再配置で全部壊れる**ので、その前に `AdditionalIncludeDirectories` 経由の書き方に統一しておく |
| 4 | `Core.vcxproj` / `Editor.vcxproj` の `ConfigurationType` が `Application` | 空の雛形なので実害はないが、`StaticLibrary` に直して使う |
| 5 | `Win32`(x86) 構成が残っているが中身が未整備 | 使っていないなら削除して構成数を半分にする |

---

## 5. 複数exe運用

### 5-1. `AsteroidEditor.exe` / `AsteroidPlayer.exe`

| | AsteroidEditor.exe | AsteroidPlayer.exe |
|---|---|---|
| リンクするもの | EngineCore + EngineEditor + ImGui + yaml-cpp | EngineCore + yaml-cpp |
| SubSystem | Windows | Windows |
| メニューバー(`Resource.rc`) | あり | なし |
| 描画パス | GameView + SceneView + ImGui の3パス | バックバッファへ1パス |
| シェーダ | 実行時コンパイル（`D3DCompileFromFile`、SM5.1） | 事前コンパイル済み `.cso`（→ 5-3） |
| アセット | `Assets/` を直接 | `Resource/Asset/` の変換済みバイナリ |
| プリプロセッサ | `ASTEROID_EDITOR` | `ASTEROID_PLAYER` |

**`AsteroidPlayer.exe` は Phase 4 で早めに作る**。分割が本当にできているかの検証装置になるから。
「Player がビルドできてウィンドウに何か映る」が通れば、Core→Editor の逆流が無いことの動く証明になる。
grep での検証（依存方向の確認）は書き忘れを見逃すが、リンカは見逃さない。

### 5-2. Play In Editor をどうするか（将来）

TODO の「assertを避ける・クラッシュさせない」と関係する論点。
エディタ内で「再生」したときにゲームコードがクラッシュしてもエディタが道連れにならないようにするには、
**Player を子プロセスとして起動する**方法がある（Unreal の standalone PIE 相当）。

ただしプロセス間でシーンの状態を渡す仕組みが要るので、**今回はやらない**。
ここでは「Editor と Player が別 exe である」ところまでを作っておけば、後から選択できる。

### 5-3. 効率化のためのツールexe

ユーザーの「エンジンの動作の効率化につながりそうな部分は exe やライブラリの分割をしたい」に対する候補。
どちらも**エンジンとは別プロセスの、ビルド時にだけ走るツール**。

#### `AssetImporter.exe`（優先度: 高）

現状、FBX/OBJ の読み込みは **実行時に assimp で行っている**（`FBXLoader.cpp` / `OBJLoader.cpp`、
`MeshFilter::Inspector` から呼ばれる）。これを事前変換にすると:

- **21MB の `assimp-vc145-mtd.dll` が製品ビルドから消える**
- `FBXLoader.cpp`(361行) / `OBJLoader.cpp`(528行) が製品ビルドから消える
- 起動時・モデル読み込み時のパースが「頂点バッファへの memcpy」だけになる（assimp のシーングラフ走査が消える）
- エディタでは今まで通り FBX を直接開ける（インポータをその場で呼ぶ形にする）

出力は `VertexData` の内容をそのまま流し込める素直なバイナリでよい（頂点フォーマットは
`Render/RenderManager.h` の `Types::VERTEX` に固定されている）。

#### `ShaderCompiler.exe`（優先度: 中）

現状、HLSL は **実行時に `D3DCompileFromFile` でコンパイル**している（`RenderManager.cpp:1225`）。
これは**エディタでは機能**（`.hlsl` を保存して PSO 作り直しで反映される = シェーダのホットリロード）なので、
エディタでは残す。製品ビルドだけ `.cso` を読むようにする。

- 製品の起動時間からシェーダコンパイル分（現状 全 PSO ぶん）が消える
- シェーダのコンパイルエラーが**ビルド時に**出るようになる（今は起動時まで分からない）
- `ShaderMetadata::ParseShaderMetadata()`（`.hlsl` のコメントから `[Range]` 等を読む、275行）も
  事前実行して結果を吐けば、製品ビルドから HLSL パーサが消える

#### ライブラリ分割の候補（現時点では不要）

`Utility/VectorClass.h`(154行) や `ObjectIDManipulator.h` はヘッダオンリーで完結していて、
別ライブラリに切り出す実益がない。**やらない。**

---

## 6. 段階的な進め方

各フェーズの完了条件を必ず満たしてから次へ進む。**Phase 1-2 はプロジェクトを1つのままやる**ので、
ビルド構成の変更と設計変更が同時に走らない。

### Phase 0 — 下ごしらえ（分割前にやる掃除）

- [ ] `OutDir` / `IntDir` を `$(SolutionDir)Build\$(Platform)\$(Configuration)\` に変更
- [ ] コミット済みビルド成果物（`DirectX12.exe` / `.lib` / `.exp`）を git から外し `.gitignore` へ
- [ ] `assimp-vc145-mtd.dll` を `ThirdParty/assimp/bin/` に移してビルド後コピーにする（21MB をルートから退かす）
- [ ] **Release 構成の assimp lib 誤リンクを修正**（4-7 #1）
- [ ] 相対 include（`../../../ImGui/...`）を `AdditionalIncludeDirectories` 経由に統一（4-7 #3）
- [ ] PCH を正式導入（`Manager/Main.h` の中身をベースに）
- [ ] 死んだファイルの処遇を決める（`Model.cpp` / `TestOBJClass.cpp` / `OldShaders/`）
- [ ] `RenderManager.h:177-178` の未使用 `m_ImGui*DescHandles` を削除
- [ ] 使っていない `Win32`(x86) 構成を削除

**完了条件**: Debug x64 / Release x64 ともにビルド通過、起動して従来通り動く。
ビルド時間を before/after で計測して記録（Phase 6 の効果測定の基準にする）。

### Phase 1 — Inspector を Core から追い出す ★山場

プロジェクトは1つのまま。

- [ ] `Component` に `virtual const char* GetTypeName() const` を追加（`REGISTER_COMPONENT` の `#Type` を公開）
- [ ] `Camera` / `Light` に不足しているアクセサを追加（4-1 の表）
- [ ] `EditorUI::InspectorRegistry` を作る
- [ ] Drawer を7本作って `Inspector()` の中身を移設（Transform → Light → Camera → MeshFilter → MeshRenderer → PostProcessComponent の順。簡単な順に、各1つずつ移して都度起動確認）
- [ ] `MaterialPropertyInspector.h` を Editor 側へ移動
- [ ] `GameObject::DrawInspector()` の中身を `InspectorWindowController` へ移設
- [ ] `Object::DrawInspector()` / `Component::Inspector()` / `DrawInspector()` を削除
- [ ] `EditorCameraController` を Editor 側の扱いにする
- [ ] `FilePicker` を Editor 側の扱いにする

**完了条件**:
```
grep -rl -i imgui Code/Component Code/GameObject Code/Render Code/Manager Code/Utility
```
が **何も出力しない**こと。かつ、エディタ上で全コンポーネントの Inspector が今まで通り編集できること
（特に MeshRenderer のシェーダ選択・テクスチャ選択・PostProcess のパス追加）。

### Phase 2 — `GameManager` の解体

- [ ] `EngineCore::Engine` を新設、`RenderView(Camera*, RenderTargetType)` を切り出す（重複2ブロックを畳む）
- [ ] `EditorApp` を新設して `ImGuiController` / `WindowManager` を移す
- [ ] `EngineCore::GetWindow()` グローバルを廃止し `Engine::Initialize(HWND)` に
- [ ] `Main.cpp` からエディタ固有部分（メニューバー処理）を `EditorApp` へ

**完了条件**:
```
grep -rn "GUIController" Code/Render Code/Component Code/Manager Code/GameObject Code/Utility
```
が **何も出力しない**こと。従来通り起動して動くこと。

### Phase 3 — プロジェクトを分割する

**ファイルは動かさない**（パスは今のまま）。`.vcxproj` の登録先だけ分ける。

- [ ] `Core/Core.vcxproj` を `StaticLibrary` にして EngineCore の29本を登録
- [ ] `Editor/Editor.vcxproj` を `StaticLibrary` にして EngineEditor を登録
- [ ] `DirectX12.vcxproj` を `AsteroidEditor.exe` にリネームし、上記2つを `ProjectReference` で参照
- [ ] `.slnx` に Core / Editor を追加
- [ ] `/WHOLEARCHIVE:EngineCore.lib` を Editor exe のリンカオプションに追加（4-4）
- [ ] 4-3 のシングルトン `GetInstance()` を .cpp へ移す

**完了条件**: ビルド通過 + 起動して従来通り動く + **Add Component ポップアップに全コンポーネントが並ぶ**
（4-4 の静的初期化が生きている証明）。

### Phase 4 — `AsteroidPlayer.exe` を追加

- [ ] `Player/Main.cpp` + `GameApp` を新設（EngineCore のみリンク）
- [ ] `ResourcePath.h` の分岐を `ASTEROID_EDITOR` / `ASTEROID_PLAYER` に（4-6）

**完了条件**: `AsteroidPlayer.exe` が **ImGui を1バイトもリンクせずに**ビルドでき、
起動してゲームカメラの絵がバックバッファに出ること。**ここが分割の本当のゴール。**
Editor exe / Player exe のバイナリサイズと TU 数を記録する。

### Phase 5 — ディレクトリ再配置

**純粋な rename のみのコミット**にする（中身は1文字も変えない）。

```
Engine/Source/{Core,Render,Component,Asset}/ …
Editor/Source/{Window,Inspector}/ …
Player/Source/ …
Tools/{AssetImporter,ShaderCompiler}/ …
ThirdParty/{ImGui,yaml-cpp,assimp,DirectXTex}/ …
Assets/  Build/(ignored)
```

**完了条件**: `git log --follow` で履歴が追えること（rename として認識されていること）。ビルド通過。

### Phase 6 — `EngineCore` を DLL 化

スクリプト機能に着手する直前にやる。

- [ ] `ENGINE_API` マクロを定義し、公開クラス・公開関数に付与
- [ ] C4251 の抑止（全モジュール同一コンパイラ・同一 `/MD` 前提を明記）
- [ ] `/WHOLEARCHIVE` を外す（DLL では不要）
- [ ] シングルトンのエクスポートを確認（4-3）

**完了条件**: 従来通り動く + Phase 0 で計測したビルド時間との比較。

### Phase 7 — ツールexe

- [ ] `AssetImporter.exe`（assimp を製品ビルドから追い出す）
- [ ] `ShaderCompiler.exe`（`.cso` 事前生成、`ShaderMetadata` も事前生成）

**完了条件**: `AsteroidPlayer.exe` の配布フォルダに `assimp*.dll` と `.hlsl` が1つも無いこと。

---

## 7. 落とし穴チェックリスト

分割作業中に踏みやすいもの。

- [ ] **静的初期化の消失**（4-4）— 「新しく作ったコンポーネントだけ Add Component に出ない」で気付く。Phase 3 で必ず目視確認
- [ ] **シングルトンの二重化**（4-3）— DLL 化してから出る。ヘッダ内 inline のローカル static が犯人
- [ ] **`Core.vcxproj` の `ConfigurationType`** が `Application` のまま
- [ ] **相対 include** が Phase 5 の移動で一斉に壊れる → Phase 0 で先に潰す
- [ ] **`/MD` の統一** — 全プロジェクトが `MultiThreadedDLL` / `MultiThreadedDebugDLL` であることを毎回確認。ここがずれると STL を跨いだ瞬間に壊れる（assimp の件も同根）
- [ ] **ImGui の SRV ヒープ共有** — `ImGui_ImplDX12_Init` は `RenderManager` の SRV ディスクリプタヒープを共有している。SceneView は `sceneViewTarget->SRVHandle.ptr` を `ImTextureID` として渡す（`SceneViewWindowController.cpp:60`）。Editor→Core の正しい向きだが、Core 側の SRV ヒープ API が Editor から見えることを確認
- [ ] **`Render` ⇄ `Component` の循環**（1-4）— Core を割りたくなったときに必ずここで止まる。今回は割らない
- [ ] **`Resource.rc` / `resource.h`** — Player 側に持っていかない
- [ ] **`DirectXTex_Debug.lib` / `_Release.lib`** — 構成ごとの切り替えが Core 側に移ることを確認

---

## 8. 未決事項

| # | 論点 | 現時点の考え |
|---|---|---|
| 1 | `Model.cpp` / `TestOBJClass.cpp` / `OldShaders/` を消してよいか | ビルド対象外なので実害はないが、参考として残している可能性がある。**要確認** |
| 2 | プロパティ反射（シリアライズ基盤）をいつ入れるか | Drawer 分離だけなら不要。ただし TODO「シーン機能の本格実装（保存・読み込み）」と「スクリプトのホットリロード時の状態保持」の**両方が同じものを要求する**ので、シーン機能に着手するタイミングで一緒に設計するのが得。既存の `ShaderMetadata` + `DrawMaterialProperties` がまさに同じ発想の先例になっている |
| 3 | `EditorCameraController` を Editor 側に置くと Core のファクトリに Editor のコンポーネントが登録される件（4-1 末尾） | 動くが気持ち悪い。Input を Core 側に抽象化すれば Core に戻せる。TODO「デバッグ機能の拡充」あたりで入力周りを触るときに再検討 |
| 4 | Play In Editor を別プロセスにするか（5-2） | 分割が済んでいれば後から選べる。今は決めない |
| 5 | ビルド時間の目標値 | Phase 0 で before を計測してから設定する |

---

## 付録: 現状の主要な結合点（行番号つき）

分割作業中に参照する用。

| 内容 | 場所 |
|---|---|
| `Object` の UI 仮想関数 | `Code/Object.h:15` |
| `Component` の Inspector 仮想関数 | `Code/Component/Component.h:44-45` |
| `REGISTER_COMPONENT` マクロ | `Code/Component/Component.h:17-26` |
| `GameObject` の Inspector 実装 | `Code/GameObject/GameObject.cpp:20-65` |
| `GameManager` が GUI を所有 | `Code/Manager/GameManager.h:5,6,23,25` |
| `GameManager::Draw()` の重複2ブロック | `Code/Manager/GameManager.cpp:104-162` |
| `EngineCore::GetWindow()` グローバル | `Code/Manager/Main.h:55` / `Main.cpp:26-31` |
| `Render` ⇄ `Component` の循環 | `Code/Render/RenderSystem.cpp:3,24-28` |
| 未使用の ImGui ハンドル | `Code/Render/RenderManager.h:177-178` |
| 実行時シェーダコンパイル | `Code/Render/RenderManager.cpp:1225` |
| Release の assimp 誤リンク | `DirectX12.vcxproj:143` |
| `OutDir` がルート直下 | `DirectX12.vcxproj:74,77` |
