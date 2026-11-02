#include "Log.h"

#include <cstdio>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace EngineCore::Log {

	namespace {
		constexpr size_t kMaxEntries = 1024;

		std::mutex s_Mutex;
		std::deque<Entry> s_Entries;
		uint64_t s_NextId = 0;
		uint64_t s_Revision = 0;
		uint64_t s_ErrorCount = 0;

		const char* ToString(Level level) {
			switch (level) {
			case Level::Info: return "Info";
			case Level::Warning: return "Warning";
			case Level::Error: return "Error";
			case Level::Fatal: return "Fatal";
			}
			return "Unknown";
		}

		std::string CurrentTime() {
			SYSTEMTIME now{};
			GetLocalTime(&now);
			char buffer[16];
			sprintf_s(buffer, "%02u:%02u:%02u", now.wHour, now.wMinute, now.wSecond);
			return buffer;
		}
	}

	void Write(Level level, const std::string& message) {
		// シェーダーのエラーなどは末尾に改行が付いてくるので、同じ本文として比較できるように落とす
		std::string text = message;
		while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
			text.pop_back();
		}

		OutputDebugStringA(("[" + std::string(ToString(level)) + "] " + text + "\n").c_str());

		std::lock_guard<std::mutex> lock(s_Mutex);
		if (!s_Entries.empty() && s_Entries.back().level == level && s_Entries.back().message == text) {
			s_Entries.back().time = CurrentTime();
			s_Entries.back().repeatCount++;
		}
		else {
			if (s_Entries.size() == kMaxEntries) {
				s_Entries.pop_front();
			}
			s_Entries.push_back({ s_NextId++, level, CurrentTime(), std::move(text), 1 });
		}
		if (level == Level::Error || level == Level::Fatal) {
			s_ErrorCount++;
		}
		s_Revision++;
	}

	void Clear() {
		std::lock_guard<std::mutex> lock(s_Mutex);
		s_Entries.clear();
		s_ErrorCount = 0;
		s_Revision++;
	}

	uint64_t GetRevision() {
		std::lock_guard<std::mutex> lock(s_Mutex);
		return s_Revision;
	}

	std::vector<Entry> Snapshot() {
		std::lock_guard<std::mutex> lock(s_Mutex);
		return std::vector<Entry>(s_Entries.begin(), s_Entries.end());
	}

	uint64_t GetErrorCount() {
		std::lock_guard<std::mutex> lock(s_Mutex);
		return s_ErrorCount;
	}

	std::string FormatHresult(HRESULT hr) {
		char buffer[32];
		sprintf_s(buffer, "HRESULT=0x%08lX", static_cast<unsigned long>(hr));
		return buffer;
	}

	void ThrowIfFailed(HRESULT hr, const std::string& message) {
		if (FAILED(hr)) {
			throw std::runtime_error(message + " (" + FormatHresult(hr) + ")");
		}
	}
}
