#include <cstdio>
#include <cstdlib>

#include "raylib.h"
#include "raymath.h"

#include "camera.h"
#include "game_state.h"
#include "renderer.h"
#include "scene.h"
#include "selection.h"

// Main game entry point
int main(void)
{
    SetTraceLogLevel(TraceLogLevel::LOG_ERROR);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "SI Free Flight – Take 2");
    {
        SetTargetFPS(60);
        GameState state;

        // Track mouse state for LMB-based camera control
        Vector2 lastMousePos = GetMousePosition();
        bool mouseCaptured = false;
        bool isCapsActive = false;

        bool islandLoaded = state.island_.isLoaded();

        // Main game loop
        while (!WindowShouldClose()) {
            float deltaTime = GetFrameTime();

            if (IsKeyPressed(KEY_X))
                isCapsActive = not isCapsActive;

            bool isShifted =
                IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (isCapsActive)
                isShifted = not isShifted;

            // Update emissive material for pulsing glow effect
            state.renderer_.updateEmissiveMaterial(deltaTime);

            if (IsKeyPressed(KEY_L)) {
                state.cameraController_.toggleCursor();
            }
            if (IsKeyPressed(KEY_H)) {
                state.renderer_.toggleHud();
            }

            // Handle object picking when cursor is visible and user clicks
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                state.selection_.pickObject(state.cameraController_.getCamera(), state.scene_);
            }
            else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                state.selection_.clear();
            }

            // Mouse look handling
            Vector2 mousePos = GetMousePosition();

            Vector2 mouseDelta = {0, 0};
            mouseDelta = {mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y};
            lastMousePos = mousePos;

            // Update camera
            state.cameraController_.handleMouseLook(mouseDelta);
            state.cameraController_.update(deltaTime, isShifted);

            // Rendering
            BeginDrawing();
            {
                ClearBackground(SKYBLUE);

                BeginMode3D(state.cameraController_.getCamera());
                {
                    // Terrain
                    if (islandLoaded) {
                        DrawMesh(state.island_.raylibMesh, state.renderer_.getUnlitMaterial(), MatrixIdentity());
                    }
                    // Scenery (buildings, trees, etc.)
                    RenderStats stats = state.renderer_.render(state.scene_, state.selection_, state.cameraController_.getCamera().position);
                    (void) stats; // for debug
                }
                EndMode3D();

                state.renderer_.drawHUD(state);
                state.renderer_.drawObjectInfo(state.cameraController_,
                                        state.scene_,
                                        state.selection_);
            }
            EndDrawing();
        }
    } // destroy GameState- must happen before CloseWindow() to avoid assertions
    CloseWindow();

    return 0;
}
