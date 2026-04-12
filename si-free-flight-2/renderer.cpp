#include "renderer.h"

#include "camera.h"
#include "game_state.h"
#include "si_coords.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>

Renderer::Renderer()
{
    defaultMaterial = {};
    unlitMaterial = {};
    emissiveMaterial = {};
    pulseTime = 0.0f;
    shouldShowHud = true;

    rlSetClipPlanes(rlGetCullDistanceNear(), 35'000);
}

Renderer::~Renderer()
{
    cleanupMaterials();
}

void Renderer::initMaterials()
{
    defaultMaterial = LoadMaterialDefault();

    // Create an unlit material (uses vertex colors directly, no lighting)
    unlitMaterial = LoadMaterialDefault();
    unlitMaterial.maps[MATERIAL_MAP_EMISSION].color = (Color){255, 255, 255, 255};

    // Create emissive material for highlighted objects (yellow glow)
    emissiveMaterial = LoadMaterialDefault();
    emissiveMaterial.maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 255, 100, 255};
    emissiveMaterial.maps[MATERIAL_MAP_EMISSION].color = (Color){255, 200, 50, 255};

    // Initialize preview renderer
    previewRenderer.init(250, 250);
}

void Renderer::cleanupMaterials()
{
    if (defaultMaterial.maps)
        UnloadMaterial(defaultMaterial);
    if (unlitMaterial.maps)
        UnloadMaterial(unlitMaterial);
    if (emissiveMaterial.maps)
        UnloadMaterial(emissiveMaterial);
    defaultMaterial = {};
    unlitMaterial = {};
    emissiveMaterial = {};
}

void Renderer::cleanup()
{
    cleanupMaterials();
    previewRenderer.unload();
}

void Renderer::updateEmissiveMaterial(float deltaTime)
{
    // Accumulate time for smooth pulsing
    pulseTime += deltaTime;

    // Oscillate between bright yellow and dim orange
    // Period: ~1.5 seconds for obvious effect
    float t = 0.5 + 0.5 * sinf(pulseTime * 2.0f); // ranges from -1 to 1

    float intensity = 0.5f + 0.5f * t; // 0.5 to 1.0
    emissiveMaterial.maps[MATERIAL_MAP_DIFFUSE].color = (Color){(unsigned char) (255.0f),
                                                                (unsigned char) (255.0f * intensity),
                                                                (unsigned char) (50.0f * intensity),
                                                                255};
    emissiveMaterial.maps[MATERIAL_MAP_EMISSION].color = (Color){(unsigned char) (255.0f),
                                                                 (unsigned char) (200.0f *
                                                                                  intensity),
                                                                 (unsigned char) (50.0f * intensity),
                                                                 255};
}

RenderStats Renderer::render(Scene const& scene, Selection const& selection, const Vector3& cameraPos)
{
    // Initialize stats
    RenderStats stats;
    stats.totalDrawCalls = 0;
    stats.totalInstances = 0;
    stats.culledFrustum = 0;
    stats.culledDistance = 0;

    // Get view-projection matrix for frustum culling
    Matrix view = rlGetMatrixModelview();
    Matrix projection = rlGetMatrixProjection();
    Matrix viewProjection = MatrixMultiply(view, projection);

    const auto& uniqueObjects = scene.getAssets();
    int currentScreenHeight = GetScreenHeight();

    for (const Asset& obj : uniqueObjects) {
        if (obj.mesh.vertexCount == 0 || obj.transforms.empty())
            continue;

        for (size_t inst = 0; inst < obj.transforms.size(); inst++) {
            Matrix t = obj.transforms[inst];
            Vector3 pos = {t.m12, t.m13, t.m14};

            // Transform bounding box to world space
            BoundingBox worldBounds =
                {{obj.bounds.min.x + pos.x, obj.bounds.min.y + pos.y, obj.bounds.min.z + pos.z},
                 {obj.bounds.max.x + pos.x, obj.bounds.max.y + pos.y, obj.bounds.max.z + pos.z}};

            // Frustum culling
            if (!IsBoxInFrustum(&worldBounds, &viewProjection)) {
                stats.culledFrustum++;
                continue;
            }

            // Distance/size culling
            float screenSize = CalculateScreenSize(&worldBounds,
                                                            &pos,
                                                            obj.maxDimension,
                                                            &viewProjection,
                                                            currentScreenHeight,
                                                            &cameraPos);

            if (screenSize < MIN_SCREEN_SIZE_PIXELS) {
                stats.culledDistance++;
                continue;
            }

            // Skip selected instance - will draw separately with emissive material
            bool isSelected = (&obj == selection.getSelectedObject() &&
                               inst == selection.getSelectedInstanceIndex());
            if (!isSelected) {
                DrawMesh(obj.mesh, obj.material, t);
            }

            stats.totalDrawCalls++;
            stats.totalInstances++;
        }
    }

    // Draw selected object with emissive material
    drawSelectionHighlight(scene, selection);
    return stats;
}

void Renderer::drawSelectionHighlight(const Scene& scene, const Selection& selection)
{
    auto pSelectedObject = selection.getSelectedObject();
    if (not pSelectedObject)
        return;
    const Asset& selObj = *pSelectedObject;

    size_t selectedInstanceIndex = selection.getSelectedInstanceIndex();
    if (selectedInstanceIndex >= selObj.transforms.size())
        return;

    const Matrix& t = selObj.transforms[selectedInstanceIndex];
    DrawMesh(selObj.mesh, emissiveMaterial, t);

    // Draw highlight bounding boxes
    for (size_t inst = 0; inst < selObj.transforms.size(); inst++) {
        Matrix transform = selObj.transforms[inst];
        Vector3 center = {(selObj.bounds.min.x + selObj.bounds.max.x) * 0.5f + transform.m12,
                          (selObj.bounds.min.y + selObj.bounds.max.y) * 0.5f + transform.m13,
                          (selObj.bounds.min.z + selObj.bounds.max.z) * 0.5f + transform.m14};
        Vector3 size = {(selObj.bounds.max.x - selObj.bounds.min.x),
                        (selObj.bounds.max.y - selObj.bounds.min.y),
                        (selObj.bounds.max.z - selObj.bounds.min.z)};

        // Actually, the mesh is at 0
        center.y -= transform.m13;

        if (inst == selectedInstanceIndex) {
            // Selected instance: bright yellow glow
            DrawCubeWires(center, size.x + 0.3f, size.y, size.z + 0.3f, GOLD);
        } else {
            // Other instances of same type: light grey
            Color lightGrey = {240, 240, 240, 255};
            DrawCubeWires(center, size.x + 0.3f, size.y, size.z + 0.3f, lightGrey);
        }
    }
}

void Renderer::drawFailedMeshes()
{
    // Placeholder for failed mesh visualization (currently disabled in original)
}

void Renderer::drawHUD(GameState const& state)
{
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();

    // Centered crosshair
    int centerX = currentScreenWidth / 2;
    int centerY = currentScreenHeight / 2;
    int gap = 0;
    int length = 15;
    DrawLine(centerX - gap, centerY, centerX - gap - length, centerY, RED);
    DrawLine(centerX + gap, centerY, centerX + gap + length, centerY, RED);
    DrawLine(centerX, centerY - gap, centerX, centerY - gap - length, RED);
    DrawLine(centerX, centerY + gap, centerX, centerY + gap + length, RED);

    // Controls info
    if (shouldShowHud)
    {
        int y_pos_help_line = -15;
        auto nextLine = [&](){ y_pos_help_line += 25; return y_pos_help_line; };
        DrawText("LMB: select; RMB: deselect", 10, nextLine(), 20, DARKGRAY);
        DrawText("WASD: Move", 10, nextLine(), 20, DARKGRAY);
        DrawText("T/G: Up/Down", 10, nextLine(), 20, DARKGRAY);
        DrawText("Q/E: Yaw", 10, nextLine(), 20, DARKGRAY);
        DrawText("R/F: Pitch", 10, nextLine(), 20, DARKGRAY);
        DrawText("Z/C: Roll", 10, nextLine(), 20, DARKGRAY);
        DrawText("Shift: fast (X to invert speed)", 10, nextLine(), 20, DARKGRAY);
        DrawText("L: release mouse", 10, nextLine(), 20, DARKGRAY);
        DrawText("H: toggle aux infos", 10, nextLine(), 20, DARKGRAY);
        DrawText(TextFormat("FPS: %02i", GetFPS()), 10, currentScreenHeight - 30, 20, DARKGRAY);


        // Island info
        if (state.island_.isLoaded()) {
            const IslandMesh& mesh = state.island_.mesh;
            DrawText(TextFormat("Island: %d vertices, %d triangles",
                                mesh.num_vertices,
                                mesh.num_triangles),
                     10,
                     currentScreenHeight - 60,
                     20,
                     DARKGRAY);
        } else {
            DrawText("Island NOT loaded - check data/ folder", 10, currentScreenHeight - 60, 20, RED);
        }

        // Scene info
        DrawText(TextFormat("Unique meshes: %zu", state.scene_.getAssetCount()),
                 10,
                 currentScreenHeight - 90,
                 20,
                 DARKGRAY);

        Camera3D const& camera = state.cameraController_.getCamera();
        SiCoords siCoords = toSiCoords(camera.position);
        DrawText(TextFormat("N: %.0f, E: %.0f, Alt: %.0f",
                            siCoords.north,
                            siCoords.east,
                            siCoords.alt),
                 currentScreenWidth - 330,
                 10,
                 20,
                 DARKGRAY);

        DrawText(TextFormat("Roll: %+.1f Pitch: %+.1f Yaw: %+.1f",
                            state.cameraController_.getRoll() * (180.0f / PI),
                            state.cameraController_.getPitch() * (180.0f / PI),
                            siYaw(state.cameraController_.getYaw() * (180.0f / PI))),
                 currentScreenWidth - 330,
                 35,
                 20,
                 DARKGRAY);
    }
}

void Renderer::drawObjectInfo(const CameraController& cameraController,
                              const Scene& scene,
                              const Selection& selection)
{
    if (not selection.getSelectedObject())
        return;

    const Asset& selObj = *selection.getSelectedObject();

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Draw preview (top-right corner)
    int previewX = screenWidth - 270;
    int previewY = screenHeight - 400;

    previewRenderer.drawPreview(
        selection,
        cameraController.getCamera(),
        previewX,
        previewY,
        defaultMaterial);

    // Draw info below preview
    int infoX = previewX - 150;
    int infoY = previewY + 270;
    int lineHeight = 25;

    // Semi-transparent background
    DrawRectangle(infoX - 10, infoY - 10, 430, 145, (Color){0, 0, 0, 180});

    // Selection info
    DrawText("SELECTED OBJECT", infoX, infoY, 24, GOLD);
    // DrawText(TextFormat("Type: R%d I%02X", selObj.resFile, selObj.objIdx),
    DrawText(TextFormat("Type ID: %s",
                        selObj.name.c_str()), infoX, infoY + lineHeight, 20, WHITE);
    SiCoords siCoords = toSiCoords(selection.getWorldPosition());
    DrawText(TextFormat("Position: (N %.0f, E %.0f, Alt %.0f)",
                        siCoords.north,
                        siCoords.east,
                        siCoords.alt),
             infoX,
             infoY + lineHeight * 2,
             20,
             WHITE);
    DrawText(TextFormat("Instances: %zu", selObj.transforms.size()),
             infoX,
             infoY + lineHeight * 3,
             20,
             WHITE);

    siCoords = toSiCoords(selection.getHitPoint());
    DrawText(TextFormat("Click: (N %.0f, E %.0f, Alt %.0f)",
                        siCoords.north,
                        siCoords.east,
                        siCoords.alt),
             infoX,
             infoY + lineHeight * 4,
             20,
             WHITE);
}
