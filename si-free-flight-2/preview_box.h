#pragma once

#include "raylib.h"
#include "selection.h"

// Preview renderer for selected object
class PreviewRenderer
{
public:
    PreviewRenderer();
    ~PreviewRenderer();

    // Non-copyable, movable
    PreviewRenderer(const PreviewRenderer&) = delete;
    PreviewRenderer& operator=(const PreviewRenderer&) = delete;
    PreviewRenderer(PreviewRenderer&& other) noexcept;
    PreviewRenderer& operator=(PreviewRenderer&& other) noexcept;

    void init(int width, int height);

    // Call before CloseWindow()
    void unload();

    // Render object preview
    void drawPreview(const Selection& selection,
                     const Camera& camera,
                     int screenX,
                     int screenY,
                     const Material& emissiveMaterial);

    bool isValid() const { return valid; }

private:
    RenderTexture2D texture;
    bool valid;
    int width;
    int height;
};

