#pragma once

#include "island.h"
#include "raylib.h"
#include "scenegraph_loader.h"
#include <cstdint>
#include <string>
#include <vector>

// Maximum number of unique object types
constexpr int kMaxAssets = 3000;

// Distance culling thresholds
constexpr float MIN_SCREEN_SIZE_PIXELS = 1.0f;
constexpr float MAX_DRAW_DISTANCE = 30000.0f * kIslandScale;

struct Asset
{
    Asset()
        : resFile(0)
        , objIdx(0)
        , bounds{0}
        , maxDimension(0)
        , verticalOffset(0)
    {
        mesh = {};
        material = {};
    }

    // Non-copyable due to Mesh and Material
    Asset(const Asset&) = delete;
    Asset& operator=(const Asset&) = delete;

    // Movable
    Asset(Asset&& other) noexcept
        : mesh(other.mesh)
        , material(other.material)
        , resFile(other.resFile)
        , objIdx(other.objIdx)
        , transforms(std::move(other.transforms))
        , bounds(other.bounds)
        , maxDimension(other.maxDimension)
        , verticalOffset(other.verticalOffset)
        , name(other.name)
    {
        other.mesh = {};
        other.material = {};
    }

    Asset& operator=(Asset&& other) noexcept
    {
        if (this != &other) {
            // Cleanup current resources
            if (mesh.vertexCount > 0)
                UnloadMesh(mesh);
            if (material.maps)
                UnloadMaterial(material);

            mesh = other.mesh;
            material = other.material;
            resFile = other.resFile;
            objIdx = other.objIdx;
            transforms = std::move(other.transforms);
            bounds = other.bounds;
            maxDimension = other.maxDimension;
            verticalOffset = other.verticalOffset;
            name = other.name;

            other.mesh = {};
            other.material = {};
        }
        return *this;
    }

    Mesh mesh;
    Material material;
    uint16_t resFile;
    uint16_t objIdx;
    std::vector<Matrix> transforms; // One transform per instance
    BoundingBox bounds;             // Bounding box in local space
    float maxDimension;             // Maximum dimension for size culling
    float verticalOffset;           // Y offset to place bottom at Y=0
    std::string name;

};

// Scene manager - handles all scene objects and placements
class Scene
{
public:
    Scene();
    ~Scene();

    // Non-copyable
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Load scenery placements from executable
    bool loadFromExecutable(const char* exe_path, const char* sidata_dir);

    // Cleanup all scene resources (call before CloseWindow())
    void cleanup();

    // Get all unique objects
    std::vector<Asset>& getAssets() { return uniqueObjects; }
    const std::vector<Asset>& getAssets() const { return uniqueObjects; }

    // Get anchor points extracted from executable
    const std::vector<AnchorPt>& getAnchorPoints() const { return loaderAnchorPoints; }

    // Get object name from res file and obj index
    static const char* GetObjectName(uint16_t resFile, uint16_t objIdx);

    // Statistics
    size_t getTotalPlacements() const;
    size_t getAssetCount() const { return uniqueObjects.size(); }

private:
    std::vector<Asset> uniqueObjects;
    std::vector<AnchorPt> loaderAnchorPoints;
};

// Culling utilities
// Calculate bounding box from mesh vertices
BoundingBox CalculateMeshBounds(const Mesh* mesh);

// Check if bounding box is inside view frustum
bool IsBoxInFrustum(const BoundingBox* box, const Matrix* viewProjection);

// Calculate screen-space size of object
float CalculateScreenSize(const BoundingBox* box,
                          const Vector3* position,
                          float maxDim,
                          const Matrix* viewProjection,
                          int screenHeight,
                          const Vector3* cameraPos);

