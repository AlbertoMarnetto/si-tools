#pragma once

#include "island.h"
#include "raylib.h"

/**
 * Handles camera state and input
 */
class CameraController
{
public:
    CameraController();
    ~CameraController() = default;

    void update(float deltaTime, bool isShifted);

    void handleMouseLook(Vector2 mouseDelta);
    void handleKeyboardRotation(float deltaTime, bool isShifted);
    void handleMovement(float deltaTime, bool isShifted);

    void pointToTarget();

    // Get the raylib camera
    Camera3D& getCamera() { return camera; }
    const Camera3D& getCamera() const { return camera; }

    // Get camera angles (radians)
    float getYaw() const { return camYaw; }
    float getPitch() const { return camPitch; }
    float getRoll() const { return camRoll; }

    // Get camera position
    Vector3 getPosition() const { return camera.position; }

    // Check if cursor should be visible
    bool isCursorVisible() const { return cursorVisible; }
    void toggleCursor();

    // Constants
    static constexpr float kRotationSpeed = 0.005f;
    static constexpr float kMoveSpeed = 2500.0f * kIslandScale;
    static constexpr float kMaxPitch = 0.499999f * PI;

private:
    Camera3D camera;

    // Euler angles (radians)
    float camYaw;   // Rotation about world Y axis
    float camPitch; // Rotation about local X axis
    float camRoll;  // Rotation about local Z axis

    // Mouse state
    Vector2 lastMousePos;
    bool mouseCaptured;
    bool cursorVisible;

    // Build camera basis vectors from Euler angles
    void updateOrientation();
};
