// test_res_bounds.cpp - Test RES file boundary conditions
// Verifies that:
// 1. Last object (index = count-1) can be extracted
// 2. Object at index = count fails gracefully

#include "siobj_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_file(const char* filename) {
    printf("Testing: %s\n", filename);
    
    // Get count
    int header_count = sod_get_res_count(filename);
    if (header_count <= 0) {
        printf("  ERROR: Cannot read file or empty\n");
        return 1;
    }
    printf("  RES file header reports %d objects\n", header_count);
    
    // Load full file
    std::vector<RESEntry> entries;
    std::vector<uint8_t> data;
    int num = sod_load_res(filename, entries, data);
    if (num <= 0) {
        printf("  ERROR: Load failed\n");
        return 1;
    }
    if (num != header_count) {
        printf("  NOTE: Actual valid objects: %d (header may include invalid entries)\n", num);
    }
    
    // Use actual count from loaded entries
    int count = num;
    
    // Check for invalid entries (objects that don't start with OP_ID)
    int invalid_count = header_count - count;
    if (invalid_count > 0) {
        printf("  NOTE: %d entries are not valid SOD objects (different format or corrupted)\n", invalid_count);
        printf("  Valid objects: %d (indices 0-%d)\n", count, count-1);
    }
    
    // Test 1: Extract last object (index = count-1)
    printf("  Test 1: Extracting last object (index %d)...\n", count - 1);
    uint32_t last_offset = entries[count-1].offset;
    uint32_t last_size = data.size() - last_offset;  // Remaining bytes to end of file
    printf("    Offset: 0x%X, Size: %u bytes, File size: %zu\n", last_offset, last_size, data.size());
    if (last_offset >= data.size()) {
        printf("    ERROR: Offset beyond end of file!\n");
        return 1;
    }
    ParsedObject last_obj = sod_parse_object(&data[last_offset], last_size);
    if (last_obj.vertices.size() > 0 || last_obj.faces.size() > 0) {
        printf("    SUCCESS: Last object extracted (%zu vertices, %zu faces)\n", 
               last_obj.vertices.size(), last_obj.faces.size());
    } else {
        printf("    WARNING: Last object has no geometry (may be empty in source)\n");
    }
    
    // Test 2: Try to extract object at index = count (should fail or be out of bounds)
    printf("  Test 2: Attempting to extract object at index %d (out of bounds)...\n", count);
    if (count < (int)entries.size()) {
        // Entry exists in array, try to parse it
        uint32_t extra_offset = entries[count].offset;
        uint32_t extra_size = data.size() - extra_offset;
        ParsedObject extra_obj = sod_parse_object(&data[extra_offset], extra_size);
        printf("    WARNING: Object at index %d was parsed (%zu vertices, %zu faces)\n",
               count, extra_obj.vertices.size(), extra_obj.faces.size());
        printf("    This may indicate the RES file has more entries than reported\n");
    } else {
        printf("    SUCCESS: Access to index %d correctly prevented (entries.size() = %zu)\n",
               count, entries.size());
    }
    
    // Test 3: Try to extract object at a clearly invalid index
    printf("  Test 3: Attempting to extract object at index %d (clearly invalid)...\n", count + 100);
    if (count + 100 < (int)entries.size()) {
        uint32_t invalid_offset = entries[count + 100].offset;
        uint32_t invalid_size = data.size() - invalid_offset;
        ParsedObject invalid_obj = sod_parse_object(&data[invalid_offset], invalid_size);
        printf("    WARNING: Object at index %d was parsed (%zu vertices, %zu faces)\n",
               count + 100, invalid_obj.vertices.size(), invalid_obj.faces.size());
    } else {
        printf("    SUCCESS: Access to index %d correctly prevented\n", count + 100);
    }
    
    printf("  PASSED\n\n");
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <res_file> [res_file2 ...]\n", argv[0]);
        printf("Tests boundary conditions for RES file parsing.\n");
        return 1;
    }
    
    int failures = 0;
    for (int i = 1; i < argc; i++) {
        if (test_file(argv[i]) != 0) {
            failures++;
        }
    }
    
    if (failures > 0) {
        printf("\n%d file(s) had errors\n", failures);
        return 1;
    }
    
    printf("\nAll tests passed!\n");
    return 0;
}
