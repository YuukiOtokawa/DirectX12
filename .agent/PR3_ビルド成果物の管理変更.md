# PR #3: ビルド成果物を git 管理から外し、出力先を Build/ に変更

- PR: https://github.com/YuukiOtokawa/DirectX12/pull/3
- ブランチ: `claude/cross-conversation-reference-b31077` → `master`
- コミット: `c37385c`（`9c51ae4` の上に作成）
- 状態: PR作成済み・未マージ（2026-10-26 時点）

## 目的

`Split_Project.md`（ブランチ `claude/project-split-planning-jf2jnn` にある）の Phase 0 のうち、ビルド成果物を git の管理から外す作業だけを先に master に入れる。
プロジェクト分割ブランチと他の作業ブランチを並行させたとき、コミットされた `DirectX12.exe` などのバイナリが、どのブランチでも毎回コンフリクトする状態を解消するため。

## 変更内容

### `DirectX12.vcxproj`（x64 の Debug / Release）

| 項目 | 変更前 | 変更後 |
| --- | --- | --- |
| `OutDir` | `$(SolutionDir)`（リポジトリ直下） | `$(SolutionDir)Build\$(Platform)\$(Configuration)\` |
| `IntDir` | 既定（`x64\$(Configuration)\`） | `$(SolutionDir)Build\$(Platform)\$(Configuration)\Intermediate\$(ProjectName)\` |
| `LocalDebuggerWorkingDirectory` | `.vcxproj.user`（git 管理外）でのみ指定 | `$(SolutionDir)` を `.vcxproj` に記載 |
| ビルド後イベント | なし | `assimp-vc145-mtd.dll` を `$(OutDir)` にコピー |

- 作業ディレクトリをリポジトリ直下に固定したのは、`Assets\` と `Code/Shader/` を相対パスで読むため（`Code/Utility/ResourcePath.h`）。
- assimp の DLL をコピーするのは、exe がリポジトリ直下から移動したため。

### `.gitignore` と追跡対象

- `Build/`、リポジトリ直下の `DirectX12.exe` / `DirectX12.exp` / `DirectX12.lib`、`imgui.ini` を追加。
- これら4ファイルを `git rm --cached` で追跡対象から外した。
- `yaml-cpp` の出力先（`x64\$(Configuration)\`）は、もともと git の管理外なので変更していない。
- `assimp-vc145-mtd.dll`、`Code/assimp/lib/*`、`Code/DirectXTex/*.lib` は外部ライブラリなので、引き続き git で管理する。

## 確認状況

| 確認 | 結果 |
| --- | --- |
| Debug x64 の MSBuild ビルド | 成功。成果物は `Build\x64\Debug\` に出力され、git の管理外になっている |
| 起動確認（リポジトリ直下を作業ディレクトリにして起動） | 8秒間落ちずに動いた。画面の内容は見ていない |
| Release x64 のビルド | 未確認 |

## マージ時の注意

1. **作業ツリーの `imgui.ini` が削除される**。追跡対象から外すコミットを取り込むと、git がファイル自体も消すため。ImGui のウィンドウ配置がリセットされる。残したい場合は、マージの後に次のコマンドで戻す。
   ```
   git show 9c51ae4:imgui.ini > imgui.ini
   ```
   リポジトリ直下の古い `DirectX12.exe` / `.exp` / `.lib` も一緒に消えるが、これは問題ない。
2. **exe をコミットしている他のブランチをこの後にマージすると、片方では変更・もう片方では削除というコンフリクトが一度だけ起きる**。次のコマンドで解決できる。
   ```
   git rm DirectX12.exe DirectX12.exp DirectX12.lib imgui.ini
   ```
3. **exe をエクスプローラーから直接起動すると Assets を見つけられない**。作業ディレクトリが `Build\x64\<構成>\` になるため。Visual Studio から起動する場合は問題ない。

## 背景：分割ブランチと並行作業のコンフリクト要因（提案）

この PR を作る前に、`Split_Project.md` と、計画を書いた後に master に入ったコミットを突き合わせて洗い出したもの。

| 要因 | 内容 | 対策の提案 |
| --- | --- | --- |
| コミットされたビルド成果物 | どのブランチでもコミットのたびにバイナリのコンフリクトが起きる | **この PR で解消** |
| `DirectX12.vcxproj` / `.filters` | ファイルを足すたびに変わる。Phase 3 では登録先自体が `Core.vcxproj` / `Editor.vcxproj` に移る | 分割は Phase ごとに master へマージし、分割ブランチを長く生かさない |
| `Inspector()` の削除（Phase 1） | 別ブランチで `Inspector()` 付きのコンポーネントを書くと、マージ後にコンパイルエラーになる | Phase 1 の間は既存の Inspector を触らない。新しい Inspector は、Phase 1 が master に入ってから Drawer として書く |
| ディレクトリ再配置（Phase 5） | 全ファイルを rename する。古いディレクトリに追加されたファイルが取り残される | 他のブランチがない時期にまとめて一気に行う |

- シェーダー（`.hlsl`）や Render 内部の作業は、Phase 1〜4 とほとんどぶつからない。
- `Split_Project.md` に書かれた行番号は、作成後のコミット（`MeshRenderer.cpp` / `RenderManager.cpp` / `GameManager.cpp` / `Main.cpp` などの変更）によって既にずれている。
