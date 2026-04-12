#include "si_coords.h"
#include <cmath>

SiCoords toSiCoords(Vector3 const& pos)
{
    SiCoords result;
    result.north = 100000 + 100 * pos.z;
    result.east = 100000 - 100 * pos.x;
    result.alt = 100 * pos.y;

    return result;
}

float toSiDistance(float dist)
{
    return dist * 100.f;
}

float siYaw(float yaw)
{
    float result = fmod(yaw - 180.f, 360.f);
    while (result < 0.f)
        result += 360.f;
    return result;
}
