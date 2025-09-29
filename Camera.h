#pragma once
#pragma once
#include "Transform.h"

class Camera
{
public:
    void Update();
    Matrix4x4 GetViewMatrix() const;
    Matrix4x4 GetProjectionMatrix() const;

private:
    Transform transform_;
    float fovY_ = 0.45f;
    float aspectRatio_ = 16.0f / 9.0f;
};


