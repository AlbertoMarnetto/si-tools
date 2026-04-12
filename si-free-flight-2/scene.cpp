#include "scene.h"

#include "island.h"
#include "scenegraph_loader.h"
#include "siobj_lib.h"
#include "write_debug.h"

#include "raymath.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>

// ============================================================================
// Culling utilities implementation
// ============================================================================

BoundingBox CalculateMeshBounds(const Mesh* mesh)
{
    BoundingBox bounds = {0};
    if (mesh->vertexCount == 0 || mesh->vertices == nullptr)
        return bounds;

    bounds.min = {mesh->vertices[0], mesh->vertices[1], mesh->vertices[2]};
    bounds.max = bounds.min;

    for (size_t i = 1; i < mesh->vertexCount; i++) {
        Vector3 v = {mesh->vertices[i * 3 + 0],
                     mesh->vertices[i * 3 + 1],
                     mesh->vertices[i * 3 + 2]};
        if (v.x < bounds.min.x)
            bounds.min.x = v.x;
        if (v.y < bounds.min.y)
            bounds.min.y = v.y;
        if (v.z < bounds.min.z)
            bounds.min.z = v.z;
        if (v.x > bounds.max.x)
            bounds.max.x = v.x;
        if (v.y > bounds.max.y)
            bounds.max.y = v.y;
        if (v.z > bounds.max.z)
            bounds.max.z = v.z;
    }
    return bounds;
}

bool IsBoxInFrustum(const BoundingBox* box, const Matrix* viewProjection)
{
    // Extract frustum planes from view-projection matrix
    // Plane format: ax + by + cz + dw = 0
    struct Plane
    {
        float x, y, z, w;
    };
    Plane frustum[6];

    // Left plane
    frustum[0].x = viewProjection->m3 + viewProjection->m0;
    frustum[0].y = viewProjection->m7 + viewProjection->m4;
    frustum[0].z = viewProjection->m11 + viewProjection->m8;
    frustum[0].w = viewProjection->m15 + viewProjection->m12;

    // Right plane
    frustum[1].x = viewProjection->m3 - viewProjection->m0;
    frustum[1].y = viewProjection->m7 - viewProjection->m4;
    frustum[1].z = viewProjection->m11 - viewProjection->m8;
    frustum[1].w = viewProjection->m15 - viewProjection->m12;

    // Bottom plane
    frustum[2].x = viewProjection->m3 + viewProjection->m1;
    frustum[2].y = viewProjection->m7 + viewProjection->m5;
    frustum[2].z = viewProjection->m11 + viewProjection->m9;
    frustum[2].w = viewProjection->m15 + viewProjection->m13;

    // Top plane
    frustum[3].x = viewProjection->m3 - viewProjection->m1;
    frustum[3].y = viewProjection->m7 - viewProjection->m5;
    frustum[3].z = viewProjection->m11 - viewProjection->m9;
    frustum[3].w = viewProjection->m15 - viewProjection->m13;

    // Near plane
    frustum[4].x = viewProjection->m3 + viewProjection->m2;
    frustum[4].y = viewProjection->m7 + viewProjection->m6;
    frustum[4].z = viewProjection->m11 + viewProjection->m10;
    frustum[4].w = viewProjection->m15 + viewProjection->m14;

    // Far plane
    frustum[5].x = viewProjection->m3 - viewProjection->m2;
    frustum[5].y = viewProjection->m7 - viewProjection->m6;
    frustum[5].z = viewProjection->m11 - viewProjection->m10;
    frustum[5].w = viewProjection->m15 - viewProjection->m14;

    // Test box against each plane
    for (int p = 0; p < 6; p++) {
        // Find the point of the box that is most in the direction of the plane
        // normal
        Vector3 positive = {frustum[p].x > 0 ? box->max.x : box->min.x,
                            frustum[p].y > 0 ? box->max.y : box->min.y,
                            frustum[p].z > 0 ? box->max.z : box->min.z};

        // If the positive point is outside the plane, the box is outside
        float dist = frustum[p].x * positive.x + frustum[p].y * positive.y +
            frustum[p].z * positive.z + frustum[p].w;
        if (dist < 0)
            return false;
    }
    return true;
}

float CalculateScreenSize(const BoundingBox* box,
                          const Vector3* position,
                          float maxDim,
                          const Matrix* viewProjection,
                          int screenHeight,
                          const Vector3* cameraPos)
{
    // The bounding box is already in world space, so just compute its center
    Vector3 center = {(box->min.x + box->max.x) * 0.5f,
                      (box->min.y + box->max.y) * 0.5f,
                      (box->min.z + box->max.z) * 0.5f};

    // Compute distance from camera position
    float dx = center.x - cameraPos->x;
    float dy = center.y - cameraPos->y;
    float dz = center.z - cameraPos->z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    if (dist > MAX_DRAW_DISTANCE)
        return 0;
    if (dist < 1.0f)
        dist = 1.0f; // Avoid division by zero

    // Approximate screen size: (object_size / distance) * projection_scale *
    // screen_height For 60-degree FOV, projection scale is roughly 1.0
    float screenPixels = (maxDim * screenHeight) / (dist * 1.5f);
    return screenPixels;
}


// ============================================================================
// Scene implementation
// ============================================================================

Scene::Scene() {}

Scene::~Scene()
{
    // Assets clean themselves up via move semantics
}

void Scene::cleanup()
{
    for (auto& obj : uniqueObjects) {
        if (obj.mesh.vertexCount > 0) {
            UnloadMesh(obj.mesh);
            obj.mesh = {};
        }
        if (obj.material.maps) {
            UnloadMaterial(obj.material);
            obj.material = {};
        }
    }
    uniqueObjects.clear();
}

const char* Scene::GetObjectName(uint16_t resFile, uint16_t objIdx)
{
    static char nameBuf[64];
    snprintf(nameBuf, sizeof(nameBuf), "Object R%d_I%02X", resFile, objIdx);
    return nameBuf;
}

size_t Scene::getTotalPlacements() const
{
    size_t total = 0;
    for (const auto& obj : uniqueObjects) {
        total += obj.transforms.size();
    }
    return total;
}

bool Scene::loadFromExecutable(const char* exe_path, const char* sidata_dir)
{
    write_debug("\nLoading scene graph from executable: %s\n", exe_path);

    // Extract placements from executable
    SceneGraphLoader loader;
    if (!loader.loadFromExecutable(exe_path)) {
        return false;
    }

    const auto& placements = loader.getPlacements();
    write_debug("Extracted %zu placements with %zu unique object types\n",
           placements.size(), loader.getUniqueObjectCount());

    // Store anchor points from loader
    loaderAnchorPoints = loader.getAnchorPoints();

    // Load all SCENERY RES files
    std::vector<RESEntry> res_entries[7]; // Index 1-6 for SCENERY1-6.RES
    std::vector<uint8_t> res_data[7];
    const char* res_files[] = {
        nullptr,               // 0 unused
        "RES/SCENERY1.RES", // 1
        "RES/SCENERY2.RES", // 2
        "RES/SCENERY3.RES", // 3
        "RES/SCENERY4.RES", // 4
        "RES/SCENERY5.RES", // 5
        "RES/SCENERY6.RES", // 6
    };

    // Build full paths
    char fullPath[256];
    for (int i = 1; i <= 6; i++) {
        sod_load_res(res_files[i], res_entries[i], res_data[i]);
        write_debug("  %s: %zu objects\n", fullPath, res_entries[i].size());
    }

    // Create unique object entries for each unique object ID
    std::set<std::pair<int, int>> seenObjects;
    for (const auto& p : placements) {
        // Determine RES file from object ID
        int res_idx = (p.obj_id >> 8) - 1;  // 0x02xx -> RES 1, etc.
        if (res_idx < 1 || res_idx > 6) continue;

        int obj_idx = (p.obj_id - 1) & 0xFF;
        if (obj_idx < 0 || obj_idx >= (int)res_entries[res_idx].size()) continue;

        auto key = std::make_pair(res_idx, obj_idx);
        if (seenObjects.find(key) == seenObjects.end()) {
            seenObjects.insert(key);
            uniqueObjects.push_back({});
            Asset& newObj = uniqueObjects.back();
            newObj.resFile = (uint16_t)res_idx;
            newObj.objIdx = (uint16_t)obj_idx;
            newObj.material = LoadMaterialDefault();
            newObj.name = p.name;
        }
    }

    write_debug("Created %zu unique objects, building meshes...\n", uniqueObjects.size());

    // Build meshes for each unique object type
    int builtCount = 0;
    for (size_t i = 0; i < uniqueObjects.size(); i++) {
        int res_idx = uniqueObjects[i].resFile;
        int obj_idx = uniqueObjects[i].objIdx;

        uint32_t offset = res_entries[res_idx][obj_idx].offset;
        ParsedObject sod_obj = sod_parse_object(&res_data[res_idx][offset],
                                                res_data[res_idx].size() - offset);

        if (sod_obj.vertices.size() > 0 && sod_obj.faces.size() > 0) {
            // Convert SOD to mesh
            Mesh mesh = {0};
            mesh.vertexCount = sod_obj.vertices.size();

            size_t total_tris = 0;
            for (size_t i = 0; i < sod_obj.faces.size(); i++) {
                int num_verts = sod_obj.faces[i].size();
                if (num_verts >= 3) {
                    total_tris += (num_verts - 2);
                }
            }
            mesh.triangleCount = total_tris;

            mesh.vertices = (float*) RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
            mesh.colors = (unsigned char*) RL_MALLOC(mesh.vertexCount * 4 * sizeof(unsigned char));
            mesh.normals = (float*) RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
            mesh.indices = (uint16_t*) RL_MALLOC(total_tris * 3 * sizeof(uint16_t));

            float scale = sod_obj.scale * kIslandScale;

            for (size_t i = 0; i < sod_obj.vertices.size(); i++) {
                mesh.vertices[i * 3 + 0] = (float) sod_obj.vertices[i].x * scale;
                mesh.vertices[i * 3 + 1] = (float) sod_obj.vertices[i].y * scale;
                mesh.vertices[i * 3 + 2] = (float) sod_obj.vertices[i].z * scale;
            }

            size_t index_offset = 0;
            for (size_t i = 0; i < sod_obj.faces.size(); i++) {
                int color_idx = sod_obj.face_colors[i];
                if (color_idx < 0)
                    color_idx = 0;
                if (color_idx > 255)
                    color_idx = 255;

                unsigned char r = sipal[color_idx * 3 + 0];
                unsigned char g = sipal[color_idx * 3 + 1];
                unsigned char b = sipal[color_idx * 3 + 2];

                int num_verts = sod_obj.faces[i].size();
                if (num_verts < 3)
                    continue;

                for (int j = 1; j < num_verts - 1; j++) {
                    int v0 = sod_obj.faces[i][0];
                    int v1 = sod_obj.faces[i][j];
                    int v2 = sod_obj.faces[i][j + 1];

                    for (int k = 0; k < 3; k++) {
                        int vid = (k == 0) ? v0 : (k == 1) ? v1 : v2;
                        if (vid >= 0 && vid < (int) sod_obj.vertices.size()) {
                            mesh.colors[vid * 4 + 0] = r;
                            mesh.colors[vid * 4 + 1] = g;
                            mesh.colors[vid * 4 + 2] = b;
                            mesh.colors[vid * 4 + 3] = 255;
                        }
                    }

                    mesh.indices[index_offset++] = v0;
                    mesh.indices[index_offset++] = v2;
                    mesh.indices[index_offset++] = v1;
                }
            }

            // Calculate normals
            for (size_t i = 0; i < index_offset; i += 3) {
                int v0 = mesh.indices[i];
                int v1 = mesh.indices[i + 1];
                int v2 = mesh.indices[i + 2];

                if (v0 < 0 || v1 < 0 || v2 < 0 || v0 >= (int) mesh.vertexCount ||
                    v1 >= (int) mesh.vertexCount || v2 >= (int) mesh.vertexCount) {
                    continue;
                }

                float ax = mesh.vertices[v1 * 3 + 0] - mesh.vertices[v0 * 3 + 0];
                float ay = mesh.vertices[v1 * 3 + 1] - mesh.vertices[v0 * 3 + 1];
                float az = mesh.vertices[v1 * 3 + 2] - mesh.vertices[v0 * 3 + 2];

                float bx = mesh.vertices[v2 * 3 + 0] - mesh.vertices[v0 * 3 + 0];
                float by = mesh.vertices[v2 * 3 + 1] - mesh.vertices[v0 * 3 + 1];
                float bz = mesh.vertices[v2 * 3 + 2] - mesh.vertices[v0 * 3 + 2];

                float nx = ay * bz - az * by;
                float ny = az * bx - ax * bz;
                float nz = ax * by - ay * bx;

                float len = sqrtf(nx * nx + ny * ny + nz * nz);
                if (len > 0.0001f) {
                    nx /= len;
                    ny /= len;
                    nz /= len;
                }

                mesh.normals[v0 * 3 + 0] += nx;
                mesh.normals[v0 * 3 + 1] += ny;
                mesh.normals[v0 * 3 + 2] += nz;
                mesh.normals[v1 * 3 + 0] += nx;
                mesh.normals[v1 * 3 + 1] += ny;
                mesh.normals[v1 * 3 + 2] += nz;
                mesh.normals[v2 * 3 + 0] += nx;
                mesh.normals[v2 * 3 + 1] += ny;
                mesh.normals[v2 * 3 + 2] += nz;
            }

            for (size_t i = 0; i < sod_obj.vertices.size(); i++) {
                float nx = mesh.normals[i * 3 + 0];
                float ny = mesh.normals[i * 3 + 1];
                float nz = mesh.normals[i * 3 + 2];

                float len = sqrtf(nx * nx + ny * ny + nz * nz);
                if (len > 0.0001f) {
                    mesh.normals[i * 3 + 0] = nx / len;
                    mesh.normals[i * 3 + 1] = ny / len;
                    mesh.normals[i * 3 + 2] = nz / len;
                } else {
                    mesh.normals[i * 3 + 0] = 0;
                    mesh.normals[i * 3 + 1] = 1;
                    mesh.normals[i * 3 + 2] = 0;
                }
            }

            UploadMesh(&mesh, false);
            uniqueObjects[i].mesh = mesh;

            // Calculate bounding box
            uniqueObjects[i].bounds = CalculateMeshBounds(&uniqueObjects[i].mesh);

            // Shift mesh vertically so bottom is at Y=0
            float yOffset = -uniqueObjects[i].bounds.min.y;
            for (size_t v = 0; v < uniqueObjects[i].mesh.vertexCount; v++) {
                uniqueObjects[i].mesh.vertices[v * 3 + 1] += yOffset;
            }

            uniqueObjects[i].bounds.min.y += yOffset;
            uniqueObjects[i].bounds.max.y += yOffset;
            uniqueObjects[i].verticalOffset = yOffset;

            Vector3 size = {uniqueObjects[i].bounds.max.x - uniqueObjects[i].bounds.min.x,
                            uniqueObjects[i].bounds.max.y - uniqueObjects[i].bounds.min.y,
                            uniqueObjects[i].bounds.max.z - uniqueObjects[i].bounds.min.z};
            uniqueObjects[i].maxDimension = fmaxf(size.x, fmaxf(size.y, size.z));

            builtCount++;
        }

        if (builtCount % 100 == 0) {
            write_debug("  Built %d/%zu unique meshes...\n", builtCount, uniqueObjects.size());
        }
    }

    write_debug("Built %zu unique meshes, creating transforms...\n", uniqueObjects.size());

    // Create transforms from placements
    int totalPlacements = 0;
    for (const auto& p : placements) {
        int res_idx = (p.obj_id >> 8) - 1;
        if (res_idx < 1 || res_idx > 6) continue;

        int obj_idx = (p.obj_id - 1) & 0xFF;

        // Find the unique object index
        int typeIdx = -1;
        for (size_t i = 0; i < uniqueObjects.size(); i++) {
            if (uniqueObjects[i].resFile == (uint16_t)res_idx &&
                uniqueObjects[i].objIdx == (uint16_t)obj_idx) {
                typeIdx = (int)i;
                break;
            }
        }
        if (typeIdx < 0) continue;

        float baseX = -((float)p.x * kIslandScale);
        float baseY = -((float)p.y * kIslandScale);
        float baseZ = ((float)p.z * kIslandScale);

        float finalY = baseY + uniqueObjects[typeIdx].verticalOffset;

        Matrix transform = MatrixTranslate(baseX, finalY, baseZ);
        uniqueObjects[typeIdx].transforms.push_back(transform);
        totalPlacements++;
    }

    write_debug("Created %d transforms from %zu raw placements\n\n",
           totalPlacements, placements.size());

    return true;
}
