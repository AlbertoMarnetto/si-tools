#include "render_texture_resource.h"
#include "raylib.h"
#include "rlgl.h"
#include <cstdlib>
#include <cstring>

// ============================================================================
// RenderTextureResource implementation
// ============================================================================

RenderTextureResource::RenderTextureResource()
    : valid(false)
{
    texture = {};
}

RenderTextureResource::~RenderTextureResource()
{
    unload();
}

RenderTextureResource::RenderTextureResource(RenderTextureResource&& other) noexcept
    : texture(other.texture)
    , valid(other.valid)
{
    other.valid = false;
    other.texture = {};
}

RenderTextureResource& RenderTextureResource::operator=(RenderTextureResource&& other) noexcept
{
    if (this != &other) {
        unload();
        texture = other.texture;
        valid = other.valid;
        other.valid = false;
        other.texture = {};
    }
    return *this;
}

void RenderTextureResource::init(int width, int height)
{
    unload();
    texture = LoadRenderTexture(width, height);
    valid = (texture.id != 0);
}

void RenderTextureResource::unload()
{
    if (valid && texture.id != 0) {
        UnloadRenderTexture(texture);
        texture = {};
        valid = false;
    }
}
