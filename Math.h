#pragma once

struct Vector2 final
{
	float x;
	float y;
};


struct Vector3 final
{
	float x;
	float y;
	float z;
};

struct Vector4 final
{
	float x;
	float y;
	float z;
	float w;
};

struct Matrix4x4 final
{
	float m[4][4];
};

class Math
{
public:
	Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

	Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	Matrix4x4 Inverse(const Matrix4x4& m);

	Matrix4x4 Transpose(const Matrix4x4& m);

	Matrix4x4 MakeIdentity();

	Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	Matrix4x4 MakeTranslationMatrix(const Vector3& translate);

	Matrix4x4 MakeRotateXMatrix(float radian);

	Matrix4x4 MakeRotateYMatrix(float radian);

	Matrix4x4 MakeRotateZMatrix(float radian);

	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	Matrix4x4 MakeOrthographicmatrix(float left, float top, float  right, float bottom, float nearClip, float farClip);

	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);


};

