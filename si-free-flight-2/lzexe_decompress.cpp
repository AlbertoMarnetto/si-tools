#include "write_debug.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>

#pragma pack(push, 1)
struct EXEHeader {
    uint16_t magic;
    uint16_t bytes_in_last_page;
    uint16_t pages_in_file;
    uint16_t num_relocs;
    uint16_t header_paragraphs;
    uint16_t min_extra_paragraphs;
    uint16_t max_extra_paragraphs;
    uint16_t initial_ss;
    uint16_t initial_sp;
    uint16_t checksum;
    uint16_t initial_ip;
    uint16_t initial_cs;
    uint16_t reloc_table_offset;
    uint16_t overlay_number;
    uint16_t lz_signature;
    uint16_t lz_version;
};
#pragma pack(pop)

// Bit stream structure
struct bitstream {
    const uint8_t* data_ptr;
    uint16_t buf;
    uint8_t count;
};

static void initbits(bitstream* bs, const uint8_t* data) {
    bs->data_ptr = data;
    bs->count = 0x10;
    bs->buf = data[0] | (data[1] << 8);
    bs->data_ptr += 2;
}

static int getbit(bitstream* bs) {
    int b = bs->buf & 1;
    if (--bs->count == 0) {
        bs->buf = bs->data_ptr[0] | (bs->data_ptr[1] << 8);
        bs->data_ptr += 2;
        bs->count = 0x10;
    } else {
        bs->buf >>= 1;
    }
    return b;
}

// Decompress LZSS code section
static int decompress_code(const uint8_t* comp_data, std::vector<uint8_t>& code_data) {
    bitstream bs;
    initbits(&bs, comp_data);
    
    // Decompression buffer (from unlzexe.c)
    static uint8_t decomp_buf[0x4500];
    uint8_t* p = decomp_buf;
    
    // Output buffer - start small and grow as needed
    code_data.resize(10240);  // 10 KB initial size
    uint8_t* out_ptr = code_data.data();
    size_t out_written = 0;
    size_t out_size = code_data.size();
    
    // Decompress
    while (true) {
        if (getbit(&bs)) {
            // Literal
            *p++ = *bs.data_ptr++;
        } else {
            // Match
            int len;
            int32_t span;
            
            if (!getbit(&bs)) {
                // 1-byte offset form
                len = getbit(&bs) << 1;
                len |= getbit(&bs);
                len += 2;
                uint8_t span8 = *bs.data_ptr++;
                span = (int16_t)(span8 | 0xFF00);  // Always negative
            } else {
                // 2/3-byte form
                uint8_t byte1 = *bs.data_ptr++;
                uint8_t byte2 = *bs.data_ptr++;
                uint16_t span16 = byte1;
                len = byte2;
                span16 |= ((len & ~0x07) << 5) | 0xE000;
                span = (int16_t)span16;
                len = (len & 0x07) + 2;
                
                if (len == 2) {
                    // 3-byte form
                    len = *bs.data_ptr++;
                    if (len == 0) {
                        break;  // End marker
                    }
                    if (len == 1) {
                        continue;  // Segment change
                    }
                    len++;
                }
            }
            
            // Copy match
            for (; len > 0; len--, p++) {
                *p = *(p + span);
            }
        }
        
        // Flush buffer when needed
        if ((size_t)(p - decomp_buf) > 0x4000) {
            // Check if we need to grow the output buffer
            if (out_written + 0x2000 > out_size) {
                size_t new_size = std::max(out_size * 2, out_size + 0x2000);
                code_data.resize(new_size);
                out_ptr = code_data.data() + out_written;
                out_size = new_size;
            }
            
            memcpy(out_ptr, decomp_buf, 0x2000);
            out_ptr += 0x2000;
            out_written += 0x2000;
            
            p -= 0x2000;
            memmove(decomp_buf, decomp_buf + 0x2000, p - decomp_buf);
        }
    }
    
    // Write remaining
    size_t remaining = p - decomp_buf;
    if (out_written + remaining > out_size) {
        size_t new_size = std::max(out_size * 2, out_written + remaining);
        code_data.resize(new_size);
        out_ptr = code_data.data() + out_written;
        out_size = new_size;
    }
    memcpy(out_ptr, decomp_buf, remaining);
    out_written += remaining;
    
    // Resize to actual size
    code_data.resize(out_written);
    
    return 0;
}

// Decompress relocation table for LZEXE v0.91
static int decompress_reloc91(const uint8_t* reloc_data, size_t reloc_size, 
                              std::vector<std::pair<uint16_t, uint16_t>>& relocs) {
    const uint8_t* ptr = reloc_data;
    const uint8_t* end = reloc_data + reloc_size;
    
    uint16_t rel_off = 0;
    uint16_t rel_seg = 0;
    
    while (ptr < end) {
        uint16_t span = *ptr++;

        if (span == 0) {
            if (ptr + 2 > end) break;
            span = ptr[0] | (ptr[1] << 8);
            ptr += 2;

            if (span == 0) {
                // Paragraph skip
                rel_seg += 0x0FFF;
                continue;
            } else if (span == 1) {
                // End marker
                break;
            }
            // Otherwise use span value read from word
        }

        rel_off += span;
        rel_seg += (rel_off & ~0x0F) >> 4;
        rel_off &= 0x0F;
        
        relocs.push_back({rel_off, rel_seg});
    }
    
    return 0;
}

int lzexe_decompress(const uint8_t* input_data, size_t input_size, 
                     std::vector<uint8_t>& output_data) {
    // Parse EXE header
    if (input_size < sizeof(EXEHeader)) {
        write_debug("Error: File too small\n");
        return -1;
    }
    
    const EXEHeader* exe_header = reinterpret_cast<const EXEHeader*>(input_data);
    
    if (exe_header->magic != 0x5A4D && exe_header->magic != 0x4D5A) {
        write_debug("Error: Not a valid EXE file (magic: 0x%04X)\n", exe_header->magic);
        return -1;
    }
    
    if (exe_header->lz_signature != 0x5A4C) {
        write_debug("Error: Not an LZEXE compressed file (sig: 0x%04X)\n", 
                exe_header->lz_signature);
        return -1;
    }
    
    int version = (exe_header->lz_version == 0x3930) ? 90 : 91;
    write_debug("LZEXE file detected (version 0.%d)\n", version);
    
    // Read LZ info (8 words at CS:0000)
    uint32_t lz_info_offset = ((uint32_t)exe_header->initial_cs + (uint32_t)exe_header->header_paragraphs) << 4;
    if (lz_info_offset + 16 > input_size) {
        write_debug("Error: LZ info offset out of bounds\n");
        return -1;
    }
    
    const uint16_t* lz_info = reinterpret_cast<const uint16_t*>(input_data + lz_info_offset);
    uint16_t lz_data_length = lz_info[4];  // Compressed data size in paragraphs
    
    // Extract original header values from LZ info
    uint16_t orig_ip = lz_info[0];
    uint16_t orig_cs = lz_info[1];
    uint16_t orig_sp = lz_info[2];
    uint16_t orig_ss = lz_info[3];
    uint16_t inf_5 = lz_info[5];  // increase of load module size (paragraphs)
    uint16_t inf_6 = lz_info[6];  // size of decompressor with reloc table (bytes)
    
    // Calculate positions
    uint32_t cs_base = ((uint32_t)exe_header->initial_cs + (uint32_t)exe_header->header_paragraphs) << 4;
    uint32_t compressed_start = ((uint32_t)exe_header->initial_cs - (uint32_t)lz_data_length + 
                                 (uint32_t)exe_header->header_paragraphs) << 4;
    uint32_t reloc_start_91 = cs_base + 0x158;  // For v0.91
    
    write_debug("Compressed code starts at: 0x%08X\n", compressed_start);
    write_debug("Relocation table starts at: 0x%08X\n", reloc_start_91);
    
    // Decompress code section
    const uint8_t* comp_data = input_data + compressed_start;
    std::vector<uint8_t> code_data;
    if (decompress_code(comp_data, code_data) != 0) {
        write_debug("Error: Code decompression failed\n");
        return -1;
    }
    
    write_debug("Decompressed code: %zu bytes\n", code_data.size());
    
    // Decompress relocation table
    const uint8_t* reloc_data = input_data + reloc_start_91;
    size_t reloc_size = input_size - reloc_start_91;
    std::vector<std::pair<uint16_t, uint16_t>> relocs;
    if (decompress_reloc91(reloc_data, reloc_size, relocs) != 0) {
        write_debug("Error: Relocation decompression failed\n");
        return -1;
    }
    
    write_debug("Decompressed relocations: %zu entries\n", relocs.size());
    
    // Build output EXE file following reference unlzexe logic:
    // 1. Write header placeholder
    // 2. Write reloc table at offset 0x1C
    // 3. Pad to paragraph boundary
    // 4. Write code section
    // 5. Update header with correct values
    
    // Start with temporary buffer
    std::vector<uint8_t> temp_output;
    temp_output.resize(0x1C + relocs.size() * 4);  // Header + reloc table
    
    // Write reloc table
    uint8_t* reloc_ptr = temp_output.data() + 0x1C;
    for (const auto& reloc : relocs) {
        reloc_ptr[0] = reloc.first & 0xFF;
        reloc_ptr[1] = (reloc.first >> 8) & 0xFF;
        reloc_ptr[2] = reloc.second & 0xFF;
        reloc_ptr[3] = (reloc.second >> 8) & 0xFF;
        reloc_ptr += 4;
    }
    
    // Calculate padding to next paragraph boundary (like reference: i = (0x200 - fpos) & 0x1ff)
    size_t fpos = temp_output.size();
    size_t padding = (0x200 - fpos) & 0x1FF;
    temp_output.insert(temp_output.end(), padding, 0);
    
    // Calculate header_paragraphs (like reference: ohead[4] = (fpos + i) >> 4)
    uint16_t header_paragraphs = (temp_output.size()) >> 4;
    
    // Append code section
    temp_output.insert(temp_output.end(), code_data.begin(), code_data.end());
    
    // Calculate file size and pad to 512-byte boundary
    size_t file_size = temp_output.size();
    size_t bytes_last = file_size % 512;
    size_t pages = (file_size + 511) / 512;
    if (bytes_last == 0) {
        bytes_last = 512;
        pages = (file_size - 1) / 512 + 1;
    }
    
    // Build final output with proper header
    output_data.resize(0x1C);  // Start with header space
    
    // Build EXE header (following reference unlzexe logic)
    EXEHeader out_header;
    memset(&out_header, 0, sizeof(out_header));
    out_header.magic = 0x5A4D;
    out_header.bytes_in_last_page = bytes_last;
    out_header.pages_in_file = pages;
    out_header.num_relocs = relocs.size();
    out_header.header_paragraphs = header_paragraphs;
    
    // Calculate min_extra and max_extra like reference wrhead()
    uint16_t min_extra = exe_header->min_extra_paragraphs;
    uint16_t max_extra = exe_header->max_extra_paragraphs;
    
    if (exe_header->max_extra_paragraphs != 0) {
        min_extra -= inf_5 + ((inf_6 + 15) >> 4) + 9;
        if (exe_header->max_extra_paragraphs != 0xFFFF) {
            max_extra -= (exe_header->min_extra_paragraphs - min_extra);
        }
    }
    
    out_header.min_extra_paragraphs = min_extra;
    out_header.max_extra_paragraphs = max_extra;
    out_header.initial_ss = orig_ss;
    out_header.initial_sp = orig_sp;
    out_header.checksum = 0;
    out_header.initial_ip = orig_ip;
    out_header.initial_cs = orig_cs;
    out_header.reloc_table_offset = 0x1C;
    out_header.overlay_number = 0;
    
    // Write header
    memcpy(output_data.data(), &out_header, sizeof(out_header));
    
    // Append rest of file (reloc table + padding + code)
    output_data.insert(output_data.end(), temp_output.begin() + 0x1C, temp_output.end());
    
    write_debug("Output file size: %zu bytes\n", output_data.size());
    write_debug("Header paragraphs: %u, Relocs: %zu, Code: %zu bytes\n", 
           header_paragraphs, relocs.size(), code_data.size());
    
    return 0;
}

#if 0  // Disable main() when used as library
int main(int argc, char* argv[]) {
    if (argc < 2) {
        write_debug("Usage: %s <input.exe> [output.exe]\n", argv[0]);
        return 1;
    }

    // Read input file
    std::ifstream infile(argv[1], std::ios::binary);
    if (!infile) {
        write_debug("Error: Cannot open input file %s\n", argv[1]);
        return 1;
    }

    infile.seekg(0, std::ios::end);
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    std::vector<uint8_t> input_data(file_size);
    infile.read(reinterpret_cast<char*>(input_data.data()), file_size);
    infile.close();

    // Decompress
    std::vector<uint8_t> output_data;
    if (lzexe_decompress(input_data.data(), file_size, output_data) != 0) {
        return 1;
    }

    // Write output
    std::string output_path = (argc >= 3) ? argv[2] : "output.exe";
    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile) {
        write_debug("Error: Cannot open output file %s\n", output_path.c_str());
        return 1;
    }

    outfile.write(reinterpret_cast<const char*>(output_data.data()), output_data.size());
    outfile.close();

    write_debug("Output written to %s\n", output_path.c_str());

    return 0;
}
#endif  // Disable main() when used as library
