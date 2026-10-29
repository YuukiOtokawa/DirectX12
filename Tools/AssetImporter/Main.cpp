// AssetImporter.exe <srcDir> <dstDir>
//
// srcDir 以下を再帰的にたどり、製品（Distribution）が読むアセットを dstDir に書き出す。
//   モデル（.obj / .fbx）: AssetPipeline で取り込み、同じ相対パスの .meshbin に変換する
//   .mtl / .objbin        : 書き出さない（.mtl の内容は .meshbin に入る。.objbin は旧形式のキャッシュ）
//   それ以外（.dds など）  : そのままコピーする
// 出力が入力より新しければ何もしない。失敗したらエラーを stderr に出し、終了コード 1 を返す。

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <string>

#include "Engine/Source/Asset/MeshAsset.h"
#include "Engine/Source/Utility/ResourcePath.h"
#include "Tools/AssetPipeline/ModelImporter.h"

namespace fs = std::filesystem;

namespace {

	bool HasExtension(const fs::path& path, const char* extension) {
		return _stricmp(path.extension().string().c_str(), extension) == 0;
	}

	// output が inputs のどれよりも新しければ true
	bool IsUpToDate(const fs::path& output, std::initializer_list<fs::path> inputs) {
		std::error_code ec;
		const auto outputTime = fs::last_write_time(output, ec);
		if (ec) return false;
		for (const auto& input : inputs) {
			const auto inputTime = fs::last_write_time(input, ec);
			if (ec) continue; // 無い入力（.mtl が無い .obj など）は比較しない
			if (inputTime > outputTime) return false;
		}
		return true;
	}

	bool ImportModelFile(const fs::path& source, const fs::path& destination) {
		const fs::path output = EngineCore::ResourcePath::MeshBin(destination.string());

		// .obj は同名の .mtl もテクスチャ名の取得に使うので、更新の判定に含める
		fs::path material = source;
		material.replace_extension(".mtl");
		if (IsUpToDate(output, { source, material })) return true;

		EngineCore::Asset::MeshAsset mesh;
		if (!AssetPipeline::ImportModel(source.string(), mesh)) {
			fprintf(stderr, "error: failed to import %s\n", source.string().c_str());
			return false;
		}
		if (!EngineCore::Asset::SaveMeshAsset(output.string(), mesh)) {
			fprintf(stderr, "error: failed to write %s\n", output.string().c_str());
			return false;
		}
		printf("imported: %s\n", output.string().c_str());
		return true;
	}

	bool CopyAssetFile(const fs::path& source, const fs::path& destination) {
		if (IsUpToDate(destination, { source })) return true;

		std::error_code ec;
		fs::copy_file(source, destination, fs::copy_options::overwrite_existing, ec);
		if (ec) {
			fprintf(stderr, "error: failed to copy %s (%s)\n", source.string().c_str(), ec.message().c_str());
			return false;
		}
		printf("copied: %s\n", destination.string().c_str());
		return true;
	}
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: AssetImporter.exe <srcDir> <dstDir>\n");
		return 1;
	}

	const fs::path srcDir = argv[1];
	const fs::path dstDir = argv[2];

	std::error_code ec;
	if (!fs::is_directory(srcDir, ec)) {
		fprintf(stderr, "error: source directory not found: %s\n", srcDir.string().c_str());
		return 1;
	}

	bool ok = true;
	for (const auto& item : fs::recursive_directory_iterator(srcDir)) {
		if (!item.is_regular_file()) continue;

		const fs::path& source = item.path();
		if (HasExtension(source, ".mtl") || HasExtension(source, ".objbin")) continue;

		const fs::path destination = dstDir / fs::relative(source, srcDir);
		fs::create_directories(destination.parent_path(), ec);
		if (ec) {
			fprintf(stderr, "error: failed to create %s (%s)\n", destination.parent_path().string().c_str(), ec.message().c_str());
			ok = false;
			continue;
		}

		const bool result = AssetPipeline::IsModelFile(source.string())
			? ImportModelFile(source, destination)
			: CopyAssetFile(source, destination);
		if (!result) ok = false;
	}

	return ok ? 0 : 1;
}
