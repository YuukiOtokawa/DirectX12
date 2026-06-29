# Antigravity Rules

## Windows環境でのビルドとバージョン管理に関するルール

1. **Visual Studioバージョンの事前調査**:
   - Windows環境でビルドを実行する前に、必ず `.vcxproj` ファイル内の `<PlatformToolset>` タグを確認し、`C:\Program Files\Microsoft Visual Studio` 配下の構成（例：`18` ディレクトリ = VS2026）を調査してください。
   - 環境に合致した正しいバージョンの `vcvars64.bat` または `VsDevCmd.bat` を明示的に呼び出してビルドを実行してください。

2. **Git操作の禁止（ユーザー管理の尊重）**:
   - ユーザーの明示的な指示や承認がない限り、`git` コマンド（git status, git restore, git checkout, git commit 等）は一切実行しないでください。
   - 変更の取り消しや修正が必要な場合は、`git restore` 等を使わず、コード編集ツール（replace_file_content等）を用いて安全に手動で書き換えてください。
