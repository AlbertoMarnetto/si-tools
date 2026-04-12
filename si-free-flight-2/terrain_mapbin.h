#pragma once

#include <cstdint>
#include <vector>

// Embedded data from MAP.BIN, taken from the original FreeFlight
// 5372 bytes, 179 map areas
// TODO: find a way to compute the same data from STUNT.EXE
extern std::vector<uint8_t> mapbin_data;

