#pragma once

#include <string>
#include <vector>

#include "Engine/Public/EngineAPI.h"
#include "Engine/Source/Render/VertexTypes.h"

class VertexData;

// エンジンが読むメッシュの形式（.meshbin）と、その読み込み。
// .obj / .fbx などの元ファイルの解析（assimp を含む）はエンジンに持たせず、AssetPipeline.lib が行う。
//   エディタ  : 起動時に SetMeshImporter() で AssetPipeline の取り込み関数を登録し、元ファイルを直接読む
//   製品      : AssetImporter.exe が事前に変換した .meshbin だけを読む
namespace EngineCore::Asset {

	// GPU に依存しないメッシュのデータ。.meshbin の中身と同じ
	struct MeshAsset {
		std::vector<Render::Types::VERTEX> vertices;
		std::vector<unsigned int> indices;
		Render::PrimitiveTopology topology = Render::PrimitiveTopology::TriangleList;
		// テクスチャのファイル名（メッシュのファイルと同じフォルダからの相対パス）
		std::vector<std::string> textures;
	};

	ENGINE_API bool SaveMeshAsset(const std::string& filePath, const MeshAsset& mesh);
	ENGINE_API bool LoadMeshAsset(const std::string& filePath, MeshAsset& mesh);

	// 元ファイル（.obj / .fbx など）を MeshAsset に変換する関数
	using MeshImporter = bool (*)(const std::string& sourcePath, MeshAsset& mesh);
	ENGINE_API void SetMeshImporter(MeshImporter importer);

	// メッシュを読み込み、テクスチャを GPU に載せた VertexData を返す。失敗したら nullptr。
	// .meshbin はそのまま読む。それ以外の拡張子は、Distribution では同名の .meshbin を、
	// Repository では SetMeshImporter() で登録された関数で元ファイルを読む
	ENGINE_API VertexData* LoadMesh(const std::string& filePath);
}
