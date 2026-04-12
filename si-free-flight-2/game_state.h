#pragma once

#include "camera.h"
#include "island.h"
#include "renderer.h"
#include "scene.h"

// Forward declarations
class Scene;
class Renderer;
class CameraController;
class Selection;

// Main game state container
class GameState
{
public:
    GameState();
    ~GameState();

    IslandResource island_;
    Scene scene_;
    Renderer renderer_;
    CameraController cameraController_;
    Selection selection_;
};

