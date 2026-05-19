#pragma once
#include <cstdint>

namespace EngineCore::General {

	class Object {
		uint64_t _instanceID;
	public:
		virtual void DrawInspector() {}

		uint64_t GetID() const { return _instanceID; }
		
		bool operator==(const Object& other) const {
			return IsValid();
		}
		bool IsValid() const;
	};
}