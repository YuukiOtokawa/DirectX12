#pragma once

#include <string>

// アセット・シェーダーの参照先ディレクトリ
// Repository  : リポジトリ内のフォルダを直接参照（エディタ用。Assets\ / Code/Shader/）
// Distribution: 実行フォルダ直下の Resource フォルダを参照（製品用。Resource\Asset\ / Resource/Shader/）
//
// どちらを使うかは exe が起動時に決める（Engine::Initialize の引数）。
// EngineCore.lib はエディタと製品の両方の exe で共有するので、_DEBUG などのマクロでは切り替えられない。
namespace EngineCore::ResourcePath {

	enum class Layout {
		Repository,
		Distribution,
	};

	void SetLayout(Layout layout);
	Layout GetLayout();

	// 参照先ディレクトリを前に付けたパスを返す
	std::string Asset(const std::string& fileName);
	std::string Shader(const std::string& fileName);
}
