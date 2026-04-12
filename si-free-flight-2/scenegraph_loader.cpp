#include "scenegraph_loader.h"
#include "lzexe_decompress.h"
#include "scene.h"
#include "write_debug.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <set>

// Use kIslandScale from island.h

SceneGraphLoader::SceneGraphLoader() {}

SceneGraphLoader::~SceneGraphLoader() {}

bool SceneGraphLoader::parseMZHeader(const uint8_t* data, size_t size,
                                      size_t& stack_offset, uint16_t& init_sp) {
    if (size < 28) {
        write_debug("File too small for MZ header\n");
        return false;
    }

    // Check MZ signature
    if (data[0] != 'M' || data[1] != 'Z') {
        write_debug("Not a valid MZ executable\n");
        return false;
    }

    // Parse header fields (little-endian)
    uint16_t header_size_paragraphs = data[8] | (data[9] << 8);
    uint16_t init_ss = data[14] | (data[15] << 8);
    init_sp = data[16] | (data[17] << 8);

    // Stack segment starts at this file offset
    size_t header_size = header_size_paragraphs * 16;
    stack_offset = header_size + (init_ss * 16);

    if (stack_offset >= size) {
        write_debug("Stack offset beyond file size\n");
        return false;
    }

    write_debug("MZ header: header_size=%zu, init_ss=0x%04X, init_sp=0x%04X\n",
           header_size, init_ss, init_sp);
    write_debug("Stack segment at file offset: 0x%zX\n", stack_offset);

    return true;
}

std::string SceneGraphLoader::synthesizeObjectName(uint16_t obj_id) {
    // SCENERY files use IDs 0x0200-0x07FF
    // Determine which RES file based on ID range
    //int res_idx = (obj_id >> 8) - 1;

    char name[32];
    snprintf(name, sizeof(name), "%04X", obj_id);
    return std::string(name);
}

void SceneGraphLoader::extractAnchorPoints(const uint8_t* decompressed_data, size_t decompressed_size) {
    // Search for anchor point pattern in decompressed data
    // First entry: (-6008, 0, 4) -> 88 E8 00 00 04 00
    const uint8_t pattern[] = {0x88, 0xE8, 0x00, 0x00, 0x04, 0x00};
    
    size_t anchorpt_offset = 0;
    bool found = false;
    
    for (size_t i = 0; i < decompressed_size - sizeof(pattern); i++) {
        bool match = true;
        for (size_t j = 0; j < sizeof(pattern); j++) {
            if (decompressed_data[i + j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            // Verify this looks like an anchor point array
            // Check entry 1 at offset + 28: (-8000, 0, 3000) -> C0 E0 00 00 B8 0B
            if (i + 34 < decompressed_size) {
                const uint8_t pattern2[] = {0xC0, 0xE0, 0x00, 0x00, 0xB8, 0x0B};
                bool match2 = true;
                for (size_t j = 0; j < 6; j++) {
                    if (decompressed_data[i + 28 + j] != pattern2[j]) {
                        match2 = false;
                        break;
                    }
                }
                if (match2) {
                    anchorpt_offset = i;
                    found = true;
                    break;
                }
            }
        }
    }
    
    if (!found) {
        write_debug("Warning: Could not find anchor point pattern in decompressed data\n");
        return;
    }
    
    const size_t anchorpt_stride = 28;
    const size_t num_anchorpts = 2048;
    
    if (anchorpt_offset + num_anchorpts * anchorpt_stride > decompressed_size) {
        write_debug("Warning: Anchor point data beyond decompressed size\n");
        return;
    }
    
    anchorPoints.reserve(num_anchorpts);
    for (size_t i = 0; i < num_anchorpts; i++) {
        size_t offset = anchorpt_offset + i * anchorpt_stride;
        int16_t x = (int16_t)(decompressed_data[offset] | (decompressed_data[offset + 1] << 8));
        int16_t y = (int16_t)(decompressed_data[offset + 2] | (decompressed_data[offset + 3] << 8));
        int16_t z = (int16_t)(decompressed_data[offset + 4] | (decompressed_data[offset + 5] << 8));
        anchorPoints.push_back({x, y, z});
    }
    
    write_debug("Extracted %zu anchor points from decompressed executable (offset 0x%zX)\n", 
           anchorPoints.size(), anchorpt_offset);
}

void SceneGraphLoader::traverseSceneGraph(const uint8_t* stack_data, size_t stack_size) {
    // Start traversal from offset 0 (SCENESTART)
    // No visited tracking - same node can be reached via different paths
    processNode(stack_data, 0, stack_size, 0, 0, 0, 0);

    write_debug("Extracted %zu placements from scene graph\n", placements.size());
}

void SceneGraphLoader::processNode(const uint8_t* data, size_t offset, size_t data_size,
                                    int32_t pos_x, int32_t pos_y, int32_t pos_z,
                                    int depth) {
    // Limit depth to prevent infinite loops
    if (depth > 200) return;
    if (offset >= data_size - 1) return;

    // Read node type
    uint16_t node_type = data[offset] | (data[offset + 1] << 8);

    switch (node_type) {
        case NODE_POS: {
            // sp_pos: type(2), dx(2), dy(2), dz(2), ptr(2)
            if (offset + 10 > data_size) return;
            int16_t dx = (int16_t)(data[offset + 2] | (data[offset + 3] << 8));
            int16_t dy = (int16_t)(data[offset + 4] | (data[offset + 5] << 8));
            int16_t dz = (int16_t)(data[offset + 6] | (data[offset + 7] << 8));
            uint16_t ptr = data[offset + 8] | (data[offset + 9] << 8);

            processNode(data, ptr, data_size, pos_x + dx, pos_y + dy, pos_z + dz, depth + 1);
            break;
        }

        case NODE_POSXZ: {
            // sp_posxz: type(2), dx(2), dz(2), ptr(2)
            if (offset + 8 > data_size) return;
            int16_t dx = (int16_t)(data[offset + 2] | (data[offset + 3] << 8));
            int16_t dz = (int16_t)(data[offset + 4] | (data[offset + 5] << 8));
            uint16_t ptr = data[offset + 6] | (data[offset + 7] << 8);

            processNode(data, ptr, data_size, pos_x + dx, pos_y, pos_z + dz, depth + 1);
            break;
        }

        case NODE_1NODE: {
            // sp_1node: type(2), obj(2), ext(2), splitdist(2), ptr(2)
            if (offset + 10 > data_size) return;
            uint16_t obj = data[offset + 2] | (data[offset + 3] << 8);
            uint16_t ptr = data[offset + 8] | (data[offset + 9] << 8);

            if (obj != 0) {
                Placement p;
                p.obj_id = obj;
                p.name = synthesizeObjectName(obj);
                p.x = pos_x;
                p.y = pos_y;
                p.z = pos_z;
                placements.push_back(p);
            }

            if (ptr != 0) {
                processNode(data, ptr, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            break;
        }

        case NODE_LEAF: {
            // sp_leaf: type(2), obj(2)
            if (offset + 4 > data_size) return;
            uint16_t obj = data[offset + 2] | (data[offset + 3] << 8);

            if (obj != 0) {
                Placement p;
                p.obj_id = obj;
                p.name = synthesizeObjectName(obj);
                p.x = pos_x;
                p.y = pos_y;
                p.z = pos_z;
                placements.push_back(p);
            }
            break;
        }

        case NODE_2NODE: {
            // sp_2node: type(2), ptr1(2), ptr2(2)
            if (offset + 6 > data_size) return;
            uint16_t ptr1 = data[offset + 2] | (data[offset + 3] << 8);
            uint16_t ptr2 = data[offset + 4] | (data[offset + 5] << 8);

            if (ptr1 != 0) {
                processNode(data, ptr1, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            if (ptr2 != 0) {
                processNode(data, ptr2, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            break;
        }

        case NODE_LIST: {
            // sp_list: type(2), ptr1(2), ptr2(2), ..., 0(2)
            size_t i = offset + 2;
            while (i + 2 <= data_size) {
                uint16_t ptr = data[i] | (data[i + 1] << 8);
                i += 2;
                if (ptr == 0) break;
                processNode(data, ptr, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            break;
        }

        case NODE_XPLANE:
        case NODE_ZPLANE: {
            // sp_xplane/sp_zplane: type(2), ptr1(2), ptr2(2), coord(2)
            if (offset + 8 > data_size) return;
            uint16_t ptr1 = data[offset + 2] | (data[offset + 3] << 8);
            uint16_t ptr2 = data[offset + 4] | (data[offset + 5] << 8);

            if (ptr1 != 0) {
                processNode(data, ptr1, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            if (ptr2 != 0) {
                processNode(data, ptr2, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            break;
        }

        case NODE_HSLOPE:
        case NODE_VSLOPE: {
            // sp_hslope/sp_vslope: type(2), ptr1(2), ptr2(2), slope(2), coord(2)
            if (offset + 10 > data_size) return;
            uint16_t ptr1 = data[offset + 2] | (data[offset + 3] << 8);
            uint16_t ptr2 = data[offset + 4] | (data[offset + 5] << 8);

            if (ptr1 != 0) {
                processNode(data, ptr1, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            if (ptr2 != 0) {
                processNode(data, ptr2, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            break;
        }

        case NODE_GLEAF: {
            // sp_gleaf: type(2), next_ptr(2), ...
            if (offset + 4 > data_size) return;
            uint16_t ptr = data[offset + 2] | (data[offset + 3] << 8);

            if (ptr != 0) {
                processNode(data, ptr, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            break;
        }

        case NODE_GROUND: {
            // sp_ground: type(2), splitdist(2), ext(2), x(2), y(2), z(2), ptr1(2), ptr2(2), ...
            if (offset + 16 > data_size) return;
            uint16_t ptr1 = data[offset + 12] | (data[offset + 13] << 8);
            uint16_t ptr2 = data[offset + 14] | (data[offset + 15] << 8);

            if (ptr1 != 0) {
                processNode(data, ptr1, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            if (ptr2 != 0) {
                processNode(data, ptr2, data_size, pos_x, pos_y, pos_z, depth + 1);
            }
            break;
        }

        default:
            // Unknown node type - skip
            break;
    }
}

bool SceneGraphLoader::loadFromExecutable(const char* exe_path) {
    write_debug("Loading scene graph from executable: %s\n", exe_path);

    // Read entire file
    FILE* f = fopen(exe_path, "rb");
    if (!f) {
        write_debug("Failed to open executable: %s\n", exe_path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> file_data(file_size);
    if (fread(file_data.data(), 1, file_size, f) != (size_t)file_size) {
        write_debug("Failed to read executable\n");
        fclose(f);
        return false;
    }
    fclose(f);

    write_debug("Read %ld bytes from executable\n", file_size);

    // Check if file is LZEXE-compressed
    // LZEXE files have "LZ" signature at offset 0x1C (after MZ header fields)
    bool is_lzexe = (file_size > 0x1E && 
                     file_data[0x1C] == 'L' && file_data[0x1D] == 'Z');
    
    std::vector<uint8_t> executable_data;
    
    if (is_lzexe) {
        write_debug("Detected LZEXE-compressed executable, decompressing...\n");
        if (lzexe_decompress(file_data.data(), file_size, executable_data) != 0) {
            write_debug("Failed to decompress LZEXE\n");
            return false;
        }
        write_debug("Decompressed to %zu bytes\n", executable_data.size());
    } else {
        write_debug("Detected uncompressed executable\n");
        executable_data = std::move(file_data);
    }

    // Extract anchor points from decompressed data
    extractAnchorPoints(executable_data.data(), executable_data.size());

    // Parse MZ header
    size_t stack_offset;
    uint16_t init_sp;
    if (!parseMZHeader(executable_data.data(), executable_data.size(), stack_offset, init_sp)) {
        return false;
    }

    // Extract stack segment
    const uint8_t* stack_data = executable_data.data() + stack_offset;
    size_t stack_size = executable_data.size() - stack_offset;

    write_debug("Stack segment size: %zu bytes\n", stack_size);

    // Traverse scene graph
    traverseSceneGraph(stack_data, stack_size);

    // Deduplicate placements
    write_debug("Deduplicating placements...\n");
    std::sort(placements.begin(), placements.end(), [](const Placement& a, const Placement& b) {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        if (a.z != b.z) return a.z < b.z;
        return a.obj_id < b.obj_id;
    });
    placements.erase(std::unique(placements.begin(), placements.end(),
                                  [](const Placement& a, const Placement& b) {
                                      return a.x == b.x && a.y == b.y && 
                                             a.z == b.z && a.obj_id == b.obj_id;
                                  }),
                     placements.end());

    write_debug("Final unique placements: %zu\n", placements.size());

    return true;
}

size_t SceneGraphLoader::getUniqueObjectCount() const {
    std::set<uint16_t> unique_ids;
    for (const auto& p : placements) {
        unique_ids.insert(p.obj_id);
    }
    return unique_ids.size();
}
