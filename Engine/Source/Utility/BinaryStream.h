#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

// ツール exe が書き出し、エンジンが読み込むバイナリファイル（.meshbin / .shadermeta）用の読み書き補助。
// 同じマシン・同じコンパイラで書いて読む前提なので、エンディアンや構造体のパディングは考慮しない。
namespace EngineCore::BinaryStream {

	template <typename T>
	void Write(std::ofstream& out, const T& value) {
		static_assert(std::is_trivially_copyable_v<T>);
		out.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	inline void Write(std::ofstream& out, const std::string& value) {
		Write(out, static_cast<uint32_t>(value.size()));
		out.write(value.data(), value.size());
	}

	template <typename T>
	void WriteArray(std::ofstream& out, const std::vector<T>& values) {
		static_assert(std::is_trivially_copyable_v<T>);
		Write(out, static_cast<uint32_t>(values.size()));
		out.write(reinterpret_cast<const char*>(values.data()), sizeof(T) * values.size());
	}

	template <typename T>
	bool Read(std::ifstream& in, T& value) {
		static_assert(std::is_trivially_copyable_v<T>);
		return static_cast<bool>(in.read(reinterpret_cast<char*>(&value), sizeof(T)));
	}

	inline bool Read(std::ifstream& in, std::string& value) {
		uint32_t size = 0;
		if (!Read(in, size)) return false;
		value.resize(size);
		return static_cast<bool>(in.read(value.data(), size));
	}

	template <typename T>
	bool ReadArray(std::ifstream& in, std::vector<T>& values) {
		static_assert(std::is_trivially_copyable_v<T>);
		uint32_t size = 0;
		if (!Read(in, size)) return false;
		values.resize(size);
		return static_cast<bool>(in.read(reinterpret_cast<char*>(values.data()), sizeof(T) * size));
	}

	// ファイルの先頭に置く識別子と版数。形式を変えたら版数を上げ、古いファイルを読まないようにする
	inline void WriteHeader(std::ofstream& out, uint32_t magic, uint32_t version) {
		Write(out, magic);
		Write(out, version);
	}

	inline bool ReadHeader(std::ifstream& in, uint32_t magic, uint32_t version) {
		uint32_t fileMagic = 0, fileVersion = 0;
		return Read(in, fileMagic) && Read(in, fileVersion) && fileMagic == magic && fileVersion == version;
	}
}
