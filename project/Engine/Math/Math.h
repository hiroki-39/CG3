#pragma once
#include <cmath>

struct Vector2
{
	float x, y;

	// Constructors
	Vector2() : x(0), y(0) {}
	Vector2(float x, float y) : x(x), y(y) {}

	// Operators
	Vector2 operator+(const Vector2& v) const
	{
		return { x + v.x, y + v.y };
	}
	Vector2 operator-(const Vector2& v) const
	{
		return { x - v.x, y - v.y };
	}
	Vector2 operator*(float s) const
	{
		return { x * s, y * s };
	}
	Vector2 operator/(float s) const
	{
		return { x / s, y / s };
	}
	Vector2& operator+=(const Vector2& v)
	{
		x += v.x; y += v.y; return *this;
	}
	Vector2& operator-=(const Vector2& v)
	{
		x -= v.x; y -= v.y; return *this;
	}

	float Length() const;
	void Normalize();
	float Dot(const Vector2& v) const;
};


struct Vector3
{
	float x, y, z;

	Vector3() : x(0), y(0), z(0) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	// Operators
	Vector3 operator+(const Vector3& v) const
	{
		return { x + v.x, y + v.y, z + v.z };
	}
	Vector3 operator-(const Vector3& v) const
	{
		return { x - v.x, y - v.y, z - v.z };
	}
	Vector3 operator*(float s) const
	{
		return { x * s, y * s, z * s };
	}
	Vector3 operator/(float s) const
	{
		return { x / s, y / s, z / s };
	}
	Vector3& operator+=(const Vector3& v)
	{
		x += v.x; y += v.y; z += v.z; return *this;
	}
	Vector3& operator-=(const Vector3& v)
	{
		x -= v.x; y -= v.y; z -= v.z; return *this;
	}

	float Length() const;
	void Normalize();
	float Dot(const Vector3& v) const;
	Vector3 Cross(const Vector3& v) const;
};


struct Vector4
{
	float x, y, z, w;

	Vector4() : x(0), y(0), z(0), w(0) {}
	Vector4(float x, float y, float z, float w) :x(x), y(y), z(z), w(w) {}

	float Dot(const Vector4& v) const;
	float Length() const;
	void Normalize();

	Vector4 operator+(const Vector4& v) const
	{
		return { x + v.x, y + v.y, z + v.z, w + v.w };
	}
	Vector4 operator-(const Vector4& v) const
	{
		return { x - v.x, y - v.y, z - v.z, w - v.w };
	}
	Vector4 operator*(float s) const
	{
		return { x * s, y * s, z * s, w * s };
	}
};


struct Matrix4x4
{
	float m[4][4];


	// Operators
	Matrix4x4 operator*(const Matrix4x4& rhs) const;
	Matrix4x4& operator*=(const Matrix4x4& rhs);
	Vector4 operator*(const Vector4& v) const;

	// 逆行列
	static Matrix4x4 Inverse(const Matrix4x4& m);

	// 転置行列
	static Matrix4x4 Transpose(const Matrix4x4& m);

	// 単位行列
	static Matrix4x4 MakeIdentity();

	// 平行移動行列
	static Matrix4x4 MakeTranslationMatrix(const Vector3& translate);
	
	// 拡大縮小行列
	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);
	
	// 回転行列
	static Matrix4x4 MakeRotateXMatrix(float  radian);
	static Matrix4x4 MakeRotateYMatrix(float  radian);
	static Matrix4x4 MakeRotateZMatrix(float  radian);
	
	// アフェイン変換行列
	static Matrix4x4 MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	//	透視射影行列
	static Matrix4x4 MakePerspectiveFov(float fovY, float aspectRatio, float nearClip, float farClip);

	// 正射影行列
	static Matrix4x4 MakeOrthographicmatrix(float left, float top, float  right, float bottom, float nearClip, float farClip);

	static Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
};

