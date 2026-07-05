#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>

namespace EngineCore::Render {

	// GPUがフェンスを通過するまで保持してから解放する汎用キュー
	// （PSO差し替え・テクスチャ差し替え時の「使用中リソースを即座に破棄しない」ためのもの）
	template <typename T>
	class DeferredReleaseQueue
	{
	public:
		void Push(T item, uint64_t fenceValue)
		{
			m_Pending.push_back({ std::move(item), fenceValue });
		}

		// completedValue まで実行が完了しているエントリを解放する（Tのデストラクタに委ねる）
		void ReleaseCompleted(uint64_t completedValue)
		{
			m_Pending.erase(
				std::remove_if(m_Pending.begin(), m_Pending.end(),
					[completedValue](const Entry& pending) {
						return completedValue >= pending.fenceValue;
					}),
				m_Pending.end()
			);
		}

	private:
		struct Entry {
			T item;
			uint64_t fenceValue;
		};
		std::vector<Entry> m_Pending;
	};

}
