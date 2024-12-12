#pragma once

#include <cstdint>

inline uint16_t swap16(uint16_t x) { return (x>>8)|(x<<8); }
inline uint32_t swap32(uint32_t x) { return (x>>24)|((x>>8)&0xFF00)|((x<<8)&0xFF0000)|(x<<24); }
