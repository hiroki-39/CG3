#pragma once
#include <cmath>

// 度をラジアンに変換
inline float ToRadians(float degrees) { return degrees * (3.1415926535f / 180.0f); }

// ラジアンを度に変換
inline float ToDegrees(float radians) { return radians * (180.0f / 3.1415926535f); }

// 値をminからmaxの範囲にクランプする
inline float Clamp(float v, float min, float max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

