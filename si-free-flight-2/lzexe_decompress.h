#pragma once

#include <cstdint>
#include <vector>

// Decompress LZEXE-compressed executable
// Returns 0 on success, -1 on failure
int lzexe_decompress(const uint8_t* input_data, size_t input_size,
                     std::vector<uint8_t>& output_data);

