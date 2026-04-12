#include "selection.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>

// ============================================================================
// Selection implementation
// ============================================================================

Selection::Selection()
    : selectedObject(nullptr)
    , selectedInstanceIndex(0)
    , hitPoint{0, 0, 0}
    , worldPos{0, 0, 0}
{}

bool Selection::RayBoxIntersect(const Vector3& rayOrigin,
                                      const Vector3& rayDir,
                                      const BoundingBox* box,
                                      float* outT)
{
    float tmin = (box->min.x - rayOrigin.x) / rayDir.x;
    float tmax = (box->max.x - rayOrigin.x) / rayDir.x;

    if (tmin > tmax) {
        float tmp = tmin;
        tmin = tmax;
        tmax = tmp;
    }

    float tymin = (box->min.y - rayOrigin.y) / rayDir.y;
    float tymax = (box->max.y - rayOrigin.y) / rayDir.y;

    if (tymin > tymax) {
        float tmp = tymin;
        tymin = tymax;
        tymax = tmp;
    }

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (box->min.z - rayOrigin.z) / rayDir.z;
    float tzmax = (box->max.z - rayOrigin.z) / rayDir.z;

    if (tzmin > tzmax) {
        float tmp = tzmin;
        tzmin = tzmax;
        tzmax = tmp;
    }

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    if (tmin < 0) {
        *outT = tmax;
        return tmax >= 0;
    }
    *outT = tmin;
    return true;
}

void Selection::pickObject(
        const Camera3D& camera,
        const Scene& scene)
{
    Vector2 clickPos = GetMousePosition();
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    if (IsCursorHidden())
    {
        clickPos = Vector2{float(screenWidth/2), float(screenHeight/2)};
    }

    // Convert screen coordinates to normalized device coordinates
    float ndcX = (2.0f * clickPos.x) / screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * clickPos.y) / screenHeight;

    // Get projection matrix
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD,
                                          (float) screenWidth / screenHeight,
                                          0.01f,
                                          10000.0f);

    // Build view matrix from camera's actual position, target, and up
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    Matrix invViewProj = MatrixInvert(MatrixMultiply(view, projection));

    // Convert NDC to world space ray
    Vector4 nearPoint = {ndcX * invViewProj.m0 + ndcY * invViewProj.m4 + 0.0f * invViewProj.m8 +
                             1.0f * invViewProj.m12,
                         ndcX * invViewProj.m1 + ndcY * invViewProj.m5 + 0.0f * invViewProj.m9 +
                             1.0f * invViewProj.m13,
                         ndcX * invViewProj.m2 + ndcY * invViewProj.m6 + 0.0f * invViewProj.m10 +
                             1.0f * invViewProj.m14,
                         ndcX * invViewProj.m3 + ndcY * invViewProj.m7 + 0.0f * invViewProj.m11 +
                             1.0f * invViewProj.m15};
    Vector4 farPoint = {ndcX * invViewProj.m0 + ndcY * invViewProj.m4 + 1.0f * invViewProj.m8 +
                            1.0f * invViewProj.m12,
                        ndcX * invViewProj.m1 + ndcY * invViewProj.m5 + 1.0f * invViewProj.m9 +
                            1.0f * invViewProj.m13,
                        ndcX * invViewProj.m2 + ndcY * invViewProj.m6 + 1.0f * invViewProj.m10 +
                            1.0f * invViewProj.m14,
                        ndcX * invViewProj.m3 + ndcY * invViewProj.m7 + 1.0f * invViewProj.m11 +
                            1.0f * invViewProj.m15};

    Vector3 rayStartWorld = {nearPoint.x / nearPoint.w,
                             nearPoint.y / nearPoint.w,
                             nearPoint.z / nearPoint.w};
    Vector3 rayEndWorld = {farPoint.x / farPoint.w,
                           farPoint.y / farPoint.w,
                           farPoint.z / farPoint.w};
    Vector3 rayDir = Vector3Normalize(Vector3Subtract(rayEndWorld, rayStartWorld));

    // Use camera position as ray start
    Vector3 rayStart = camera.position;

    // Test against all objects
    int closestIdx = -1;
    size_t closestInst = 0;
    float closestT = 100000.0f;
    Vector3 closestHit = {0, 0, 0};
    Vector3 closestWorldPos = {0, 0, 0};

    const auto& uniqueObjects = scene.getAssets();

    for (size_t i = 0; i < uniqueObjects.size(); i++) {
        const Asset& obj = uniqueObjects[i];
        if (obj.mesh.vertexCount == 0)
            continue;

        // Test each instance of this object type
        for (size_t inst = 0; inst < obj.transforms.size(); inst++) {
            const Matrix& t = obj.transforms[inst];

            // Transform bounding box to world space
            BoundingBox worldBounds =
                {{obj.bounds.min.x + t.m12, obj.bounds.min.y + t.m13, obj.bounds.min.z + t.m14},
                 {obj.bounds.max.x + t.m12, obj.bounds.max.y + t.m13, obj.bounds.max.z + t.m14}};

            // Actually, the mesh is at 0
            worldBounds.max.y -= t.m13;
            worldBounds.min.y -= t.m13;

            float hitT;
            if (RayBoxIntersect(rayStart, rayDir, &worldBounds, &hitT)) {
                if (hitT < closestT) {
                    closestT = hitT;
                    closestIdx = (int) i;
                    closestInst = inst;
                    closestHit = Vector3Add(rayStart, Vector3Scale(rayDir, hitT));
                    closestWorldPos = {t.m12, t.m13, t.m14};
                }
            }
        }
    }

    if (closestIdx >= 0) {
        hitPoint = closestHit;
        worldPos = closestWorldPos;
        selectedObject = &uniqueObjects.at(closestIdx);
        selectedInstanceIndex = closestInst;
    }
}
