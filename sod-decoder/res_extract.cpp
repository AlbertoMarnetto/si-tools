// res_extract.cpp - Extract resources from RES files
// Usage: res_extract <res_file> [output_dir]
// Extracts all resources, saving TEXT as .txt and SOD as .sod

#include "siobj_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Load RES file without SOD validation (for text/binary resources)
int load_res_raw(const char* filename, std::vector<RESEntry>& entries, std::vector<uint8_t>& data) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;
    
    // Read header
    uint8_t header[8];
    if (fread(header, 1, 8, f) != 8) {
        fclose(f);
        return -1;
    }
    
    // Check signature
    if (header[0] != 'R' || header[1] != 'S') {
        fclose(f);
        return -1;
    }
    
    uint8_t id_high = header[2];
    uint8_t num = header[3];
    uint32_t offset_pos = (id_high > 0x0F) ? 8 : 4;
    uint16_t base_id = (id_high + 2) << 8;
    
    // Read entire file
    fseek(f, 0, SEEK_END);
    uint32_t file_size = ftell(f);
    fseek(f, offset_pos, SEEK_SET);
    
    data.resize(file_size - offset_pos);
    fread(data.data(), 1, file_size - offset_pos, f);
    fclose(f);
    
    // Read offsets
    for (int i = 0; i < num; i++) {
        uint32_t offset = *(uint32_t*)&data[i * 4];
        if (offset < file_size) {
            RESEntry entry;
            entry.id = base_id + i;
            entry.offset = offset - offset_pos;  // Adjust for our data array
            entries.push_back(entry);
        }
    }
    
    return entries.size();
}

// Check if data looks like text (printable ASCII with null terminator)
bool is_text_resource(const uint8_t* data, uint32_t size) {
    if (size < 2) return false;
    
    // Must end with null terminator
    if (data[size - 1] != 0) return false;
    
    // Count printable characters
    int printable = 0;
    int non_printable = 0;
    for (uint32_t i = 0; i < size - 1; i++) {
        uint8_t c = data[i];
        if (c >= 32 && c < 127) {
            printable++;
        } else if (c == '\n' || c == '\r' || c == '\t') {
            printable++;
        } else {
            non_printable++;
        }
    }
    
    // Text if >80% printable and <5% non-printable
    float total = (float)(printable + non_printable);
    if (total < 10) return false;  // Too short
    
    return ((float)printable / total) > 0.8f && ((float)non_printable / total) < 0.05f;
}

// Check if data is SOD (starts with OP_ID = 0x000C)
bool is_sod_resource(const uint8_t* data, uint32_t size) {
    if (size < 2) return false;
    uint16_t first_word = data[0] | (data[1] << 8);
    return (first_word == 0x000C);  // OP_ID
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <res_file> [output_dir]\n", argv[0]);
        printf("Extracts resources from RES files.\n");
        printf("  TEXT resources saved as .txt\n");
        printf("  SOD resources saved as .sod\n");
        printf("  Other resources saved as .bin\n");
        return 1;
    }
    
    const char* res_filename = argv[1];
    const char* output_dir = (argc >= 3) ? argv[2] : ".";
    
    // Create output directory if needed
    mkdir(output_dir, 0755);
    
    printf("Extracting resources from: %s\n", res_filename);
    printf("Output directory: %s\n\n", output_dir);
    
    // Load RES file (raw, no SOD validation)
    std::vector<RESEntry> entries;
    std::vector<uint8_t> data;
    int num = load_res_raw(res_filename, entries, data);

    if (num <= 0) {
        printf("Failed to load RES file\n");
        return 1;
    }

    printf("Found %d resources\n\n", num);
    
    int text_count = 0;
    int sod_count = 0;
    int bin_count = 0;
    int unknown_count = 0;
    
    for (int i = 0; i < num; i++) {
        uint32_t offset = entries[i].offset;
        uint16_t res_id = entries[i].id;
        
        // Determine size (next resource offset or end of file)
        uint32_t size;
        if (i + 1 < num) {
            size = entries[i + 1].offset - offset;
        } else {
            size = data.size() - offset;
        }
        
        if (offset >= data.size() || size == 0) {
            printf("[%03d] 0x%04X: INVALID (offset=0x%X, size=%u)\n", 
                   i, res_id, offset, size);
            continue;
        }
        
        const uint8_t* res_data = &data[offset];
        
        // Determine resource type
        char filename[256];
        const char* type_str;
        
        if (is_text_resource(res_data, size)) {
            type_str = "TEXT";
            snprintf(filename, sizeof(filename), "%s/res_%03d_%04X.txt", output_dir, i, res_id);
            text_count++;
        } else if (is_sod_resource(res_data, size)) {
            type_str = "SOD";
            snprintf(filename, sizeof(filename), "%s/res_%03d_%04X.sod", output_dir, i, res_id);
            sod_count++;
        } else {
            type_str = "BIN";
            snprintf(filename, sizeof(filename), "%s/res_%03d_%04X.bin", output_dir, i, res_id);
            bin_count++;
        }
        
        // Write resource to file
        FILE* f = fopen(filename, "wb");
        if (f) {
            fwrite(res_data, 1, size, f);
            fclose(f);
            printf("[%03d] 0x%04X: %s (%5u bytes) -> %s\n", 
                   i, res_id, type_str, size, filename);
        } else {
            printf("[%03d] 0x%04X: %s (%5u bytes) -> ERROR writing file\n", 
                   i, res_id, type_str, size);
            unknown_count++;
        }
    }
    
    printf("\n--- Summary ---\n");
    printf("TEXT resources: %d\n", text_count);
    printf("SOD resources:  %d\n", sod_count);
    printf("Binary resources: %d\n", bin_count);
    if (unknown_count > 0) {
        printf("Errors: %d\n", unknown_count);
    }
    
    return 0;
}
