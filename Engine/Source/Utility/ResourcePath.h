#pragma once

#include <string>

#include "Engine/Public/EngineAPI.h"

// アセット・シェーダーの参照先ディレクトリ
// Repository  : リポジトリ内のフォルダを直接参照（エディタ用。Assets\ / Engine/Shader/）
// Distribution: 実行フォルダ直下の Resource フォルダを参照（製品用。Resource\Asset\ / Resource/Shader/）
//
// どちらを使うかは exe が起動時に決める（Engine::Initialize の引数）。
// EngineCore.lib はエディタと製品の両方の exe で共有するので、_DEBUG などのマクロでは切り替えられない。
//
// Distribution の Resource フォルダは、ビルド時にツール exe が生成する（Player.vcxproj の配布ターゲット）。
//   シェーダー: ShaderCompiler.exe が .hlsl から .cso とメタデータを作る
//   アセット  : AssetImporter.exe が .obj / .fbx を .meshbin に変換し、テクスチャはそのままコピーする
namespace EngineCore::ResourcePath {

	enum class Layout {
		Repository,
		Distribution,
	};

	ENGINE_API void SetLayout(Layout layout);
	ENGINE_API Layout GetLayout();

	// 参照先ディレクトリを前に付けたパスを返す
	ENGINE_API std::string Asset(const std::string& fileName);
	ENGINE_API std::string Shader(const std::string& fileName);

	// ツール exe が出力するファイルのパス。元ファイルのパスの拡張子を置き換えて作る
	// 例: "Resource/Shader/Bloom.hlsl" と "pixBlurH" → "Resource/Shader/Bloom.pixBlurH.cso"
	ENGINE_API std::string CompiledShader(const std::string& shaderFile, const std::string& entryPoint);
	// 例: "Resource/Shader/Bloom.hlsl" → "Resource/Shader/Bloom.shadermeta"
	ENGINE_API std::string ShaderMetadataFile(const std::string& shaderFile);
	// 例: "Resource\Asset\sky.obj" → "Resource\Asset\sky.meshbin"
	ENGINE_API std::string MeshBin(const std::string& modelFile);
}
