#pragma once
#include <cstdint>

namespace EngineCore::Manager { class ObjectManager; }

namespace EngineCore::General {

	class Object {
		uint64_t _instanceID = 0;

		// ID(ハンドル) はオブジェクトの生成・登録を司る ObjectManager だけが設定する
		friend class EngineCore::Manager::ObjectManager;
		void SetID(uint64_t id) { _instanceID = id; }
	public:
		virtual void DrawInspector() {}

		uint64_t GetID() const { return _instanceID; }

		// 同一インスタンス(同じ index+generation のハンドル)かどうかで比較する
		bool operator==(const Object& other) const {
			return GetID() == other.GetID();
		}
		bool IsValid() const;
	};
}