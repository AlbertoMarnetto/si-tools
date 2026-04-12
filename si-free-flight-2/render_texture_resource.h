#pragma once

#include "raylib.h"

// Forward declarations
class Scene;
class Renderer;
class CameraController;
class Selection;

// RAII wrapper for render texture
class RenderTextureResource
{
public:
    RenderTextureResource();
    ~RenderTextureResource();

    // Non-copyable, movable
    RenderTextureResource(const RenderTextureResource&) = delete;
    RenderTextureResource& operator=(const RenderTextureResource&) = delete;
    RenderTextureResource(RenderTextureResource&& other) noexcept;
    RenderTextureResource& operator=(RenderTextureResource&& other) noexcept;

    void init(int width, int height);
    void unload();

    RenderTexture2D texture;
    bool valid;
};

