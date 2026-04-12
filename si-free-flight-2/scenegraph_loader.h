#pragma once

#include "island.h"
#include <cstdint>
#include <string>
#include <vector>

struct AnchorPt;

// Scene graph node types (from STRUCS.ASM)
enum SceneNodeType : uint16_t {
    NODE_POS = 0,
    NODE_1NODE = 2,
    NODE_2NODE = 4,
    NODE_XPLANE = 6,
    NODE_ZPLANE = 8,
    NODE_LEAF = 10,
    NODE_GROUND = 12,
    NODE_GLEAF = 14,
    NODE_HSLOPE = 16,
    NODE_VSLOPE = 18,
    NODE_LIST = 20,
    NODE_POSXZ = 22,
};

// Placement extracted from scene graph
struct Placement {
    uint16_t obj_id;      // Object ID
    std::string name;     // Synthesized name (e.g., "obj_R1_I11")
    int32_t x, y, z;      // SOD coordinates
};

// Scene graph loader - extracts placements from LZEXE-compressed executable
class SceneGraphLoader {
public:
    SceneGraphLoader();
    ~SceneGraphLoader();

    // Load scene graph from LZEXE-compressed executable and extract placements
    // Returns true on success
    bool loadFromExecutable(const char* exe_path);

    const std::vector<Placement>& getPlacements() const { return placements; }
    const std::vector<AnchorPt>& getAnchorPoints() const { return anchorPoints; }
    size_t getUniqueObjectCount() const;

private:
    bool parseMZHeader(const uint8_t* data, size_t size, 
                       size_t& stack_offset, uint16_t& init_sp);

    void traverseSceneGraph(const uint8_t* stack_data, size_t stack_size);

    void processNode(const uint8_t* data, size_t offset, size_t data_size,
                     int32_t pos_x, int32_t pos_y, int32_t pos_z,
                     int depth);

    static std::string synthesizeObjectName(uint16_t obj_id);

    void extractAnchorPoints(const uint8_t* decompressed_data, size_t decompressed_size);

    std::vector<Placement> placements;
    std::vector<AnchorPt> anchorPoints;
};

