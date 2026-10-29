#pragma once

#include <string>

#include "Engine/Source/Asset/MeshAsset.h"

// モデルの元ファイル（.obj / .fbx）を、エンジンのメッシュ形式（MeshAsset）に変換する。
// assimp を使うのはここだけ。エディタ（実行中の取り込み）と AssetImporter.exe（配布用の事前変換）が使い、
// EngineCore.dll と AsteroidPlayer.exe には入らない。
namespace AssetPipeline {

	// 拡張子で判別して取り込む。EngineCore::Asset::SetMeshImporter() に渡せる形にしてある
	bool ImportModel(const std::string& sourcePath, EngineCore::Asset::MeshAsset& mesh);

	// ImportModel が扱える拡張子か
	bool IsModelFile(const std::string& path);

	bool ImportObj(const std::string& sourcePath, EngineCore::Asset::MeshAsset& mesh);
	bool ImportFbx(const std::string& sourcePath, EngineCore::Asset::MeshAsset& mesh);
}
