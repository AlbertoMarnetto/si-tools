#pragma once

#include "raylib.h"
#include "preview_box.h"
#include "scene.h"
#include "selection.h"

class GameState;
class CameraController;

// Render statistics
struct RenderStats
{
    int totalDrawCalls;
    int totalInstances;
    int culledFrustum;
    int culledDistance;
};

// Renderer - handles all rendering operations
class Renderer
{
public:
    Renderer();
    ~Renderer();

    // Non-copyable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Initialize materials
    void initMaterials();

    // Update emissive material based on time (call each frame)
    void updateEmissiveMaterial(float deltaTime);

    // Cleanup all resources (call before CloseWindow())
    void cleanup();

    // Cleanup materials only (deprecated, use cleanup())
    void cleanupMaterials();

    // Get materials
    Material getDefaultMaterial() const { return defaultMaterial; }
    Material getUnlitMaterial() const { return unlitMaterial; }
    Material getEmissiveMaterial() const { return emissiveMaterial; }

    // Render the scene
    RenderStats render(Scene const& state, Selection const& selection, const Vector3& cameraPos);

    // Draw HUD
    void drawHUD(const GameState& gameState);

    // Draw object info panel (for selected object)
    void drawObjectInfo(const CameraController& cameraController,
                        const Scene& scene,
                        const Selection& selection);

    // Draw about screen
    void drawAbout();

    // Setup window icon and about texture from embedded data
    void setupEmbeddedIcon(const unsigned char* pngData, int size);

    // Preview renderer
    PreviewRenderer& getPreviewRenderer() { return previewRenderer; }

    void toggleHud() { shouldShowHud = not shouldShowHud; }
    void toggleAbout() { showAbout_ = not showAbout_; }
    bool isAboutActive() const { return showAbout_; }

private:
    Material defaultMaterial;
    Material unlitMaterial;
    Material emissiveMaterial;
    PreviewRenderer previewRenderer;
    float pulseTime; // Accumulated time for pulsing effect
    Texture2D aboutTexture;

    // Draw selected object highlight
    void drawSelectionHighlight(const Scene& scene, const Selection& selection);

    // Draw failed mesh markers (debug)
    void drawFailedMeshes();

    bool shouldShowHud;
    bool showAbout_;
};

