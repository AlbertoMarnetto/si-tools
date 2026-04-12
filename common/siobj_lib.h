#pragma once

#include <vector>
#include <array>
#include <cstdint>

// Stunt Island palette (768 bytes = 256 colors * RGB)
extern const uint8_t sipal[768];

// SOD Vertex structure
struct SIVERTEX {
    int x, y, z;
    int other;
    int flags;
};

// Parsed 3D object from SOD bytecode
struct ParsedObject {
    std::vector<SIVERTEX> vertices;
    std::vector<std::vector<int>> faces;
    std::vector<int> face_colors;
    int y_offset;
    float scale;
    int skipped_spheres;
    int skipped_blocks;
    int skipped_disks;
    int skipped_dots;
    int approx_spheres;
    int approx_disks;

    ParsedObject() : y_offset(0), scale(1.0f), skipped_spheres(0), skipped_blocks(0),
                     skipped_disks(0), skipped_dots(0), approx_spheres(0), approx_disks(0) {}
};

// RES file entry
struct RESEntry {
    uint16_t id;
    uint32_t offset;
};

// Get number of entries in RES file without loading full data
// Returns number of entries, or -1 on error
int sod_get_res_count(const char* filename);

// Load RES file and return list of entries
// Returns number of entries loaded, or -1 on error
int sod_load_res(const char* filename, std::vector<RESEntry>& entries, std::vector<uint8_t>& data);

// Parse a single SOD object from buffer
// buf: pointer to start of SOD object data (after RES header)
// Returns ParsedObject with geometry
ParsedObject sod_parse_object(const uint8_t* buf, uint32_t size);

// Export ParsedObject to OBJ file
void sod_export_obj(const ParsedObject& obj, const char* filename);

// Export ParsedObject to MTL file
void sod_export_mtl(const ParsedObject& obj, const char* filename);

// Stunt Island palette (768 bytes = 256 colors * 3 channels)
extern const uint8_t sipal[768];

