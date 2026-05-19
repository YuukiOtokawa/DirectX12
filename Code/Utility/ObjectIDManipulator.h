#pragma once

#include <cstdint>

using Generation = uint32_t;
using Index = uint32_t;
using ID = uint64_t;

inline ID CreateID(const Index index, const Generation generation) {
	return (static_cast<ID>(generation) << 32) | index;
}

inline Index GetIndex(const ID id) {
	return static_cast<Index>(id & 0xFFFFFFFF);
}

inline Generation GetGeneration(const ID id) {
	return static_cast<Generation>(id >> 32);
}
