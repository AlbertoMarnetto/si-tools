#pragma once

#include "raylib.h"
#include "scene.h"
#include <cstddef>

// Selection system - handles object picking and selection state
class Selection
{
public:
    Selection();
    ~Selection() = default;

    // Pick object at screen coordinates
    void pickObject(const Camera3D& camera,
                   const Scene& scene);

    Asset const* getSelectedObject() const { return selectedObject; }

    size_t getSelectedInstanceIndex() const { return selectedInstanceIndex; }

    Vector3 getHitPoint() const { return hitPoint; }

    Vector3 getWorldPosition() const { return worldPos; }

    // Clear selection
    void clear()
    {
        selectedObject = nullptr;
        selectedInstanceIndex = 0;
        hitPoint = {0, 0, 0};
        worldPos = {0, 0, 0};
    }

private:
    Asset const* selectedObject;
    size_t selectedInstanceIndex;
    Vector3 hitPoint;
    Vector3 worldPos;

    // Ray-AABB intersection test
    static bool RayBoxIntersect(const Vector3& rayOrigin,
                                const Vector3& rayDir,
                                const BoundingBox* box,
                                float* outT);
};


