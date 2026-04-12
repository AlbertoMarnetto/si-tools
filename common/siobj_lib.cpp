#include "siobj_lib.h"

#include "buffer.h"
#include "write_debug.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

static const double kTau = 6.28318530717958647692;

// Model OP codes
#define OP_CLRV      (0x00)
#define OP_GETV      (0x09)
#define OP_ID        (0x0c)
#define OP_PUTV      (0x12)
#define OP_INTV      (0x1b)
#define OP_ADDV      (0x24)
#define OP_SUBV      (0x2d)
#define OP_NEGV      (0x36)
#define OP_SHLV      (0x3f)
#define OP_SHRV      (0x48)
#define OP_VEC       (0x4b)
#define OP_BLOKADD   (0x51)
#define OP_BLOKNEG   (0x5a)
#define OP_IPG       (0x63)
#define OP_VECTORS   (0x75)
#define OP_PUSHCENT  (0x7e)
#define OP_POPCENT   (0x87)
#define OP_MOVECENT  (0x90)
#define OP_PERS      (0x99)
#define OP_MOVETO    (0x00a2)
#define OP_LINECOL   (0x00b4)
#define OP_ZLINETO   (0x00bd)
#define OP_POLYZ     (0x00f3)
#define OP_SPHERE    (0x00fc)
#define OP_DISC      (0x0105)
#define OP_PERFDISK  (0x010e)
#define OP_COLOR     (0x0129)
#define OP_SHADES    (0x0156)
#define OP_SHADE1    (0x0144)
#define OP_GVPOLY    (0x0168)
#define OP_IFVIS     (0x017a)
#define OP_SORT      (0x018c)
#define OP_RETURN    (0x0195)
#define OP_GOTO      (0x019e)
#define OP_GOSUB     (0x01a7)
#define OP_REJ       (0x01b9)
#define OP_ROTY      (0x01dd)
#define OP_PUSHRELMAT (0x01e6)
#define OP_POPRELMAT  (0x01ef)
#define OP_ANIMATE    (0x01f8)
#define OP_GVPOLY16  (0x02be)
#define OP_SHADE16   (0x02b5)
#define OP_DOT       (0x02d0)
#define OP_BLOCK     (0x02d9)
#define OP_ROTX      (0x02e2)
#define OP_ROTZ      (0x02eb)
#define OP_BVLT      (0x02f4)
#define OP_BVGT      (0x02fd)
#define OP_BZLT      (0x0318)
#define OP_SCALEIT   (0x0321)

static float scale_table[] = {
    10.0f,
    5.0f,
    2.5f,
    1.25f,
    0.625f,
    0.3125f,
    0.15625f,
    0.078125f,
    0.0390625f,
    0.01953125f,
    0.009765625f,
    0.0048828125f,
    0.00244140625f,
    0.001220703125f,
    0.0006103515625f,
    0.00030517578125f,
    0.000152587890625f,
    0.0000762939453125f
};

// Palette
const uint8_t sipal[] = {
    0x00,0x00,0x00,
    0x04,0x04,0x04,
    0x0c,0x0c,0x0c,
    0x14,0x14,0x14,
    0x20,0x1c,0x1c,
    0x28,0x24,0x24,
    0x30,0x28,0x28,
    0x38,0x30,0x30,
    0x40,0x38,0x38,
    0x4c,0x40,0x40,
    0x54,0x48,0x48,
    0x5c,0x50,0x50,
    0x64,0x58,0x58,
    0x6c,0x60,0x60,
    0x74,0x68,0x68,
    0x80,0x70,0x70,
    0x3c,0x0c,0x18,
    0x48,0x0c,0x1c,
    0x54,0x10,0x20,
    0x60,0x14,0x24,
    0x6c,0x14,0x28,
    0x78,0x18,0x30,
    0x84,0x18,0x34,
    0x90,0x1c,0x38,
    0x9c,0x20,0x3c,
    0xa8,0x20,0x40,
    0xb4,0x24,0x44,
    0xc4,0x28,0x4c,
    0xd0,0x28,0x50,
    0xdc,0x2c,0x54,
    0xf8,0x6c,0xb0,
    0xfc,0x9c,0xdc,
    0x50,0x40,0x40,
    0x54,0x44,0x44,
    0x58,0x48,0x48,
    0x60,0x50,0x50,
    0x64,0x54,0x54,
    0x68,0x5c,0x5c,
    0x70,0x60,0x60,
    0x74,0x68,0x68,
    0x78,0x6c,0x6c,
    0x80,0x74,0x74,
    0x84,0x78,0x78,
    0x88,0x80,0x80,
    0x90,0x84,0x84,
    0x94,0x8c,0x8c,
    0x98,0x90,0x90,
    0xa0,0x98,0x98,
    0xa4,0x9c,0x9c,
    0xa8,0xa0,0xa0,
    0xb0,0xa8,0xa8,
    0xb4,0xac,0xac,
    0xb8,0xb4,0xb4,
    0xc0,0xb8,0xb8,
    0xc4,0xc0,0xc0,
    0xc8,0xc4,0xc4,
    0xd0,0xcc,0xcc,
    0xd4,0xd0,0xd0,
    0xd8,0xd8,0xd8,
    0xe0,0xdc,0xdc,
    0xe4,0xe4,0xe4,
    0xe8,0xe8,0xe8,
    0xf0,0xf0,0xf0,
    0xf4,0xf4,0xf4,
    0x3c,0x34,0x20,
    0x40,0x38,0x24,
    0x48,0x40,0x28,
    0x4c,0x40,0x28,
    0x54,0x48,0x2c,
    0x58,0x4c,0x30,
    0x60,0x54,0x34,
    0x64,0x54,0x34,
    0x6c,0x5c,0x38,
    0x70,0x60,0x3c,
    0x78,0x68,0x40,
    0x80,0x6c,0x44,
    0x84,0x70,0x48,
    0x8c,0x78,0x4c,
    0x90,0x7c,0x4c,
    0x98,0x80,0x50,
    0x9c,0x84,0x54,
    0xa4,0x8c,0x58,
    0xa8,0x90,0x58,
    0xb0,0x94,0x5c,
    0xb4,0x98,0x60,
    0xbc,0xa0,0x64,
    0xc4,0xa8,0x68,
    0xc8,0xac,0x6c,
    0xd0,0xb0,0x70,
    0xd4,0xb4,0x70,
    0xdc,0xbc,0x74,
    0xe0,0xc0,0x78,
    0xe8,0xc4,0x7c,
    0xec,0xc8,0x7c,
    0xf4,0xd0,0x80,
    0xfc,0xd8,0x88,
    0x28,0x24,0x18,
    0x28,0x28,0x18,
    0x30,0x2c,0x1c,
    0x30,0x30,0x1c,
    0x38,0x34,0x20,
    0x38,0x38,0x20,
    0x40,0x3c,0x24,
    0x40,0x3c,0x24,
    0x44,0x44,0x28,
    0x48,0x44,0x28,
    0x4c,0x48,0x2c,
    0x54,0x50,0x30,
    0x54,0x50,0x30,
    0x5c,0x54,0x34,
    0x5c,0x58,0x34,
    0x60,0x5c,0x38,
    0x64,0x60,0x3c,
    0x68,0x64,0x3c,
    0x6c,0x68,0x40,
    0x70,0x6c,0x40,
    0x74,0x70,0x44,
    0x78,0x74,0x48,
    0x80,0x78,0x48,
    0x80,0x7c,0x4c,
    0x84,0x80,0x4c,
    0x88,0x80,0x50,
    0x8c,0x88,0x54,
    0x90,0x88,0x54,
    0x94,0x8c,0x58,
    0x98,0x90,0x58,
    0x9c,0x94,0x5c,
    0xa4,0x9c,0x60,
    0x00,0x00,0x00,
    0x34,0x18,0x00,
    0x44,0x1c,0x00,
    0x54,0x24,0x04,
    0x64,0x2c,0x0c,
    0x78,0x38,0x18,
    0x8c,0x48,0x20,
    0x9c,0x54,0x34,
    0x1c,0x28,0x60,
    0x28,0x30,0x74,
    0x38,0x34,0x88,
    0x3c,0x40,0x90,
    0x44,0x50,0xa0,
    0x50,0x60,0xb0,
    0x60,0x74,0xc4,
    0x70,0x84,0xd8,
    0x70,0x54,0x4c,
    0x7c,0x60,0x58,
    0x8c,0x70,0x68,
    0x9c,0x80,0x78,
    0xa8,0x90,0x84,
    0xb8,0xa0,0x98,
    0xc8,0xb0,0xa8,
    0xd8,0xc4,0xbc,
    0x4c,0x48,0x60,
    0x54,0x54,0x70,
    0x64,0x64,0x84,
    0x78,0x78,0x94,
    0x88,0x88,0xa4,
    0x98,0x9c,0xb0,
    0xa8,0xb0,0xc4,
    0xb8,0xbc,0xc8,
    0x50,0x60,0x50,
    0x60,0x70,0x60,
    0x74,0x84,0x74,
    0x84,0x94,0x84,
    0x98,0xa8,0x98,
    0xac,0xbc,0xac,
    0xbc,0xcc,0xbc,
    0xd0,0xe0,0xd0,
    0x20,0x30,0x08,
    0x24,0x34,0x08,
    0x2c,0x3c,0x08,
    0x30,0x44,0x0c,
    0x38,0x4c,0x0c,
    0x3c,0x50,0x10,
    0x44,0x58,0x10,
    0x4c,0x60,0x14,
    0x54,0x68,0x14,
    0x60,0x70,0x14,
    0x64,0x78,0x18,
    0x70,0x80,0x18,
    0x78,0x88,0x20,
    0x7c,0x90,0x1c,
    0x80,0x94,0x20,
    0x88,0x98,0x20,
    0x8c,0x9c,0x24,
    0x94,0xa0,0x30,
    0x98,0xa4,0x40,
    0x9c,0xa8,0x48,
    0xa4,0xac,0x58,
    0xa4,0xb0,0x5c,
    0xac,0xb4,0x58,
    0xb8,0xb8,0x64,
    0x24,0x44,0x14,
    0x2c,0x50,0x18,
    0x34,0x5c,0x20,
    0x3c,0x68,0x24,
    0x44,0x78,0x2c,
    0x50,0x84,0x34,
    0x58,0x90,0x3c,
    0x64,0xa0,0x44,
    0xc4,0x38,0x00,
    0xe0,0x54,0x00,
    0xfc,0x80,0x00,
    0xf0,0xb8,0x1c,
    0x14,0x8c,0xb8,
    0x3c,0x54,0x6c,
    0xd0,0x08,0xa0,
    0x94,0x00,0x6c,
    0x10,0x48,0x78,
    0xfc,0x00,0xfc,
    0xfc,0x00,0xfc,
    0xfc,0x00,0xfc,
    0xfc,0x00,0xfc,
    0xa0,0x98,0x98,
    0x74,0x68,0x68,
    0xfc,0x00,0x00,
    0xf0,0x88,0x00,
    0xfc,0xb8,0x00,
    0xfc,0xe4,0x00,
    0xfc,0xf4,0xec,
    0x8c,0x90,0xa8,
    0xa0,0xa4,0xb8,
    0xb0,0xb4,0xcc,
    0xc4,0xc8,0xdc,
    0x40,0x94,0xe8,
    0x40,0x9c,0xe8,
    0x30,0xa4,0xec,
    0x34,0xac,0xf4,
    0x44,0xb0,0xf0,
    0x50,0xb0,0xec,
    0x60,0xb4,0xe8,
    0x6c,0xb8,0xe8,
    0x7c,0xbc,0xe4,
    0x88,0xc0,0xe0,
    0x98,0xc0,0xe8,
    0xa4,0xc8,0xec,
    0xb4,0xcc,0xf4,
    0xc0,0xd8,0xf8,
    0xd0,0xe0,0xfc,
    0xfc,0xf8,0x94,
    0x00,0xfc,0x00,
    0x9c,0x6c,0x00,
    0xfc,0x84,0x00,
    0xfc,0x00,0x00,
    0xfc,0x00,0x28,
    0xfc,0x00,0x58,
    0xfc,0x00,0x88,
    0xfc,0x00,0xb4,
    0xfc,0x00,0xe4,
    0xe4,0x00,0xfc,
    0xb4,0x00,0xfc,
    0x88,0x00,0xfc,
    0x58,0x00,0xfc,
    0x28,0x00,0xfc,
    0x00,0x00,0xfc,
    0xfc,0xfc,0xfc
};

// Simplified Binfile for loading
class Binfile {
private:
    FILE *file;
    unsigned int file_size;
    bool valid;

public:
    Binfile(const char *name) {
        file = fopen(name,"rb");
        if (file == NULL) {
            valid = false;
            return;
        }
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        valid = true;
    }

    ~Binfile() {
        if (file) fclose(file);
    }

    unsigned int size() { return valid ? file_size : 0; }

    bool read(char *data) {
        if (!valid) return false;
        size_t read = fread(data, 1, file_size, file);
        return read == file_size;
    }
};

// Global for current object
ParsedObject current_obj;

// Track which vertex registers are from IPG primitives (for debugging)
static bool ipg_vertices[2000] = {false};

static int g_cur_color = 0; // Global current color
static SIVERTEX g_cur_vert;
static int g_relpos_x = 0;
static int g_relpos_y = 0;
static int g_relpos_z = 0;
static std::vector<int> g_relpos_stack;
static std::array<double, 9> g_relmat = {1.0, 0.0, 0.0,
                                         0.0, 1.0, 0.0,
                                         0.0, 0.0, 1.0};
static std::vector<std::array<double, 9>> g_relmat_stack;
static int g_variables[16] = {0};
static int g_current_plane_index = -1;
static std::array<int8_t, 512> g_face_visibility = {};

// Modified vertex functions to collect instead of render
static void collect_vertex(int x, int y, int z) {
    SIVERTEX v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.flags = true;
    current_obj.vertices.push_back(v);
}

static int append_transformed_vertex(const SIVERTEX &src) {
    SIVERTEX v;
    v.x = src.x + g_relpos_x;
    v.y = src.y + g_relpos_y;
    v.z = src.z + g_relpos_z;
    v.other = 0;
    v.flags = true;
    current_obj.vertices.push_back(v);
    return (int)current_obj.vertices.size() - 1;
}

static SIVERTEX transform_input_vertex(int x, int y, int z) {
    // Match the original SOD interpreter more closely:
    // apply relmat to the raw input point and keep the resulting local
    // coordinates in SOD space. Export-time axis conversion happens later.
    double in_x = (double)x;
    double in_y = (double)y;
    double in_z = (double)z;

    SIVERTEX v;
    v.x = (int)std::lround(in_x * g_relmat[0] + in_y * g_relmat[1] + in_z * g_relmat[2]);
    v.y = (int)std::lround(in_x * g_relmat[3] + in_y * g_relmat[4] + in_z * g_relmat[5]);
    v.z = (int)std::lround(in_x * g_relmat[6] + in_y * g_relmat[7] + in_z * g_relmat[8]);
    v.other = 0;
    v.flags = true;
    return v;
}

static std::array<double, 9> mul_mat3(const std::array<double, 9> &lhs, const std::array<double, 9> &rhs) {
    std::array<double, 9> out = {};
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            out[r * 3 + c] =
                lhs[r * 3 + 0] * rhs[0 * 3 + c] +
                lhs[r * 3 + 1] * rhs[1 * 3 + c] +
                lhs[r * 3 + 2] * rhs[2 * 3 + c];
        }
    }
    return out;
}

static void apply_rotation_x(int angle_units) {
    double angle = (kTau * (double)angle_units) / 4096.0;
    double c = std::cos(angle);
    double s = std::sin(angle);
    std::array<double, 9> rot = {1.0, 0.0, 0.0,
                                 0.0, c,   -s,
                                 0.0, s,    c};
    g_relmat = mul_mat3(g_relmat, rot);
}

static void apply_rotation_y(int angle_units) {
    double angle = (kTau * (double)angle_units) / 4096.0;
    double c = std::cos(angle);
    double s = std::sin(angle);
    std::array<double, 9> rot = { c, 0.0, -s,
                                 0.0, 1.0, 0.0,
                                  s, 0.0, c};
    g_relmat = mul_mat3(g_relmat, rot);
}

static void apply_rotation_z(int angle_units) {
    double angle = (kTau * (double)angle_units) / 4096.0;
    double c = std::cos(angle);
    double s = std::sin(angle);
    std::array<double, 9> rot = {c,  -s, 0.0,
                                 s,   c, 0.0,
                                 0.0, 0.0, 1.0};
    g_relmat = mul_mat3(g_relmat, rot);
}

struct ParserStateSnapshot {
    int cur_color;
    SIVERTEX cur_vert;
    int relpos_x;
    int relpos_y;
    int relpos_z;
    std::vector<int> relpos_stack;
    std::array<double, 9> relmat;
    std::vector<std::array<double, 9>> relmat_stack;
    std::array<int8_t, 512> face_visibility;
};

static ParserStateSnapshot snapshot_parser_state() {
    ParserStateSnapshot s;
    s.cur_color = g_cur_color;
    s.cur_vert = g_cur_vert;
    s.relpos_x = g_relpos_x;
    s.relpos_y = g_relpos_y;
    s.relpos_z = g_relpos_z;
    s.relpos_stack = g_relpos_stack;
    s.relmat = g_relmat;
    s.relmat_stack = g_relmat_stack;
    s.face_visibility = g_face_visibility;
    return s;
}

static void restore_parser_state(const ParserStateSnapshot &s) {
    g_cur_color = s.cur_color;
    g_cur_vert = s.cur_vert;
    g_relpos_x = s.relpos_x;
    g_relpos_y = s.relpos_y;
    g_relpos_z = s.relpos_z;
    g_relpos_stack = s.relpos_stack;
    g_relmat = s.relmat;
    g_relmat_stack = s.relmat_stack;
    g_face_visibility = s.face_visibility;
}

static int compute_face_visibility_3d(const SIVERTEX &a, const SIVERTEX &d, const SIVERTEX &b,
                                      int relx, int rely, int relz) {
    // Mirror `getnorm` + `reject3d` closely:
    // 1. differences are 16-bit signed values
    // 2. cross-product components are formed in 32-bit
    // 3. the final dot product uses only the low signed 16 bits of each
    //    normal component
    int16_t za = (int16_t)(a.z - d.z);
    int16_t yb = (int16_t)(b.y - d.y);
    int16_t ya = (int16_t)(a.y - d.y);
    int16_t zb = (int16_t)(b.z - d.z);
    int16_t xa = (int16_t)(a.x - d.x);
    int16_t xb = (int16_t)(b.x - d.x);

    int32_t nx32 = (int32_t)ya * (int32_t)zb - (int32_t)za * (int32_t)yb;
    int32_t ny32 = (int32_t)za * (int32_t)xb - (int32_t)xa * (int32_t)zb;
    int32_t nz32 = (int32_t)xa * (int32_t)yb - (int32_t)ya * (int32_t)xb;

    int16_t nx = (int16_t)nx32;
    int16_t ny = (int16_t)ny32;
    int16_t nz = (int16_t)nz32;

    int16_t px = (int16_t)(d.x + relx);
    int16_t py = (int16_t)(d.y + rely);
    int16_t pz = (int16_t)(d.z + relz);

    int32_t dot = (int32_t)pz * (int32_t)nz;
    dot += (int32_t)py * (int32_t)ny;
    dot += (int32_t)px * (int32_t)nx;
    return (dot < 0) ? -1 : 0;
}

// Helper to set vertex coordinates
static void set_vert(SIVERTEX *v, int x, int y, int z) {
    v->x = x;
    v->y = y;
    v->z = z;
    v->flags = true;
}

// Vertex manipulation helpers
static void get_vert(SIVERTEX *d, SIVERTEX *s) {
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
    d->flags = true;
}

static void mid_vert(SIVERTEX *d, SIVERTEX *s) {
    d->x = (d->x + s->x) / 2;
    d->y = (d->y + s->y) / 2;
    d->z = (d->z + s->z) / 2;
    d->flags = true;
}

static void add_vert(SIVERTEX *d, SIVERTEX *s) {
    d->x += s->x;
    d->y += s->y;
    d->z += s->z;
}

static void sub_vert(SIVERTEX *d, SIVERTEX *s) {
    d->x -= s->x;
    d->y -= s->y;
    d->z -= s->z;
}

static void neg_vert(SIVERTEX *v) {
    v->x = -v->x;
    v->y = -v->y;
    v->z = -v->z;
}

static void shr_vert(SIVERTEX *v, int s) {
    v->x = v->x >> s;
    v->y = v->y >> s;
    v->z = v->z >> s;
}

static void shl_vert(SIVERTEX *v, int s) {
    v->x = v->x << s;
    v->y = v->y << s;
    v->z = v->z << s;
}

// Helper to add a sphere as icosphere approximation (like Blender does)
// radius is in SOD units, center is the vertex position
void add_sphere_geometry(const SIVERTEX &center, int radius, int color, ParsedObject &obj) {
    // Icosphere with 20 faces (similar to Blender's default subdivision level 1)
    // Vertices of a normalized icosphere
    std::vector<float> ico_verts = {
        -0.525731f, 0.850651f, 0.0f,
        0.525731f, 0.850651f, 0.0f,
        -0.525731f, -0.850651f, 0.0f,
        0.525731f, -0.850651f, 0.0f,
        0.0f, -0.525731f, 0.850651f,
        0.0f, 0.525731f, 0.850651f,
        0.0f, -0.525731f, -0.850651f,
        0.0f, 0.525731f, -0.850651f,
        0.850651f, 0.0f, -0.525731f,
        0.850651f, 0.0f, 0.525731f,
        -0.850651f, 0.0f, -0.525731f,
        -0.850651f, 0.0f, 0.525731f,
    };
    
    std::vector<int> ico_faces = {
        0, 11, 5,  0, 5, 1,  0, 1, 7,  0, 7, 10,  0, 10, 11,
        1, 5, 9,  5, 11, 4,  11, 10, 2,  10, 7, 6,  7, 1, 8,
        3, 9, 4,  3, 4, 2,  3, 2, 6,  3, 6, 8,  3, 8, 9,
        4, 9, 5,  2, 4, 11,  6, 2, 10,  8, 6, 7,  9, 8, 1,
    };
    
    int vert_base = obj.vertices.size();

    // Safety check: avoid degenerate geometry if radius is 0 or negative
    if (radius <= 0) return;

    // Scale and translate vertices
    for (size_t i = 0; i < ico_verts.size(); i += 3) {
        SIVERTEX v;
        v.x = center.x + (int)(ico_verts[i] * radius);
        v.y = center.y + (int)(ico_verts[i+1] * radius);
        v.z = center.z + (int)(ico_verts[i+2] * radius);
        v.other = 0;
        v.flags = true;
        obj.vertices.push_back(v);
    }
    
    // Add triangular faces
    for (size_t i = 0; i < ico_faces.size(); i += 3) {
        obj.faces.push_back({
            vert_base + ico_faces[i],
            vert_base + ico_faces[i+1],
            vert_base + ico_faces[i+2]
        });
        obj.face_colors.push_back(color); // Add face color
    }
    
    obj.approx_spheres++;
}

// Helper to add a disk as fan triangulation (like Blender does)
// center is the vertex position, normal_vertex defines the orientation
// radius is in SOD units
void add_disk_geometry(const SIVERTEX &center, const SIVERTEX &normal_vertex, 
                       int radius, int color, ParsedObject &obj) {
    const int DISK_SEGMENTS = 16;  // Number of triangles radiating from center

    // Safety check: avoid degenerate geometry
    if (radius <= 0) return;

    int vert_base = obj.vertices.size();

    // Add center vertex
    obj.vertices.push_back(center);
    
    // Calculate disk normal (simplified: use normal_vertex as guidance)
    // For now, create disk in XY plane and rotate based on normal_vertex
    float nx = (float)normal_vertex.x / 32768.0f;
    float ny = (float)normal_vertex.y / 32768.0f;
    float nz = (float)normal_vertex.z / 32768.0f;
    
    // Create rim vertices
    for (int i = 0; i < DISK_SEGMENTS; i++) {
        float angle = (2.0f * 3.14159265f * i) / DISK_SEGMENTS;
        float cx = cos(angle) * radius;
        float sy = sin(angle) * radius;
        
        SIVERTEX v;
        // Apply approximate rotation based on normal
        if (fabs(nz) > 0.9f) {
            // Disk is mostly in XY plane
            v.x = center.x + (int)cx;
            v.y = center.y + (int)sy;
            v.z = center.z;
        } else if (fabs(ny) > 0.9f) {
            // Disk is mostly in XZ plane  
            v.x = center.x + (int)cx;
            v.y = center.y;
            v.z = center.z + (int)sy;
        } else {
            // Disk is mostly in YZ plane
            v.x = center.x;
            v.y = center.y + (int)cx;
            v.z = center.z + (int)sy;
        }
        v.other = 0;
        v.flags = true;
        obj.vertices.push_back(v);
    }
    
    // Add fan triangles
    for (int i = 0; i < DISK_SEGMENTS; i++) {
        int next = (i + 1) % DISK_SEGMENTS;
        obj.faces.push_back({
            vert_base,                    // center
            vert_base + 1 + i,            // current rim vertex
            vert_base + 1 + next          // next rim vertex
        });
        obj.face_colors.push_back(color); // Add face color
    }
    
    obj.approx_disks++;
}

// IPG (Initialize Primitive Geometry) handler
// IPG creates primitive shapes (cubes, quads) for collision/bounding boxes
// We mark the vertices as valid but do NOT export the bounding box geometry
int siobj_parse_ipg(int16_t *buf, int bi, SIVERTEX *verts, int *vert_mapping) {
    int16_t w, w1, w2, w3, w4;
    int vert_index;
    int ipg_opcode_idx = bi - 1;

    w = buf[bi++];  // IPG type
    switch (w) {
    case 0: {  // Cube - bounding box, not visible
        w1 = buf[bi++];  // half_x
        w2 = buf[bi++];  // half_y
        w3 = buf[bi++];  // half_z
        w4 = buf[bi++] / 0xc;  // start vertex
        vert_index = w4;

        // Mark these registers as IPG vertices for debugging
        for (int i = 0; i < 8; i++) {
            ipg_vertices[vert_index + i] = true;
        }

        verts[vert_index + 0] = transform_input_vertex(-w1, -w2, -w3);
        verts[vert_index + 1] = transform_input_vertex(-w1, -w2,  w3);
        verts[vert_index + 2] = transform_input_vertex( w1, -w2,  w3);
        verts[vert_index + 3] = transform_input_vertex( w1, -w2, -w3);
        verts[vert_index + 4] = transform_input_vertex(-w1,  w2, -w3);
        verts[vert_index + 5] = transform_input_vertex(-w1,  w2,  w3);
        verts[vert_index + 6] = transform_input_vertex( w1,  w2,  w3);
        verts[vert_index + 7] = transform_input_vertex( w1,  w2, -w3);
        current_obj.skipped_blocks++;
//         write_debug("DEBUG IPG Cube: at bi=%d, start_reg=%d (enc=%04x), half=(%d,%d,%d)\n",
//                   ipg_opcode_idx, vert_index, w4*0xc, w1, w2, w3);
        break;
    }

    case 2: {  // Ground quad - collision surface, not visible
        w1 = buf[bi++];  // half_x
        w2 = buf[bi++];  // half_z
        w4 = buf[bi++] / 0xc;  // start vertex
        vert_index = w4;

        for (int i = 0; i < 4; i++) {
            ipg_vertices[vert_index + i] = true;
        }

        verts[vert_index + 0] = transform_input_vertex(-w1, 0, -w2);
        verts[vert_index + 1] = transform_input_vertex(-w1, 0,  w2);
        verts[vert_index + 2] = transform_input_vertex( w1, 0,  w2);
        verts[vert_index + 3] = transform_input_vertex( w1, 0, -w2);
        current_obj.skipped_blocks++;
//         write_debug("DEBUG IPG Ground Quad: at bi=%d, start_reg=%d (enc=%04x), params=(%d,%d)\n",
//                   ipg_opcode_idx, vert_index, w4*0xc, w1, w2);
        break;
    }

    case 8: {  // Flat quad - XY plane, not visible
        w1 = buf[bi++];  // half_x
        w2 = buf[bi++];  // half_y
        w4 = buf[bi++] / 0xc;  // start vertex
        vert_index = w4;

        for (int i = 0; i < 4; i++) {
            ipg_vertices[vert_index + i] = true;
        }

        verts[vert_index + 0] = transform_input_vertex( w1,  w2, 0);
        verts[vert_index + 1] = transform_input_vertex(-w1,  w2, 0);
        verts[vert_index + 2] = transform_input_vertex(-w1, -w2, 0);
        verts[vert_index + 3] = transform_input_vertex( w1, -w2, 0);
        current_obj.skipped_blocks++;
//         write_debug("DEBUG IPG Flat Quad: at bi=%d, start_reg=%d (enc=%04x), params=(%d,%d)\n",
//                   ipg_opcode_idx, vert_index, w4*0xc, w1, w2);
        break;
    }

    default: {
        write_debug("Unknown IPG type: %d at bi=%d\n", w, ipg_opcode_idx);
        w1 = buf[bi++];
        w2 = buf[bi++];
        w3 = buf[bi++];
        break;
    }
    }
    return bi;
}

// Forward declaration to support recursive parsing
int siobj_parse_collect_internal(int16_t *buf, int bi, SIVERTEX *verts, int *vert_mapping, int block_end = -1, bool stop_on_return = false);

// Main entry point
int siobj_parse_collect(int16_t *buf, int bi) {
    SIVERTEX verts[2000];
    int vert_mapping[2000];
    int i;
    
    // Clear vertices
    for (i = 0; i < 2000; i++) {
        verts[i].flags = false;
        vert_mapping[i] = -1;
    }
    
    return siobj_parse_collect_internal(buf, bi, verts, vert_mapping, -1, false);
}

// Internal implementation with support for nested calls
// block_end: -1 for main stream (no limit), or absolute position where block ends
int siobj_parse_collect_internal(int16_t *buf, int bi, SIVERTEX *verts, int *vert_mapping, int block_end, bool stop_on_return) {
    int16_t w, w1, w2, w3, w4, w5;
    int i;
    int num;
    int cur_color = 0;
    int opcode_count = 0;
    int safety_counter = 0;  // Prevent infinite loops

    // Track IPG vertices created inside this nested context? For simplicity, we use global array
    // and clear it at the top-level only. That happens in siobj_parse_collect().

    while (1) {
        // Check block boundary
        if (block_end >= 0 && bi >= block_end) {
//            write_debug("Reached block_end at bi=%d, exiting\n", bi);
            return bi;
        }
        
        safety_counter++;
        if (safety_counter > 100000) {
            write_debug("ERROR: Safety counter exceeded - possible infinite loop in parser\n");
            return bi;
        }
        
        w = buf[bi++];
        int opcode_idx = bi - 1;
        opcode_count++;
        
        // Reduced logging verbosity for production
        if (opcode_count <= 3 || w == OP_IFVIS || w == OP_SORT || w == OP_RETURN) {
            // Log important control opcodes or start of blocks
            // write_debug("Opcode #%d: %04x at index %d\n", opcode_count, w, bi-1);
        }

        switch (w) {
        case OP_VECTORS: {
            int vectors_opcode_idx = bi - 1;
            w = buf[bi++];            // number of verts
            w1 = buf[bi++] / 0xc;     // start register
//             write_debug("DEBUG VECTORS at bi=%d: count=%d, start_reg=%d\n", vectors_opcode_idx, w, w1);
            for (i = 0; i < w; i++) {
                w2 = buf[bi++];
                w3 = buf[bi++];
                w4 = buf[bi++];
                verts[w1] = transform_input_vertex(w2, w3, w4);
                verts[w1].flags = true;
                if (i < 5) { // log first few
//                    write_debug("  VECTORS: reg=%d <- (%d,%d,%d)\n", w1, w2, w3, w4);
                }
                w1++;
            }
            break;
        }

        case OP_IPG: {
            int ipg_opcode_idx = bi - 1;
//             write_debug("DEBUG IPG opcode at bi=%d\n", ipg_opcode_idx);
            bi = siobj_parse_ipg(buf, bi, verts, vert_mapping);
            break;
        }

        case OP_BLOKNEG: {
            int count = buf[bi++];
            int dest = buf[bi++] / 0xc;
            int src = buf[bi++] / 0xc;
//             write_debug("DEBUG BLOKNEG: count=%d, dest_reg=%d, src_reg=%d\n", count, dest, src);
            for (int j = 0; j < count; j++) {
                if (src + j >= 0 && src + j < 2000 && dest + j >= 0 && dest + j < 2000 && verts[src + j].flags) {
                    set_vert(&verts[dest + j], -verts[src + j].x, -verts[src + j].y, -verts[src + j].z);
                    if (j < 3) { // log first few
                        write_debug("  Copy: dest[%d] = -src[%d] = (%d,%d,%d)\n",
                                  dest+j, src+j, verts[dest+j].x, verts[dest+j].y, verts[dest+j].z);
                    }
                }
            }
            break;
        }

        case OP_BLOKADD: {
            int count = buf[bi++];
            int dest = buf[bi++] / 0xc;
            int src = buf[bi++] / 0xc;
//             write_debug("DEBUG BLOKADD: count=%d, dest_reg=%d, src_reg=%d, g_cur_vert=(%d,%d,%d)\n",
//                       count, dest, src, g_cur_vert.x, g_cur_vert.y, g_cur_vert.z);
            for (int j = 0; j < count; j++) {
                if (src + j >= 0 && src + j < 2000 && dest + j >= 0 && dest + j < 2000 && verts[src + j].flags) {
                    int new_x = verts[src + j].x + g_cur_vert.x;
                    int new_y = verts[src + j].y + g_cur_vert.y;
                    int new_z = verts[src + j].z + g_cur_vert.z;
                    set_vert(&verts[dest + j], new_x, new_y, new_z);
                    if (j < 3) {
                        write_debug("  Copy: dest[%d] = src[%d] + cur_vert = (%d+%d=%d, %d+%d=%d, %d+%d=%d)\n",
                                  dest+j, src+j,
                                  verts[src+j].x, g_cur_vert.x, new_x,
                                  verts[src+j].y, g_cur_vert.y, new_y,
                                  verts[src+j].z, g_cur_vert.z, new_z);
                    }
                }
            }
            break;
        }

        case OP_GETV: {
            w = buf[bi++] / 0xc;
            get_vert(&g_cur_vert, &verts[w]);
            break;
        }

        case OP_PUTV: {
            w = buf[bi++] / 0xc;
//             write_debug("DEBUG PUTV: reg=%d, from cur_vert=(%d,%d,%d)\n", w, g_cur_vert.x, g_cur_vert.y, g_cur_vert.z);
            get_vert(&verts[w], &g_cur_vert);
            verts[w].flags = true;
            break;
        }

        case OP_INTV: {
            w = buf[bi++] / 0xc;
            mid_vert(&g_cur_vert, &verts[w]);
            break;
        }

        case OP_ADDV: {
            w = buf[bi++] / 0xc;
            add_vert(&g_cur_vert, &verts[w]);
            break;
        }

        case OP_SUBV: {
            w = buf[bi++] / 0xc;
            sub_vert(&g_cur_vert, &verts[w]);
            break;
        }

        case OP_SHRV: {
            w = buf[bi++];
            shr_vert(&g_cur_vert, w);
            break;
        }

        case OP_SHLV: {
            w = buf[bi++];
            shl_vert(&g_cur_vert, w);
            break;
        }

        case OP_NEGV: {
            neg_vert(&g_cur_vert);
            break;
        }

        case OP_CLRV: {
            set_vert(&g_cur_vert, 0, 0, 0);
            break;
        }

        case OP_POLYZ: {
            int poly_opcode_idx = bi - 1;
            num = buf[bi++];  // num vertices
            if (num > 100 || num < 0) {
                write_debug("WARNING: POLYZ with suspicious num=%d\n", num);
                break;
            }
            std::vector<int> poly_verts;
            std::vector<int> raw_regs;
            int ipg_ref_count = 0;
            for (i = 0; i < num; i++) {
                w1 = buf[bi++] / 0xc;
                if (w1 >= 0 && w1 < 2000 && verts[w1].flags) {
                    raw_regs.push_back(w1);
                    if (ipg_vertices[w1]) ipg_ref_count++;
                    // Log vertex reference details for first few polygons
                    if (poly_verts.size() < 5) {
//                         write_debug("DEBUG POLYZ vertex: reg=%d, coords=(%d,%d,%d), flags=%d\n",
//                                   w1, verts[w1].x, verts[w1].y, verts[w1].z, verts[w1].flags);
                    }
                    poly_verts.push_back(append_transformed_vertex(verts[w1]));
                } else {
                    if (w1 >= 0 && w1 < 2000) {
//                         write_debug("DEBUG POLYZ vertex: reg=%d, flags=%d (not valid)\n", w1, verts[w1].flags);
                    }
                }
            }
            if (poly_verts.size() >= 3) {
                current_obj.faces.push_back(poly_verts);
                current_obj.face_colors.push_back(g_cur_color); // Add face color
//                 write_debug("DEBUG POLYZ at bi=%d: color=%d, requested=%d, got=%d, IPG_refs=%d\n",
//                           poly_opcode_idx, g_cur_color, num, (int)poly_verts.size(), ipg_ref_count);
            }
            break;
        }

        case OP_GVPOLY:
        case OP_GVPOLY16: {
            int gpoly_opcode_idx = bi - 1;
            num = buf[bi++];
            if (num > 100 || num < 0) {
                write_debug("WARNING: GVPOLY with suspicious num=%d\n", num);
                // Try to stay in sync if possible, but this is a desync
                break;
            }
            std::vector<int> gpoly_verts;
            std::vector<int> gpoly_regs;
            int ipg_ref_count = 0;
            for (i = 0; i < num; i++) {
                w1 = buf[bi++] / 0xc;
                if (w1 >= 0 && w1 < 2000 && verts[w1].flags) {
                    gpoly_regs.push_back(w1);
                    if (ipg_vertices[w1]) ipg_ref_count++;
                    // Log first few vertex details
                    if (gpoly_verts.size() < 5) {
//                         write_debug("DEBUG GVPOLY vertex: reg=%d, coords=(%d,%d,%d), flags=%d\n",
//                                   w1, verts[w1].x, verts[w1].y, verts[w1].z, verts[w1].flags);
                    }
                    gpoly_verts.push_back(append_transformed_vertex(verts[w1]));
                } else {
                    if (w1 >= 0 && w1 < 2000) {
//                         write_debug("DEBUG GVPOLY vertex: reg=%d, flags=%d (not valid)\n", w1, verts[w1].flags);
                    }
                }
            }
            // color/material parameter (encoded as register * 0xc)
            w = buf[bi++];
            int local_color = (w >> 8) & 0xFF; // Extract local color index from high byte

            // Skip shading data: 1 word per vertex (confirmed by bytecode analysis)
            for (i = 0; i < num; i++) {
                bi += 1; // Skip 1 word per vertex
            }
            if (gpoly_verts.size() >= 3) {
                current_obj.faces.push_back(gpoly_verts);
                // Use local color if available, otherwise global current color
                current_obj.face_colors.push_back(local_color ? local_color : g_cur_color);
//                 write_debug("DEBUG GVPOLY at bi=%d: color=%d (local=%d), requested=%d, got=%d, IPG_refs=%d\n",
//                           gpoly_opcode_idx, g_cur_color, local_color, num, (int)gpoly_verts.size(), ipg_ref_count);
            }
            break;
        }

        // Generate approximate geometry for spheres and disks
        case OP_SPHERE: {
            w1 = buf[bi++] / 0xc;  // center vertex register
            w2 = buf[bi++];        // radius in SOD units

            // Add the referenced vertex if valid and mark it as used
            if (w1 >= 0 && w1 < 2000 && verts[w1].flags) {
                // Pass raw SOD radius, add_sphere_geometry handles the geometry generation in SOD units
                add_sphere_geometry(verts[w1], w2, g_cur_color, current_obj); // Pass g_cur_color
            }
            break;
        }

        case OP_BLOCK: {
            w1 = buf[bi++];
            w2 = buf[bi++];
            current_obj.skipped_blocks++;
            break;
        }

        case OP_PERFDISK: {
            w1 = buf[bi++] / 0xc;  // center vertex register
            w2 = buf[bi++] / 0xc;  // normal/orientation vertex register
            w3 = buf[bi++];        // radius in SOD units

            // Add the referenced vertices if valid
            if (w1 >= 0 && w1 < 2000 && w2 >= 0 && w2 < 2000 && verts[w1].flags && verts[w2].flags) {
                // Apply scaling logic: radius is scaled by the plane's scale factor
                int scaled_radius = (int)(w3 * current_obj.scale);
                add_disk_geometry(verts[w1], verts[w2], scaled_radius, g_cur_color, current_obj); // Pass g_cur_color
            }
            break;
        }

        case OP_DISC: {
            w1 = buf[bi++] / 0xc;  // center vertex register
            w2 = buf[bi++];        // radius in SOD units

            // Add the referenced vertex if valid
            if (w1 >= 0 && w1 < 2000 && verts[w1].flags) {
                // Simplified: use default normal [0,0,1] for simple disk
                SIVERTEX normal;
                normal.x = 0; normal.y = 0; normal.z = 32767;
                // Apply scaling logic: radius is scaled by the plane's scale factor
                int scaled_radius = (int)(w2 * current_obj.scale);
                add_disk_geometry(verts[w1], normal, scaled_radius, g_cur_color, current_obj); // Pass g_cur_color
            }
            break;
        }

        case OP_COLOR: {
            w = buf[bi++];
            g_cur_color = w; // Update global current color
            break;
        }

        case OP_DOT: {
            w = buf[bi++];
            current_obj.skipped_dots++;
            break;
        }

        case OP_PERS: {
            w = buf[bi++];
            w1 = buf[bi++];
            break;
        }

        case OP_SHADE16: {
            // 16-bit shading control - sets lighting/shading parameters
            w = buf[bi++];
            w1 = buf[bi++];
            break;
        }

        case OP_SHADE1: {
            // Single vertex shading control
            w = buf[bi++];
            w1 = buf[bi++];
            break;
        }

        case OP_MOVETO: {
            w = buf[bi++];
            break;
        }

        case OP_ZLINETO:
        case OP_LINECOL: {
            w = buf[bi++];
            break;
        }

        case OP_PUSHRELMAT:
        {
            g_relmat_stack.push_back(g_relmat);
            break;
        }

        case OP_POPRELMAT: {
            if (!g_relmat_stack.empty()) {
                g_relmat = g_relmat_stack.back();
                g_relmat_stack.pop_back();
            }
            break;
        }

        case OP_ANIMATE: {
            // Sprite animation opcode. It does not contribute mesh geometry to
            // OBJ export, but it does carry a variable-length payload that must
            // be skipped correctly to keep the SOD stream in sync.
            w1 = buf[bi++]; // point register
            w2 = buf[bi++]; // radius / 2
            w3 = buf[bi++]; // variable offset
            w4 = buf[bi++]; // sprite count
            for (int j = 0; j < w4; j++) {
                bi++;
            }
            break;
        }

        case OP_ROTX: {
            w = buf[bi++];
            if (w >= 0 && w < (int)(sizeof(g_variables) / sizeof(g_variables[0])) * 2) {
                apply_rotation_x(g_variables[w / 2]);
            }
            break;
        }

        case OP_ROTY: {
            w = buf[bi++];
            if (w >= 0 && w < (int)(sizeof(g_variables) / sizeof(g_variables[0])) * 2) {
                apply_rotation_y(g_variables[w / 2]);
            }
            break;
        }

        case OP_ROTZ: {
            w = buf[bi++];
            if (w >= 0 && w < (int)(sizeof(g_variables) / sizeof(g_variables[0])) * 2) {
                apply_rotation_z(g_variables[w / 2]);
            }
            break;
        }

        case OP_SCALEIT: {
            w = buf[bi++];
            break;
        }

        case OP_VEC: {
            w1 = buf[bi++];
            w2 = buf[bi++];
            w3 = buf[bi++];
            break;
        }

        case OP_SHADES: {
            w = buf[bi++];
            w = buf[bi++];
            for (int j = 0; j < w; j++) {
                w1 = buf[bi++];
                w2 = buf[bi++];
                w3 = buf[bi++];
            }
            break;
        }

        case OP_SORT: {
            int opcode_idx = bi - 1;
            int16_t block_size_bytes = buf[bi++];
            // Skip two sort-specific parameters (not needed for geometry extraction)
            bi += 2;
            int block_end_abs = opcode_idx + (block_size_bytes / 2);
            int nested_start = bi;
            // Parse nested block; do not stop on RETURN, parse until block end
            bi = siobj_parse_collect_internal(buf, nested_start, verts, vert_mapping, block_end_abs, false);
            // Ensure we advance to the block end if the nested parser returned early
            if (bi < block_end_abs) {
                bi = block_end_abs;
            }
            break;
        }

        case OP_GOSUB: {
            // Relative byte offset from the operand word to the subroutine entry.
            // The original assembly does:
            //   push si
            //   add si, es:[si]
            //   getword
            //   call ax
            // So the target is computed from the operand location itself.
            int16_t rel_bytes = buf[bi++];
            int target = (bi - 1) + (rel_bytes / 2);

            if (target >= 0) {
                siobj_parse_collect_internal(buf, target, verts, vert_mapping, -1, true);
            } else {
                write_debug("WARNING: GOSUB target before start of buffer: rel=%d at bi=%d\n", (int)rel_bytes, bi - 1);
            }
            break;
        }

        case OP_GOTO: {
            // `com_goto` adds the relative byte offset to the operand word
            // location, then continues execution from that target.
            int16_t rel_bytes = buf[bi++];
            int target = (bi - 1) + (rel_bytes / 2);
            if (target >= 0) {
                bi = target;
            } else {
                write_debug("WARNING: GOTO target before start of buffer: rel=%d at bi=%d\n", (int)rel_bytes, bi - 1);
            }
            break;
        }

        case OP_IFVIS: {
            // IFVIS: conditional visibility block
            int flags = buf[bi++];
            int size_bytes = buf[bi++];
            int size_words = size_bytes / 2;  // total block size in words (including opcode)
            int block_end_abs = opcode_idx + size_words;  // absolute index after block
            int nested_start = bi;  // start of nested bytecode
//             write_debug("DEBUG IFVIS at bi=%d: flags=0x%04x, size_bytes=%d, block_end_abs=%d, nested_start=%d\n",
//                       opcode_idx, flags, size_bytes, block_end_abs, nested_start);
            // Parse nested content with block_end limit (do not stop on RETURN, parse whole block)
            if (nested_start < block_end_abs) {
                bi = siobj_parse_collect_internal(buf, nested_start, verts, vert_mapping, block_end_abs, false);
                // After parsing, bi should be >= block_end_abs (normally exactly block_end_abs)
                if (bi < block_end_abs) {
//                    write_debug("WARNING: IFVIS parser stopped early, skipping to block_end\n");
                    bi = block_end_abs;
                }
            } else {
//                 write_debug("DEBUG IFVIS: block empty, skipping\n");
                bi = block_end_abs;
            }
//             write_debug("DEBUG IFVIS: after block, bi=%d\n", bi);
            break;
        }

        case OP_PUSHCENT: {
            g_relpos_stack.push_back(g_relpos_x);
            g_relpos_stack.push_back(g_relpos_y);
            g_relpos_stack.push_back(g_relpos_z);
            break;
        }

        case OP_POPCENT: {
            if (g_relpos_stack.size() >= 3) {
                g_relpos_z = g_relpos_stack.back();
                g_relpos_stack.pop_back();
                g_relpos_y = g_relpos_stack.back();
                g_relpos_stack.pop_back();
                g_relpos_x = g_relpos_stack.back();
                g_relpos_stack.pop_back();
            }
            break;
        }

        case OP_MOVECENT: {
            // Add the referenced vertex to the current relative origin.
            w = buf[bi++] / 0xc;
            if (w >= 0 && w < 2000 && verts[w].flags) {
                g_relpos_x += verts[w].x;
                g_relpos_y += verts[w].y;
                g_relpos_z += verts[w].z;
            }
            break;
        }

        case OP_BVLT:
        case OP_BVGT:
        case OP_BZLT: {
            w1 = buf[bi++];            // variable or point register
            w2 = buf[bi++];            // comparison value
            int16_t rel_bytes = buf[bi++]; // relative byte offset from this word
            bool take_branch = false;

            if (w == OP_BVLT) {
                int var_idx = w1 / 2;
                if (var_idx >= 0 && var_idx < (int)(sizeof(g_variables) / sizeof(g_variables[0]))) {
                    take_branch = (g_variables[var_idx] <= (uint16_t)w2);
                }
            } else if (w == OP_BVGT) {
                int var_idx = w1 / 2;
                if (var_idx >= 0 && var_idx < (int)(sizeof(g_variables) / sizeof(g_variables[0]))) {
                    take_branch = (g_variables[var_idx] >= (uint16_t)w2);
                }
            } else {
                int reg = w1 / 0xc;
                if (reg >= 0 && reg < 2000 && verts[reg].flags) {
                    // `com_bzlt` scales the immediate by `shift` and compares it
                    // against the current point's local Z after subtracting relpos[z].
                    // This decoder does not yet model `shift`, so use the raw
                    // signed immediate as the closest available approximation.
                    int cmp_z = (int16_t)w2;
                    take_branch = (verts[reg].z <= (cmp_z - g_relpos_z));
                }
            }

            if (take_branch) {
                bi = (bi - 1) + (rel_bytes / 2);
            }
            break;
        }

        case OP_REJ: {
            int face_base = buf[bi++];
            int count = buf[bi++];
            for (int j = 0; j < count; j++) {
                int reg_a = buf[bi++] / 0xc;
                int reg_d = buf[bi++] / 0xc;
                int reg_b = buf[bi++] / 0xc;
            //    int face_idx = face_base + j;

            //    if (face_idx >= 0 && face_idx < (int)g_face_visibility.size() &&
            //        reg_a >= 0 && reg_a < 2000 &&
            //        reg_d >= 0 && reg_d < 2000 &&
            //        reg_b >= 0 && reg_b < 2000 &&
            //        verts[reg_a].flags && verts[reg_d].flags && verts[reg_b].flags) {
            //        g_face_visibility[face_idx] =
            //            (int8_t)compute_face_visibility_3d(verts[reg_a], verts[reg_d], verts[reg_b],
            //                                               g_relpos_x, g_relpos_y, g_relpos_z);
            //    } else if (face_idx >= 0 && face_idx < (int)g_face_visibility.size()) {
            //        g_face_visibility[face_idx] = 0;
            //    }
            }
            break;
        }

        case OP_RETURN:
//            write_debug("OP_RETURN at opcode %d (bi=%d, block_end=%d, stop_on_return=%d)\n", opcode_count, bi, block_end, stop_on_return);
            if (stop_on_return) {
//                write_debug("  -> returning due to stop_on_return\n");
                return bi;
            }
            // If we're in a nested block, don't necessarily exit.
            // Some blocks contain multiple return-terminated sections.
            // We'll continue until the block boundary is reached.
            if (block_end < 0) {
                return bi;
            }
            break;

        default:
            if (opcode_count <= 5 || w == 0) {
//                write_debug("Unknown/unhandled opcode %04x at index %d (opcode #%d)\n", w, bi-1, opcode_count);
            }
            
            // In nested blocks or main stream, try to skip one word
            // and continue. This is safer for recovering geometry even
            // if we hit something unknown.
            bi++;
            
            if (w == 0) {
                write_debug("WARNING: Null opcode encountered, may indicate end or corruption\n");
                if (block_end >= 0) return block_end;
                return bi;
            }
            break;
        }
    }
    return bi;
}

bool siobj_parse_collect(Buffer &buf) {
    int16_t w;
    int i;

    // Read SIOBJ header
    w = buf.NextWord();
    if (w != OP_ID) {
        write_debug("ERROR: Id not found:%x at offset %x\n", w, buf.pos - 2);
        return false;
    }

    w = buf.NextWord();  // shapesize (unknown/unused - FreeFlight just reads and ignores)

    w = buf.NextWord();  // Y off ground
    current_obj.y_offset = w;
//    write_debug("Y Offset: %d\n", current_obj.y_offset);

    w = buf.NextWord();  // scale and flags
    int flags = w >> 8;
    w = w & 0xff;
    if (w > 17) {
        write_debug("Warning: Scale out of range:%d\n", w);
    }
    current_obj.scale = scale_table[w] / 10.0f;
//    write_debug("Scale index: %d (scale factor: %f)\n", (int)w, current_obj.scale);

    w = buf.NextWord();  // collision

    // Clear vertices, faces, and face_colors for the new object
    current_obj.vertices.clear();
    current_obj.faces.clear();
    current_obj.face_colors.clear(); // Clear face colors
    g_face_visibility.fill(0);
    g_cur_color = 0; // Reset global current color
    set_vert(&g_cur_vert, 0, 0, 0);
    g_relpos_x = 0;
    g_relpos_y = 0;
    g_relpos_z = 0;
    g_relpos_stack.clear();
    g_relmat = {1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                0.0, 0.0, 1.0};
    g_relmat_stack.clear();
    memset(g_variables, 0, sizeof(g_variables));
    // Clear IPG vertex tracking
    std::fill(std::begin(ipg_vertices), std::end(ipg_vertices), false);

//    write_debug("Starting SOD parse at offset %x\n", buf.pos);

    // Call internal parser
    if (siobj_parse_collect((int16_t*)&buf.data[buf.pos], 0) == false) {
        write_debug("Error parsing SOD\n");
        return false;
    }
    
//     write_debug("DEBUG: Final collection: %d vertices referenced in polygons, %d faces with colors\n", 
//               (int)current_obj.vertices.size(), (int)current_obj.faces.size());
    
    return true;
}

// Function to write MTL file
void write_mtl(const char* obj_filename, const std::vector<int>& unique_colors) {
    std::string mtl_filename = obj_filename;
    size_t dot_pos = mtl_filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        mtl_filename = mtl_filename.substr(0, dot_pos);
    }
    mtl_filename += ".mtl";

    FILE* f = fopen(mtl_filename.c_str(), "w");
    if (!f) {
        write_debug("ERROR: Could not open MTL file %s for writing\n", mtl_filename.c_str());
        return;
    }

    for (int color_idx : unique_colors) {
        if (color_idx < 0 || color_idx >= 256) continue; // Ensure color index is valid
        
        fprintf(f, "newmtl material_%d\n", color_idx);
        fprintf(f, "Kd %f %f %f\n", 
                (float)sipal[color_idx*3]/255.0f, 
                (float)sipal[color_idx*3+1]/255.0f, 
                (float)sipal[color_idx*3+2]/255.0f);
        fprintf(f, "Ka %f %f %f\n", // Ambient color (same as diffuse for simplicity)
                (float)sipal[color_idx*3]/255.0f, 
                (float)sipal[color_idx*3+1]/255.0f, 
                (float)sipal[color_idx*3+2]/255.0f);
    }

    fclose(f);
}

// Export functions
void export_obj(const ParsedObject& obj, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "# Exported from Stunt Pilot plane decoder\n");
    fprintf(f, "# Scale: %f, Y offset: %d\n", obj.scale, obj.y_offset);
    fprintf(f, "# Vertices: %d, Faces: %d\n", (int)obj.vertices.size(), (int)obj.faces.size());

    // Generate MTL file
    std::set<int> unique_colors_set;
    for (int color_idx : obj.face_colors) {
        unique_colors_set.insert(color_idx);
    }
    std::vector<int> unique_colors(unique_colors_set.begin(), unique_colors_set.end());
    std::sort(unique_colors.begin(), unique_colors.end()); // Sort for consistent MTL output

    write_mtl(filename, unique_colors); // Write MTL file using the OBJ filename as base
    fprintf(f, "mtllib %s.mtl\n", filename); // Reference the MTL file in OBJ

    // Write vertices - scale up by 1000 for better visibility
    float scale_mult = obj.scale * 1000.0f;
//     write_debug("DEBUG: Exporting with scale_mult = %f\n", scale_mult);
    
    for (int i = 0; i < (int)obj.vertices.size() && i < 5; i++) {
        const auto& v = obj.vertices[i];
        float x = v.x * scale_mult;
        float y = (v.y * scale_mult) + (obj.y_offset * scale_mult);
        float z = v.z * scale_mult;
//         write_debug("DEBUG: Vert %d: raw(%d,%d,%d) -> scaled(%f,%f,%f)\n", i, v.x, v.y, v.z, x, y, z);
    }

    for (const auto& v : obj.vertices) {
        if (v.flags) {
            float x = v.x * scale_mult;
            float y = (v.y * scale_mult) + (obj.y_offset * scale_mult);
            float z = v.z * scale_mult;
            fprintf(f, "v %f %f %f\n", x, y, z);
        }
    }

    // Write faces (OBJ uses 1-based indexing)
    int large_face_count = 0;
    int current_mtl_idx = -1;
    for (size_t i = 0; i < obj.faces.size(); ++i) {
        const auto& face = obj.faces[i];
        int face_color_idx = obj.face_colors[i];

        if (face.size() > 20) {
            large_face_count++;
//             write_debug("DEBUG: Large poly with %d vertices\n", (int)face.size());
        }

        // Change material only if different from previous face
        if (face_color_idx != current_mtl_idx) {
            fprintf(f, "usemtl material_%d\n", face_color_idx);
            current_mtl_idx = face_color_idx;
        }

        fprintf(f, "f");
        for (int idx : face) {
            fprintf(f, " %d", idx + 1);
        }
        fprintf(f, "\n");
    }
    
    if (large_face_count > 0) {
//         write_debug("DEBUG: Found %d polygons with >20 vertices\n", large_face_count);
    }

    fclose(f);
}

void export_json(const ParsedObject& obj, const char* filename) {
    // Intentionally left blank - JSON export disabled
}

// ============================================================================
// Library wrapper functions
// ============================================================================

int sod_get_res_count(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;
    
    // Read header (4 bytes minimum)
    uint8_t header[4];
    if (fread(header, 1, 4, f) != 4) {
        fclose(f);
        return -1;
    }
    fclose(f);
    
    // Check signature "RS"
    uint16_t sig = header[0] | (header[1] << 8);
    if (sig != 0x5352) {
        return -1;
    }
    
    // Byte 3 is the entry count
    return header[3];
}

int sod_load_res(const char* filename, std::vector<RESEntry>& entries, std::vector<uint8_t>& data) {
    Buffer buf;
    if (!buf.LoadFile(filename)) {
        return -1;
    }

    if (buf.data.size() < 8) {
        return -1;
    }

    // Check signature "RS"
    uint16_t sig = buf.NextWord();
    if (sig != 0x5352) {
        return -1;
    }

    // Read ID range and number of entries
    uint16_t id_high = buf.NextByte();
    uint16_t num = buf.NextByte();

    if (num <= 0 || num > 10000) {
        return -1;
    }

    // Determine offset table position
    uint32_t offset_pos = (id_high > 0x0F) ? 8 : 4;

    // Base ID
    uint16_t base_id = (id_high + 2) << 8;

    // Check we have enough data for the offset table
    if (offset_pos + num * 4 > buf.data.size()) {
        return -1;
    }

    // Copy data
    data = buf.data;

    // Read offsets
    entries.resize(num);
    uint32_t* offsets = (uint32_t*)&data[offset_pos];
    int valid_entries = 0;
    for (int i = 0; i < num; i++) {
        // Validate offset - must be within file bounds and after the offset table
        uint32_t offset = offsets[i];
        if (offset >= offset_pos + num * 4 && offset < buf.data.size()) {
            // Also check that the object starts with OP_ID (0x000C)
            uint16_t first_word = (data[offset] | (data[offset + 1] << 8));
            if (first_word == 0x000C) {  // OP_ID
                entries[valid_entries].id = base_id + i;
                entries[valid_entries].offset = offset;
                valid_entries++;
            }
        }
    }
    
    // Resize to actual valid entries
    entries.resize(valid_entries);

    return valid_entries;
}

ParsedObject sod_parse_object(const uint8_t* buf, uint32_t size) {
    // Reset global state
    current_obj = ParsedObject();
    std::fill(std::begin(ipg_vertices), std::end(ipg_vertices), false);
    for (int i = 0; i < 16; i++) g_variables[i] = 0;
    g_relpos_x = g_relpos_y = g_relpos_z = 0;
    g_relpos_stack.clear();
    g_relmat = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    g_relmat_stack.clear();
    g_cur_color = 0;
    g_cur_vert = SIVERTEX{0,0,0,0,0};

    // Create Buffer and copy data (Buffer owns the memory)
    Buffer buffer;
    buffer.data.resize(size);
    buffer.data.assign(buf, buf+size);
    buffer.pos = 0;

    // Parse SOD object directly (no RES header - objects are raw SOD bytecode)
    siobj_parse_collect(buffer);

    return current_obj;
}

void sod_export_mtl(const ParsedObject& obj, const char* filename) {
    // Get unique colors
    std::set<int> unique_colors;
    for (int c : obj.face_colors) {
        if (c >= 0 && c < 256) {
            unique_colors.insert(c);
        }
    }

    // Write MTL file
    std::string mtl_filename = filename;
    size_t dot_pos = mtl_filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        mtl_filename = mtl_filename.substr(0, dot_pos);
    }
    mtl_filename += ".mtl";

    FILE* f = fopen(mtl_filename.c_str(), "w");
    if (!f) {
        return;
    }

    for (int c : unique_colors) {
        uint8_t r = sipal[c * 3 + 0];
        uint8_t g = sipal[c * 3 + 1];
        uint8_t b = sipal[c * 3 + 2];
        fprintf(f, "newmtl material_%d\n", c);
        fprintf(f, "Kd %.6f %.6f %.6f\n", r/255.0f, g/255.0f, b/255.0f);
        fprintf(f, "Ka %.6f %.6f %.6f\n", r/255.0f, g/255.0f, b/255.0f);
    }
    fclose(f);
}

