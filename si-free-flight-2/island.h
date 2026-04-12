#pragma once

#include "raylib.h"
#include <cstdint>
#include <cstddef>
#include <vector>

// Scale down coordinates - original Stunt Island coords are huge (32000 units)
// Raylib works better with smaller coordinates for precision. A scale of 1.
// induces z-fighting
constexpr float kIslandScale = 1.0f / 10;

// Anchor point structure - only base coordinates needed for terrain mesh
struct AnchorPt
{
    int16_t x, y, z;  // Base coordinates (aptax, aptay, aptaz)
};

// Map area structure from MAP.BIN
// Note: This is a packed structure (no padding)
struct __attribute__((packed)) MapArea
{
    int16_t num;
    int16_t pos;
    int16_t a, b, c, d;
    int16_t color;
    int16_t num_verts;
    int16_t v[7]; // vertex indices (up to 7)
};

// Island mesh structure
struct IslandMesh
{
    std::vector<float> vertices;
    std::vector<float> colors;
    std::vector<float> normals;
    std::vector<int> indices;
    int num_vertices = 0;
    int num_indices = 0;
    int num_triangles = 0;
};

// RAII wrapper for island mesh resources
class IslandResource
{
public:
    IslandResource();
    ~IslandResource();

    // Non-copyable, movable
    IslandResource(const IslandResource&) = delete;
    IslandResource& operator=(const IslandResource&) = delete;
    IslandResource(IslandResource&& other) noexcept;
    IslandResource& operator=(IslandResource&& other) noexcept;

    bool loadFromAnchorPoints(const std::vector<AnchorPt>& anchorpts);
    bool isLoaded() const { return loaded; }

    // Explicit cleanup (call before CloseWindow())
    void cleanup();

    IslandMesh mesh;
    Mesh raylibMesh;
    BoundingBox bounds;

private:
    bool loaded;
};

