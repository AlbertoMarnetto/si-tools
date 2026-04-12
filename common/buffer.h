#pragma once

#include <cstdbool>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

/**
 * Simple buffer+pointer combo
 */
struct Buffer
{
    std::vector<uint8_t> data;
    size_t pos{0};

    bool LoadFile(const char* path)
    {
        FILE* handle = fopen(path, "rb");
        if (!handle) {
            return false;
        }

        fseek(handle, 0, SEEK_END);
        data.resize(ftell(handle));
        fseek(handle, 0, SEEK_SET);

        fread(data.data(), 1, data.size(), handle);
        fclose(handle);
        pos = 0;
        return true;
    }

    void Clear()
    {
        data.clear();
        pos = 0;
    }

    uint8_t NextByte()
    {
        return data[pos++];
    }

    uint16_t NextWord()
    {
        uint16_t result = data[pos] | ((uint16_t) data[pos + 1] << 8);
        pos += 2;
        return result;
    }
};

