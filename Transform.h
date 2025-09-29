#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

struct Transform
{
    Vector3 position{ 0, 0, 0 };
    Vector3 rotation{ 0, 0, 0 };
    Vector3 scale{ 1, 1, 1 };

    Matrix4x4 GetWorldMatrix() const;
};

