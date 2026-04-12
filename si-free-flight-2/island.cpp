#include "island.h"

#include "buffer.h"
#include "terrain_mapbin.h"
#include "write_debug.h"

#include "siobj_lib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

constexpr int kMaxVertices = 10000;
constexpr int kMaxIndices = 30000;

// Calculate triangle normal
static void calc_normal(const float* v0, const float* v1, const float* v2, float* out)
{
    float ax = v1[0] - v0[0], ay = v1[1] - v0[1], az = v1[2] - v0[2];
    float bx = v2[0] - v0[0], by = v2[1] - v0[1], bz = v2[2] - v0[2];
    out[0] = ay * bz - az * by;
    out[1] = az * bx - ax * bz;
    out[2] = ax * by - ay * bx;
    float len = sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
    if (len > 0.0001f) {
        out[0] /= len;
        out[1] /= len;
        out[2] /= len;
    }
}

// Load island terrain from memory buffers
// Returns true on success, false on failure
static bool do_load_from_memory(
    std::vector<AnchorPt> const& anchorpts,
    std::vector<uint8_t> const& map_areas_buffer,
    IslandMesh* mesh)
{
    int i, j;

    int num_map_areas = map_areas_buffer.size() / sizeof(MapArea);

    // Temporary storage for vertices and indices
    std::vector<float> temp_verts(kMaxVertices * 3);
    std::vector<float> temp_colors(kMaxVertices * 3);
    std::vector<float> temp_normals(kMaxVertices * 3, 0.0f); // Zero-initialized
    std::vector<int> temp_indices(kMaxIndices);

    int vertex_count = 0;
    int index_count = 0;

    // First, add water plane (large quad at y=-1)
    // Use the S.I. water color (RGB: (16, 72, 120), determined using color
    // picker)
    float water_r = 16.0f / 255.0f;
    float water_g = 72.0f / 255.0f;
    float water_b = 120.0f / 255.0f;

    // Water vertices (counter-clockwise for proper winding)
    float water_verts[][3] = {
        {-32000.0f * kIslandScale, -50.0f * kIslandScale, 32000.0f * kIslandScale},
        {32000.0f * kIslandScale, -50.0f * kIslandScale, 32000.0f * kIslandScale},
        {32000.0f * kIslandScale, -50.0f * kIslandScale, -32000.0f * kIslandScale},
        {-32000.0f * kIslandScale, -50.0f * kIslandScale, -32000.0f * kIslandScale},
    };

    for (i = 0; i < 4; i++) {
        if (vertex_count < kMaxVertices) {
            temp_verts[vertex_count * 3 + 0] = water_verts[i][0];
            temp_verts[vertex_count * 3 + 1] = water_verts[i][1];
            temp_verts[vertex_count * 3 + 2] = water_verts[i][2];

            temp_colors[vertex_count * 3 + 0] = water_r;
            temp_colors[vertex_count * 3 + 1] = water_g;
            temp_colors[vertex_count * 3 + 2] = water_b;
            vertex_count++;
        }
    }

    // Water indices (two triangles for the quad)
    int water_base = vertex_count - 4;
    int water_indices[] = {
        water_base + 0,
        water_base + 1,
        water_base + 2,
        water_base + 0,
        water_base + 2,
        water_base + 3,
    };
    for (i = 0; i < 6; i++) {
        if (index_count < kMaxIndices) {
            temp_indices[index_count++] = water_indices[i];
        }
    }

    // Process map areas (terrain polygons)
    int total_areas = 0;
    int skipped_flag = 0;
    int skipped_invalid = 0;
    int rendered_areas = 0;

    // Track height and color distribution
    float min_height = 999999, max_height = -999999;
    int color_histogram[256] = {0};

    float global_y_offset = -5;
    for (i = 0; i < num_map_areas; i++) {
        MapArea map_area;
        std::memcpy(&map_area, &map_areas_buffer[i * sizeof(MapArea)], sizeof(MapArea));
        total_areas++;

        // Skip if position flag is not set
        if (!(map_area.pos & 0xff)) {
            skipped_flag++;
            continue;
        }

        int cur_color = map_area.color;
        if (cur_color < 0)
            cur_color = 0;
        if (cur_color >= 256)
            cur_color = 255;

        // Skip map area 65 - stray polygons
        // These polygons have invalid vertex indices that create a huge stray triangle
        if (i == 65 || i == 131)
        {
            skipped_invalid++;
            continue;
        }

        color_histogram[cur_color]++;

        float r = sipal[cur_color * 3 + 0] * (1.0f / 255.0f);
        float g = sipal[cur_color * 3 + 1] * (1.0f / 255.0f);
        float b = sipal[cur_color * 3 + 2] * (1.0f / 255.0f);

        // Validate vertices - relaxed check (allow all valid vertex indices)
        bool valid = true;
        float poly_avg_height = 0;
        for (j = 0; j < map_area.num_verts; j++) {
            int vidx = map_area.v[j];
            AnchorPt const& anchor_pt = anchorpts.at(vidx);

            if (vidx < 0 || vidx >= anchorpts.size()) {
                valid = false;
                break;
            }
            // Skip only extremely out-of-range vertices
            if (anchor_pt.y > 30000 || anchor_pt.y < -30000) {
                valid = false;
                break;
            }
            // Track height (negated and scaled)
            float h = -(float) (anchor_pt.y) * kIslandScale;
            poly_avg_height += h;
            if (h < min_height)
                min_height = h;
            if (h > max_height)
                max_height = h;

            //write_debug("@@@ %03d %03d : %d %d %d\n", i, j, anchor_pt.x, anchor_pt.y, anchor_pt.z);
        }

        if (!valid) {
            skipped_invalid++;
            continue;
        }

        rendered_areas++;

        // Check if this is a big horizontal polygon
        // Horizontal: all vertices have similar Y values (within 0.1 units)
        // Big: bounding box in XZ is larger than 50x50 units
        bool is_horizontal = true;
        float min_y = 999999, max_y = -999999;
        float min_x = 999999, max_x = -999999;
        float min_z = 999999, max_z = -999999;
        
        for (j = 0; j < map_area.num_verts; j++) {
            int vidx = map_area.v[j];
            AnchorPt const& anchor_pt = anchorpts.at(vidx);
            float y = -(float)(anchor_pt.y) * kIslandScale;
            float x = -(float)(anchor_pt.x) * kIslandScale;
            float z = (float)(anchor_pt.z) * kIslandScale;
            
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (z < min_z) min_z = z;
            if (z > max_z) max_z = z;
        }
        
        float y_variation = max_y - min_y;
        float xz_size = fmaxf(max_x - min_x, max_z - min_z);
        
        bool is_big_horizontal = (y_variation < 0.05f) && (xz_size > 8.0f);
        
        // Apply small offset to big horizontal polygons to prevent Z-fighting
        float y_offset = 0.0f;
        if (is_big_horizontal)
        {
            y_offset = global_y_offset;
            global_y_offset += 0.1;
        }

        // Store vertices for this polygon
        int poly_base = vertex_count;
        int num_verts = map_area.num_verts;
        if (num_verts > 7)
            num_verts = 7;

        for (j = 0; j < num_verts; j++) {
            int vidx = map_area.v[j];
            AnchorPt const& anchor_pt = anchorpts.at(vidx);
            if (vertex_count < kMaxVertices) {
                temp_verts[vertex_count * 3 + 0] = -(float) (anchor_pt.x) * kIslandScale;
                temp_verts[vertex_count * 3 + 1] = -(float) (anchor_pt.y) * kIslandScale + y_offset * kIslandScale;
                temp_verts[vertex_count * 3 + 2] = (float) (anchor_pt.z) * kIslandScale;

                temp_colors[vertex_count * 3 + 0] = r;
                temp_colors[vertex_count * 3 + 1] = g;
                temp_colors[vertex_count * 3 + 2] = b;
                vertex_count++;
            }
        }

        // Triangulate polygon (fan triangulation from first vertex)
        // For a polygon with vertices 0,1,2,3,4... create triangles:
        // (0,2,1), (0,3,2), (0,4,3)...  [reversed winding for correct culling]
        for (j = 1; j < num_verts - 1 && index_count + 3 <= kMaxIndices; j++) {
            int i0 = poly_base;
            int i1 = poly_base + j;
            int i2 = poly_base + j + 1;

            // Reverse winding order
            temp_indices[index_count++] = i0;
            temp_indices[index_count++] = i2;
            temp_indices[index_count++] = i1;

            // Compute normal for this triangle
            float normal[3];
            calc_normal(&temp_verts[i0 * 3], &temp_verts[i2 * 3], &temp_verts[i1 * 3], normal);

            // Accumulate normal for each vertex (for smooth shading)
            for (int k = 0; k < 3; k++) {
                temp_normals[i0 * 3 + k] += normal[k];
                temp_normals[i1 * 3 + k] += normal[k];
                temp_normals[i2 * 3 + k] += normal[k];
            }
        }
    }

    // Normalize all accumulated normals
    for (int i = 0; i < vertex_count; i++) {
        float len = sqrtf(temp_normals[i * 3] * temp_normals[i * 3] +
                          temp_normals[i * 3 + 1] * temp_normals[i * 3 + 1] +
                          temp_normals[i * 3 + 2] * temp_normals[i * 3 + 2]);
        if (len > 0.0001f) {
            temp_normals[i * 3] /= len;
            temp_normals[i * 3 + 1] /= len;
            temp_normals[i * 3 + 2] /= len;
        } else {
            // Default up vector for flat surfaces
            temp_normals[i * 3] = 0;
            temp_normals[i * 3 + 1] = 1;
            temp_normals[i * 3 + 2] = 0;
        }
    }

    // Ensure all normals point upward (positive Y) for terrain
    for (int i = 0; i < vertex_count; i++) {
        if (temp_normals[i * 3 + 1] < 0) {
            temp_normals[i * 3] = -temp_normals[i * 3];
            temp_normals[i * 3 + 1] = -temp_normals[i * 3 + 1];
            temp_normals[i * 3 + 2] = -temp_normals[i * 3 + 2];
        }
    }

    // Copy to mesh structure
    mesh->num_vertices = vertex_count;
    mesh->num_indices = index_count;
    mesh->num_triangles = index_count / 3;

    mesh->vertices.resize(vertex_count * 3);
    mesh->colors.resize(vertex_count * 3);
    mesh->normals.resize(vertex_count * 3);
    mesh->indices.resize(index_count);

    // Copy data from temporary vectors to mesh
    std::copy(temp_verts.begin(), temp_verts.begin() + vertex_count * 3, mesh->vertices.begin());
    std::copy(temp_colors.begin(), temp_colors.begin() + vertex_count * 3, mesh->colors.begin());
    std::copy(temp_normals.begin(), temp_normals.begin() + vertex_count * 3, mesh->normals.begin());
    std::copy(temp_indices.begin(), temp_indices.begin() + index_count, mesh->indices.begin());

    return true;
}

// ============================================================================

IslandResource::IslandResource()
    : loaded(false)
{
    mesh = {};
    raylibMesh = {};
    bounds = {};
}

IslandResource::~IslandResource()
{
    // Note: cleanup() should be called explicitly before CloseWindow()
    // This is a safety check to prevent double-free
    if (raylibMesh.vertexCount > 0) {
        UnloadMesh(raylibMesh);
    }
}

void IslandResource::cleanup()
{
    if (raylibMesh.vertexCount > 0) {
        UnloadMesh(raylibMesh);
        raylibMesh = {};
    }
}

IslandResource::IslandResource(IslandResource&& other) noexcept
    : loaded(other.loaded)
    , mesh(other.mesh)
    , raylibMesh(other.raylibMesh)
    , bounds(other.bounds)
{
    other.loaded = false;
    other.mesh = {};
    other.raylibMesh = {};
    other.bounds = {};
}

bool IslandResource::loadFromAnchorPoints(
    const std::vector<AnchorPt>& anchorpts)
{
    // Use embedded MAP.BIN data

    if (!do_load_from_memory(
            anchorpts,
            mapbin_data,
            &mesh)) {
        return false;
    }

    // Calculate bounding box
    float minX = mesh.vertices[0], maxX = mesh.vertices[0];
    float minY = mesh.vertices[1], maxY = mesh.vertices[1];
    float minZ = mesh.vertices[2], maxZ = mesh.vertices[2];
    for (int i = 1; i < mesh.num_vertices; i++) {
        float x = mesh.vertices[i * 3 + 0];
        float y = mesh.vertices[i * 3 + 1];
        float z = mesh.vertices[i * 3 + 2];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }
    bounds.min = {minX, minY, minZ};
    bounds.max = {maxX, maxY, maxZ};

    // Create raylib mesh
    raylibMesh.vertexCount = mesh.num_vertices;
    raylibMesh.triangleCount = mesh.num_triangles;
    raylibMesh.vertices = (float*) RL_MALLOC(mesh.num_vertices * 3 * sizeof(float));
    raylibMesh.colors = (unsigned char*) RL_MALLOC(mesh.num_vertices * 4 * sizeof(unsigned char));
    raylibMesh.normals = (float*) RL_MALLOC(mesh.num_vertices * 3 * sizeof(float));
    raylibMesh.indices = (uint16_t*) RL_MALLOC(mesh.num_indices * sizeof(uint16_t));

    for (int i = 0; i < mesh.num_vertices * 3; i++)
        raylibMesh.vertices[i] = mesh.vertices[i];
    for (int i = 0; i < mesh.num_vertices; i++) {
        raylibMesh.colors[i * 4 + 0] = (unsigned char)(mesh.colors[i * 3 + 0] * 255.0f);
        raylibMesh.colors[i * 4 + 1] = (unsigned char)(mesh.colors[i * 3 + 1] * 255.0f);
        raylibMesh.colors[i * 4 + 2] = (unsigned char)(mesh.colors[i * 3 + 2] * 255.0f);
        raylibMesh.colors[i * 4 + 3] = 255;
    }
    for (int i = 0; i < mesh.num_vertices * 3; i++)
        raylibMesh.normals[i] = mesh.normals[i];
    for (int i = 0; i < mesh.num_indices; i++)
        raylibMesh.indices[i] = (uint16_t)mesh.indices[i];

    UploadMesh(&raylibMesh, false);
    loaded = true;
    return true;
}
