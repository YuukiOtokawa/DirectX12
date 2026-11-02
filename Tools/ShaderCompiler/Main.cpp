// ShaderCompiler.exe <srcDir> <dstDir> [--debug]
//
// srcDir 以下の *.hlsl（.hlsli は除く）を事前コンパイルし、製品（Distribution）が読むファイルを dstDir に書き出す。
//   <名前>.<エントリ>.cso : エントリ関数ごとのバイトコード。vtx* は vs_5_1、pix* は ps_5_1
//   <名前>.shadermeta     : マテリアルのプロパティ（ParseShaderMetadata の結果）
// ファイル名は EngineCore::ResourcePath::CompiledShader / ShaderMetadataFile と同じ規則で作る。
// エントリ関数が 1 つも無い .hlsl（他の .hlsl から include されるだけのもの）は何も出力しない。
// 失敗したらエラーを stderr に出し、終了コード 1 を返す。

#include <Windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "Engine/Source/Render/ShaderMetadata.h"
#include "Engine/Source/Utility/ResourcePath.h"

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;

namespace {

	// 行頭の「戻り値の型 vtx*( / pix*(」をエントリ関数とみなす
	std::vector<std::string> FindEntryPoints(const fs::path& file) {
		static const std::regex entryRegex(R"(^\s*\w+\s+((?:vtx|pix)\w*)\s*\()");

		std::vector<std::string> entries;
		std::set<std::string> seen;
		std::ifstream in(file);
		std::string line;
		while (std::getline(in, line)) {
			std::smatch match;
			if (std::regex_search(line, match, entryRegex) && seen.insert(match[1].str()).second)
				entries.push_back(match[1].str());
		}
		return entries;
	}

	bool StartsWith(const std::string& s, const char* prefix) {
		return s.rfind(prefix, 0) == 0;
	}

	bool CompileEntry(const fs::path& source, const std::string& entry, const char* target, bool debug, const fs::path& output) {
		const UINT flags = debug ? (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION) : D3DCOMPILE_OPTIMIZATION_LEVEL3;

		ComPtr<ID3DBlob> code;
		ComPtr<ID3DBlob> errors;
		HRESULT hr = D3DCompileFromFile(source.wstring().c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entry.c_str(), target, flags, 0, &code, &errors);

		if (errors && errors->GetBufferSize() > 0) {
			fprintf(stderr, "%.*s\n", static_cast<int>(errors->GetBufferSize()), static_cast<const char*>(errors->GetBufferPointer()));
		}
		if (FAILED(hr)) {
			fprintf(stderr, "error: failed to compile %s (%s, %s) hr=0x%08lX\n",
				source.string().c_str(), entry.c_str(), target, static_cast<unsigned long>(hr));
			return false;
		}

		std::ofstream out(output, std::ios::binary);
		out.write(static_cast<const char*>(code->GetBufferPointer()), code->GetBufferSize());
		if (!out) {
			fprintf(stderr, "error: failed to write %s\n", output.string().c_str());
			return false;
		}
		return true;
	}

	bool CompileShaderFile(const fs::path& source, const fs::path& destination, bool debug) {
		const std::vector<std::string> entries = FindEntryPoints(source);
		if (entries.empty()) {
			printf("skip (no entry point): %s\n", source.string().c_str());
			return true;
		}

		std::error_code ec;
		fs::create_directories(destination.parent_path(), ec);

		bool ok = true;
		for (const auto& entry : entries) {
			const char* target = StartsWith(entry, "vtx") ? "vs_5_1" : "ps_5_1";
			const fs::path output = EngineCore::ResourcePath::CompiledShader(destination.string(), entry);
			if (CompileEntry(source, entry, target, debug, output))
				printf("compiled: %s\n", output.string().c_str());
			else
				ok = false;
		}

		const EngineCore::Render::ShaderMetadata metadata = EngineCore::Render::ParseShaderMetadata(source.string());
		const std::string metaPath = EngineCore::ResourcePath::ShaderMetadataFile(destination.string());
		if (!EngineCore::Render::SaveShaderMetadata(metaPath, metadata)) {
			fprintf(stderr, "error: failed to write %s\n", metaPath.c_str());
			ok = false;
		}

		return ok;
	}
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: ShaderCompiler.exe <srcDir> <dstDir> [--debug]\n");
		return 1;
	}

	const fs::path srcDir = argv[1];
	const fs::path dstDir = argv[2];
	bool debug = false;
	for (int i = 3; i < argc; ++i) {
		if (std::string(argv[i]) == "--debug") debug = true;
	}

	std::error_code ec;
	if (!fs::is_directory(srcDir, ec)) {
		fprintf(stderr, "error: source directory not found: %s\n", srcDir.string().c_str());
		return 1;
	}

	bool ok = true;
	for (const auto& item : fs::recursive_directory_iterator(srcDir)) {
		if (!item.is_regular_file()) continue;

		// 拡張子は大文字小文字を区別しない。.hlsli は include 専用なので対象外
		if (_stricmp(item.path().extension().string().c_str(), ".hlsl") != 0) continue;

		const fs::path destination = dstDir / fs::relative(item.path(), srcDir);
		if (!CompileShaderFile(item.path(), destination, debug))
			ok = false;
	}

	return ok ? 0 : 1;
}
