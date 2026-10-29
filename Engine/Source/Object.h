#pragma once
#include <cstdint>

#include "Engine/Public/EngineAPI.h"

namespace EngineCore::Manager { class ObjectManager; }

namespace EngineCore::General {

	class ENGINE_API Object {
		uint64_t m_InstanceID = 0;

		// ID(ハンドル) はオブジェクトの生成・登録を司る ObjectManager だけが設定する
		friend class EngineCore::Manager::ObjectManager;
		void SetID(uint64_t id) { m_InstanceID = id; }
	public:
		virtual ~Object() = default;

		uint64_t GetID() const { return m_InstanceID; }

		// 同一インスタンス(同じ index+generation のハンドル)かどうかで比較する
		bool operator==(const Object& other) const {
			return GetID() == other.GetID();
		}
		bool IsValid() const;
	};
}