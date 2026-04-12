// sod_decode.cpp - SOD decoder using siobj_lib
// Usage: ./sod_decode <res_file> [object_index] [max_objects]

#include "siobj_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cctype>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

// Export ParsedObject to OBJ file (copied from siobj_lib.cpp)
extern void export_obj(const ParsedObject& obj, const char* filename);

int main(int argc, char* argv[]) {
    const char* res_filename = "../FreeFlight/SIDATA/SCENERY1.RES";
    int object_index = -1;
    int max_objects = 20;

    if (argc >= 2) res_filename = argv[1];
    if (argc >= 3) object_index = atoi(argv[2]);
    if (argc >= 4) max_objects = atoi(argv[3]);

    if (argc == 1) {
        printf("Usage: %s <res_file> [object_index] [max_objects]\n", argv[0]);
        printf("  Example: %s ../FreeFlight/SIDATA/SCENERY1.RES -1 50\n", argv[0]);
        printf("  Example: %s ../FreeFlight/SIDATA/SCENERY1.RES 8\n", argv[0]);
        printf("\nWithout object_index, lists all objects in the file.\n");
        printf("Use max_objects=0 to just show the count without parsing.\n");
        return 0;
    }

    printf("Loading %s...\n", res_filename);

    // Quick count first
    int num = sod_get_res_count(res_filename);
    if (num < 0) {
        printf("Failed to read %s (invalid RES file or file not found)\n", res_filename);
        return 1;
    }
    printf("RES file contains %d objects\n", num);

    // If max_objects is 0, just show the count
    if (argc >= 4 && max_objects == 0) {
        return 0;
    }

    std::vector<RESEntry> entries;
    std::vector<uint8_t> data;
    num = sod_load_res(res_filename, entries, data);

    if (num < 0) {
        printf("Failed to load %s\n", res_filename);
        return 1;
    }

    // Extract basename from res_filename (without path and extension)
    char basename[64];
    strncpy(basename, strrchr(res_filename, '/') ? strrchr(res_filename, '/') + 1 : res_filename, sizeof(basename) - 1);
    basename[sizeof(basename) - 1] = '\0';
    char* dot = strrchr(basename, '.');
    if (dot) *dot = '\0';
    // Convert to lowercase
    for (char* p = basename; *p; p++) *p = tolower(*p);

    int id = entries[0].id & 0xFF00;  // Get base ID
    printf("File loaded: %d entries (ID range: 0x%04X - 0x%04X)\n", num, id, id + num - 1);

    int start_idx = (object_index >= 0) ? object_index : 0;
    int end_idx = (object_index >= 0) ? object_index + 1 : (max_objects < num ? max_objects : num);

    for (int idx = start_idx; idx < end_idx; idx++) {
        if (idx >= num) break;

        printf("=== Parsing object %d (ID 0x%04X) at offset %x ===\n", idx, entries[idx].id, entries[idx].offset);

        // Parse object
        ParsedObject obj = sod_parse_object(&data[entries[idx].offset], data.size() - entries[idx].offset);

        if (obj.vertices.size() > 0 && obj.faces.size() > 0) {
            char obj_filename[256];
            // Format: basename_index_offset.obj (e.g., planes_000_00000484.obj)
            sprintf(obj_filename, "%s_%03d_%08X.obj", basename, idx, entries[idx].offset);

            export_obj(obj, obj_filename);
            sod_export_mtl(obj, obj_filename);

            printf("Exported %zu vertices, %zu faces\n", obj.vertices.size(), obj.faces.size());

            if (obj.approx_spheres > 0 || obj.approx_disks > 0) {
                printf("Approximated primitives: %d spheres, %d disks\n",
                       obj.approx_spheres, obj.approx_disks);
            }
            if (obj.skipped_spheres > 0 || obj.skipped_blocks > 0 ||
                obj.skipped_disks > 0 || obj.skipped_dots > 0) {
                printf("Skipped primitives: %d spheres, %d blocks, %d disks, %d dots\n",
                       obj.skipped_spheres, obj.skipped_blocks,
                       obj.skipped_disks, obj.skipped_dots);
            }

            printf("OBJ: %s\n", obj_filename);
            printf("MTL: %s.mtl\n", obj_filename);

            if (object_index < 0 && idx < 10) {
                printf("[Object 0x%04X: %zu verts, %zu faces", entries[idx].id, obj.vertices.size(), obj.faces.size());
                if (obj.approx_spheres > 0 || obj.approx_disks > 0) {
                    printf(" ~%ds%dd", obj.approx_spheres, obj.approx_disks);
                }
                if (obj.skipped_spheres > 0 || obj.skipped_blocks > 0 ||
                    obj.skipped_disks > 0 || obj.skipped_dots > 0) {
                    printf(" -%ds%db%dd%do", obj.skipped_spheres, obj.skipped_blocks,
                             obj.skipped_disks, obj.skipped_dots);
                }
                printf("]\n");
            }
        } else {
            printf("Failed to parse or no geometry\n");
        }

        if (object_index >= 0) break;
    }

    return 0;
}
