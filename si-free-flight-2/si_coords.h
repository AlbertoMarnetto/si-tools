#pragma once

#include "raylib.h"

/**
 * Coordinates as shown by Stunt Island's set editor
 */
struct SiCoords
{
    float north;
    float east;
    float alt;
};

SiCoords toSiCoords(Vector3 const& pos);
float toSiDistance(float dist);

float siYaw(float yaw);
