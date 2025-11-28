#pragma once
#include <cmath>

class Vector4
{
public:
    float x;
    float y;
    float z;
    float w;

    // --- コンストラクタ ---
    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // --- 演算子オーバーロード ---
    Vector4 operator+(const Vector4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
    Vector4 operator-(const Vector4& v) const { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
    Vector4 operator*(float s) const { return { x * s, y * s, z * s, w * s }; }
    Vector4 operator/(float s) const { return { x / s, y / s, z / s, w / s }; }

    Vector4& operator+=(const Vector4& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
	}

    Vector4& operator-=(const Vector4& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }

    Vector4& operator*=(float s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
	}

    Vector4& operator/=(float s)
    {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
	}


    // --- ベクトル演算 ---

	// ベクトルの長さを計算
    float Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }

	// 正規化
    Vector4 Normalize() const
    {
        float len = Length();
        if (len == 0) { return Vector4(0, 0, 0, 0); }
        return { x / len, y / len, z / len, w / len };
    }

	// 内積
    static float Dot(const Vector4& a, const Vector4& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
};
