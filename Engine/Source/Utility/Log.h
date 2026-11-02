#pragma once

#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>

#include "Engine/Public/EngineAPI.h"

// エンジン共通のログ。RenderManager に依存しないので、初期化前や失敗中でも書ける。
// 出力先は OutputDebugStringA と内部のバッファで、エディタのログウィンドウはバッファを読んで表示する。
namespace EngineCore::Log {

	enum class Level { Info, Warning, Error, Fatal };

	struct Entry {
		uint64_t id;			// 追加順の通し番号。古い行が捨てられても変わらない
		Level level;
		std::string time;		// "HH:MM:SS"
		std::string message;
		uint32_t repeatCount;	// 1 以上
	};

	// 直前と同じレベル・本文なら、新しい行を作らず repeatCount を増やす
	ENGINE_API void Write(Level level, const std::string& message);
	ENGINE_API void Clear();

	// 内容が変わるたびに増える。表示側は変化したときだけ Snapshot を取る
	ENGINE_API uint64_t GetRevision();
	ENGINE_API std::vector<Entry> Snapshot();
	// Error 以上の累計。古い行が捨てられても減らず、Clear で 0 に戻る
	ENGINE_API uint64_t GetErrorCount();

	ENGINE_API std::string FormatHresult(HRESULT hr);

	// 失敗なら message と HRESULT を本文にした std::runtime_error を投げる。記録は捕捉側で行う
	ENGINE_API void ThrowIfFailed(HRESULT hr, const std::string& message);
}
