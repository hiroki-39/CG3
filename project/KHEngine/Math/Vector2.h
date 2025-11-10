#pragma once
#include <cmath>

class Vector2
{
public:
	float x;
	float y;

	// --- コンストラクタ ---
	Vector2() : x(0), y(0) {}
	Vector2(float x, float y) : x(x), y(y) {}

	// --- 演算子 ---
	Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }
	Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }
	Vector2 operator*(float s) const { return { x * s, y * s }; }
	Vector2 operator/(float s) const { return { x / s, y / s }; }

	Vector2& operator+=(const Vector2& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	Vector2& operator-=(const Vector2& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vector2& operator*=(float s)
	{
		x *= s;
		y *= s;
		return *this;
	}

	Vector2& operator/=(float s)
	{
		x /= s;
		y /= s;
		return *this;
	}

	// --- ベクトル演算 ---

	// ベクトルの長さを計算
	float Length() const { return std::sqrt(x * x + y * y); }

	// 正規化
	Vector2 Normalize() const
	{
		float len = Length();
		return len > 0 ? Vector2(x / len, y / len) : Vector2(0, 0);
	}

	// 内積
	static float Dot(const Vector2& a, const Vector2& b) { return a.x * b.x + a.y * b.y; }
};
