# DirectX12 ゲームエンジン (Asteroid Engine)

DirectX 12 を使用した自作ゲームエンジンです。Unity ライクなコンポーネント指向のアーキテクチャと、ImGui ベースのエディター UI を備えています。

## 主な機能

### レンダリング
- **DirectX 12** によるレンダリングパイプライン
- **ディファードレンダリング**(Geometry パス + Lighting パス)
- **シャドウマッピング**(実装中)
- **ポストプロセス**(グレースケール、色反転、カラー調整など)
- カスタムマテリアル / Unlit シェーダー対応
- シェーダーは実行時コンパイル(SM 5.1)— `.hlsl` を編集すれば C++ の再ビルド不要

### エディター(ImGui)
- **Hierarchy** — シーン内のオブジェクト一覧
- **Inspector** — コンポーネントのプロパティ編集
- **Scene View / Game View** — エディターカメラとゲームカメラの分離表示
- **Graphics Debug** — G-Buffer などのレンダーターゲット確認

### コンポーネントシステム
- `GameObject` + `Component` ベースの設計
- Transform / Camera / Light / MeshFilter / MeshRenderer / SpriteRenderer / PostProcess など

### アセット読み込み
- **assimp** によるモデル読み込み(FBX / OBJ)
- **DirectXTex / DDSTextureLoader** によるテクスチャ読み込み(DDS 対応)
- **yaml-cpp** による設定ファイル読み込み

## 動作環境

- Windows 10 / 11(x64)
- DirectX 12 対応 GPU
- Visual Studio(MSVC / MSBuild)

## ビルド方法

`DirectX12.slnx` を Visual Studio で開いてビルドするか、Developer Command Prompt などからコマンドラインでビルドします。

```powershell
msbuild DirectX12.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

ビルド後、リポジトリルートに `DirectX12.exe` が生成されます。

> **Note:** HLSL シェーダーはビルド時ではなく実行時に `D3DCompileFromFile` でコンパイルされるため、シェーダーのエラーはアプリ起動時(パイプライン作成時)に検出されます。

## ディレクトリ構成

```
DirectX12/
├── Main.cpp / GameManager.cpp   # エントリーポイント・エンジン全体の管理
├── Code/
│   ├── Render/          # レンダリング (RenderManager, RenderSystem, RenderTargetManager など)
│   ├── Shader/          # HLSL シェーダー (Geometry, Deferred, Shadow, PostProcess など)
│   ├── Component/       # コンポーネント (Transform, Camera, Light, MeshRenderer など)
│   ├── GameObject/      # GameObject
│   ├── Manager/         # ObjectManager, ComponentFactory
│   ├── GUIController/   # ImGui エディターウィンドウ
│   ├── Utility/         # FBX/OBJ ローダー、ベクトル演算など
│   ├── assimp/          # assimp (モデル読み込みライブラリ)
│   └── DirectXTex/      # DirectXTex (テクスチャライブラリ)
├── Assets/              # モデル・テクスチャなどのアセット
├── ImGui/               # Dear ImGui
└── yaml-cpp/            # yaml-cpp
```

## 使用ライブラリ

| ライブラリ | 用途 |
|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) | エディター UI |
| [assimp](https://github.com/assimp/assimp) | 3D モデル読み込み |
| [DirectXTex](https://github.com/microsoft/DirectXTex) | テクスチャ処理 |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | YAML 設定ファイル |

## 今後の予定

- ギズモの実装(ImGuizmo)
- シーン機能の本格実装(保存・読み込み、ゲーム内切り替え)
- シェーダーテクスチャのエディター設定対応
- デバッグ機能の拡充(フレームレート表示、処理時間計測、ログウィンドウ)
- ドラッグ&ドロップ操作、ファイルエクスプローラーの追加
- デプロイ機能
- スクリプト機能(ホットリロード付き)
