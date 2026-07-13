#pragma once

// アセット・シェーダーの参照先ディレクトリ
// Debug  : リポジトリ内のフォルダを直接参照(開発用)
// Release: 実行フォルダ直下の Resource フォルダを参照(配布用)
#ifdef _DEBUG
#define ASSET_DIR  "Assets\\"
#define SHADER_DIR "Code/Shader/"
#else
#define ASSET_DIR  "Resource\\Asset\\"
#define SHADER_DIR "Resource/Shader/"
#endif
