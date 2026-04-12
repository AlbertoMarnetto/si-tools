#include "camera.h"
#include "raylib.h"
#include "raymath.h"

#include <iostream>

CameraController::CameraController()
    : camYaw(0)
    , camPitch(0)
    , camRoll(0)
    , lastMousePos{0, 0}
    , mouseCaptured(false)
    , cursorVisible(true)
{
    camera = {};

    camera.position = Vector3{100.0f, 100.0f, 100.0f};
    camera.target = Vector3{0.0f, 20.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Initialize camera angles from orientation
    pointToTarget();

    cursorVisible = true;
    toggleCursor();
}

void CameraController::pointToTarget()
{
    Vector3 delta = Vector3Subtract(camera.target, camera.position);
    Vector3 initForward = Vector3Normalize(delta);
    camYaw = atan2f(initForward.z, -initForward.x);
    camPitch = asinf(initForward.y);
    camRoll = 0.0f;
}

void CameraController::toggleCursor()
{
    cursorVisible = !cursorVisible;
    if (cursorVisible) {
        EnableCursor();
        ShowCursor();
    } else {
        HideCursor();
        DisableCursor();
    }
}

void CameraController::handleMouseLook(Vector2 mouseDelta)
{
    if (not IsCursorHidden())
        return;

    camYaw += mouseDelta.x * kRotationSpeed;
    camPitch -= mouseDelta.y * kRotationSpeed;

    // Clamp pitch
    if (camPitch > kMaxPitch)
        camPitch = kMaxPitch;
    if (camPitch < -kMaxPitch)
        camPitch = -kMaxPitch;

    updateOrientation();
}

void CameraController::handleKeyboardRotation(float deltaTime, bool isShifted)
{
    float rotSpeed = kRotationSpeed * 3.0f * (isShifted ? 0.1f : 1.0f);

    // Q/E -> Yaw
    if (IsKeyDown(KEY_Q))
        camYaw -= rotSpeed;
    if (IsKeyDown(KEY_E))
        camYaw += rotSpeed;

    // R/F -> Pitch
    if (IsKeyDown(KEY_R))
        camPitch += rotSpeed;
    if (IsKeyDown(KEY_F))
        camPitch -= rotSpeed;

    // Z/C -> Roll
    if (IsKeyDown(KEY_Z))
        camRoll -= rotSpeed;
    if (IsKeyDown(KEY_C))
        camRoll += rotSpeed;

    // Clamp pitch
    if (camPitch > kMaxPitch)
        camPitch = kMaxPitch;
    if (camPitch < -kMaxPitch)
        camPitch = -kMaxPitch;

    updateOrientation();
}

void CameraController::handleMovement(float deltaTime, bool isShifted)
{
    float moveSpeed = kMoveSpeed * (isShifted ? 1.0f : 0.1f);

    // Rebuild forward vector for movement
    float cosYaw = cosf(camYaw), sinYaw = sinf(camYaw);
    float cosPitch = cosf(camPitch), sinPitch = sinf(camPitch);

    Vector3 forward;
    forward.x = sinYaw * cosPitch;
    forward.y = sinPitch;
    forward.z = -cosYaw * cosPitch;
    forward = Vector3Normalize(forward);

    Vector3 right;
    right.x = cosYaw;
    right.y = 0;
    right.z = sinYaw;
    right = Vector3Normalize(right);

    // WASD movement
    if (IsKeyDown(KEY_W)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(forward, moveSpeed * deltaTime));
        camera.target = Vector3Add(camera.target, Vector3Scale(forward, moveSpeed * deltaTime));
    }
    if (IsKeyDown(KEY_S)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(forward, -moveSpeed * deltaTime));
        camera.target = Vector3Add(camera.target, Vector3Scale(forward, -moveSpeed * deltaTime));
    }
    if (IsKeyDown(KEY_A)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(right, -moveSpeed * deltaTime));
        camera.target = Vector3Add(camera.target, Vector3Scale(right, -moveSpeed * deltaTime));
    }
    if (IsKeyDown(KEY_D)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(right, moveSpeed * deltaTime));
        camera.target = Vector3Add(camera.target, Vector3Scale(right, moveSpeed * deltaTime));
    }
    if (IsKeyDown(KEY_T)) {
        camera.position.y += moveSpeed * deltaTime;
        camera.target.y += moveSpeed * deltaTime;
    }
    if (IsKeyDown(KEY_G)) {
        camera.position.y -= moveSpeed * deltaTime;
        camera.target.y -= moveSpeed * deltaTime;
    }
}

void CameraController::update(float deltaTime, bool isShifted)
{
    handleKeyboardRotation(deltaTime, isShifted);
    handleMovement(deltaTime, isShifted);
}

void CameraController::updateOrientation()
{
    float cosYaw = cosf(camYaw), sinYaw = sinf(camYaw);
    float cosPitch = cosf(camPitch), sinPitch = sinf(camPitch);
    float cosRoll = cosf(camRoll), sinRoll = sinf(camRoll);

    // Forward direction
    Vector3 forward;
    forward.x = sinYaw * cosPitch;
    forward.y = sinPitch;
    forward.z = -cosYaw * cosPitch;
    forward = Vector3Normalize(forward);

    // Right direction
    Vector3 right;
    right.x = cosYaw * cosRoll + sinYaw * sinPitch * sinRoll;
    right.y = -cosPitch * sinRoll;
    right.z = sinYaw * cosRoll - cosYaw * sinPitch * sinRoll;
    right = Vector3Normalize(right);

    // Up direction
    Vector3 up;
    up.x = cosYaw * sinRoll - sinYaw * sinPitch * cosRoll;
    up.y = cosPitch * cosRoll;
    up.z = sinYaw * sinRoll + cosYaw * sinPitch * cosRoll;
    up = Vector3Normalize(up);

    camera.target = Vector3Add(camera.position, forward);
    camera.up = up;
}
