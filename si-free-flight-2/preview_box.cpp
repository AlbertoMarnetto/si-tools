#include "preview_box.h"

#include "si_coords.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>

PreviewRenderer::PreviewRenderer()
    : valid(false)
    , width(0)
    , height(0)
{
    texture = {};
}

PreviewRenderer::~PreviewRenderer()
{
    // Note: unload() should be called explicitly before CloseWindow()
    // This is a safety check to prevent double-free
    if (valid && texture.id != 0) {
        UnloadRenderTexture(texture);
        texture = {};
        valid = false;
    }
}

PreviewRenderer::PreviewRenderer(PreviewRenderer&& other) noexcept
    : texture(other.texture)
    , valid(other.valid)
    , width(other.width)
    , height(other.height)
{
    other.valid = false;
    other.texture = {};
    other.width = 0;
    other.height = 0;
}

PreviewRenderer& PreviewRenderer::operator=(PreviewRenderer&& other) noexcept
{
    if (this != &other) {
        if (valid && texture.id != 0) {
            UnloadRenderTexture(texture);
        }
        texture = other.texture;
        valid = other.valid;
        width = other.width;
        height = other.height;
        other.valid = false;
        other.texture = {};
        other.width = 0;
        other.height = 0;
    }
    return *this;
}

void PreviewRenderer::init(int w, int h)
{
    if (valid && texture.id != 0) {
        UnloadRenderTexture(texture);
    }
    texture = LoadRenderTexture(w, h);
    valid = (texture.id != 0);
    width = w;
    height = h;
}

void PreviewRenderer::unload()
{
    if (valid && texture.id != 0) {
        UnloadRenderTexture(texture);
        texture = {};
        valid = false;
    }
}

void PreviewRenderer::drawPreview(const Selection& selection,
                                  const Camera& camera,
                                  int screenX,
                                  int screenY,
                                  const Material& material)
{
    Asset const* obj = selection.getSelectedObject();
    if (!obj || obj->mesh.vertexCount == 0 || !valid)
        return;

    // For the target preview, render the asset in a separate 3D world, in the
    // origin, and place a camera near it at the same angle as the player is wrt
    // the selected instance.

    float obj_size = Vector3Length(obj->bounds.max - obj->bounds.min);
    float distance = obj_size * 1.5;

    auto t = obj->transforms.at(selection.getSelectedInstanceIndex());
    Vector3 target{t.m12, t.m13, t.m14};
    Vector3 vecToTarget = target - camera.position;
    Vector3 forward = Vector3Normalize(vecToTarget);

    Vector3 cameraPos = Vector3Subtract((Vector3){0, 0, 0}, Vector3Scale(forward, distance));
    cameraPos.y += 2.0f;  // Slightly above center

    // Set up preview camera
    Camera3D previewCamera = {0};
    previewCamera.position = cameraPos;
    previewCamera.target = (Vector3){0, 0, 0};  // Look at origin where object is drawn
    previewCamera.up = (Vector3){0, 1, 0};
    previewCamera.fovy = 45.0f;
    previewCamera.projection = CAMERA_PERSPECTIVE;

    // Render to texture
    BeginTextureMode(texture);
    {
        ClearBackground(BLANK); // Transparent background

        BeginMode3D(previewCamera);
        {
            // Draw object centered at origin with emissive material
            DrawMesh(obj->mesh, material, MatrixIdentity());
        }
        EndMode3D();
    }
    EndTextureMode();

    // Draw preview texture to screen
    // Draw background
    DrawRectangle(screenX, screenY, width, height, (Color){30, 30, 40, 220});

    // Draw preview texture (flip Y since render textures are upside down)
    Rectangle source = {0, 0, (float) width, -(float) height};
    Rectangle dest = {(float) screenX, (float) screenY, (float) width, (float) height};
    DrawTexturePro(texture.texture, source, dest, (Vector2){0, 0}, 0.0f, WHITE);

    // Draw border around preview
    DrawRectangleLines(screenX, screenY, width, height, GOLD);

    char distText[30];
    snprintf(distText, sizeof(distText), "Dist: %.0f", toSiDistance(Vector3Length(vecToTarget)));
    DrawText(distText, screenX + 5, screenY + 5, 16, GOLD);
}
