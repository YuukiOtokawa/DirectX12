# プロジェクトの構成

確認日: 2026-10-26。保存済みソースとプロジェクト設定の静的確認に基づく。実行検証は未実施。

## 概要

Windows向けのDirectX 12ゲームエンジン。GameObjectとComponentでオブジェクトを構成し、ImGuiによる編集画面を持つ。
`DirectX12.vcxproj` はC++20、PlatformToolset `v145` を指定している。将来の作業では設定を再確認する。

## 調査の入口

以下のパスはリポジトリルートからの相対パス。

| 対象 | 入口 |
| --- | --- |
| エントリーポイント | `Code/Manager/Main.cpp` |
| 初期化、更新、描画の進行 | `Code/Manager/GameManager.cpp` |
| オブジェクト管理 | `Code/Manager/ObjectManager.h` / `.cpp` |
| GameObjectとコンポーネント所有 | `Code/GameObject/GameObject.h` / `.cpp` |
| コンポーネントの基本API | `Code/Component/Component.h` / `.cpp` |
| コンポーネント登録 | `Code/Manager/ComponentFactory.h` / `.cpp` |
| 描画 | `Code/Render/RenderManager.h` / `.cpp`、同フォルダ内の各描画クラス |
| シェーダー | `Code/Shader/` |
| エディター画面 | `Code/GUIController/` |
| 初期ワールド | `Code/Utility/WorldInitializer.cpp` |
| アセット・シェーダーの参照先 | `Code/Utility/ResourcePath.h` |

## 静的に確認できた仕様

- `GameObject` は `std::unique_ptr<Component>` の配列でコンポーネントを所有する。`AddComponent<T>()` は生成後にownerを設定して配列に追加する。
- `Component` は `Start`、`Update`、`Draw`、`Inspector` などのAPIを持つ。宣言の存在だけで実行順序を推測せず、呼び出し元を確認する。
- `GameManager::Initialize()` はEditorCamera、初期ワールド、PostProcessManagerを準備する。
- `RenderManager.cpp` に `D3DCompileFromFile` による実行時シェーダーコンパイル処理がある。
- `ResourcePath.h` のマクロはDebugで `Assets/` と `Code/Shader/`、それ以外で `Resource/Asset/` と `Resource/Shader/` を参照する。個々の読み込み箇所がこのマクロを使うかも確認する。
- `DirectX12.slnx` に登録されているのは `DirectX12.vcxproj`、`ImGui/ImGui.vcxproj`、`yaml-cpp/yaml-cpp.vcxproj`。`Core/` と `Editor/` にもプロジェクトファイルがあるが、これだけで分割運用が完了したとは判断しない。

## 調査時の注意

`README.md` のエントリーポイントの配置説明と実際の配置には差がある。ファイルの所在は現在のツリーで確認する。
`TODO.md` や `Refactoring.md` の完了印を、そのまま実装・動作確認の証拠にしない。
GPUリソースに関わる変更案では、所有者、解放時期、GPU完了待ち、ディスクリプタとリソース状態の整合性を確認する。

