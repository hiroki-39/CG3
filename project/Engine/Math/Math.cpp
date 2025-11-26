#pragma once
#include <cmath>
#include <iostream>

namespace Math
{
	struct Vector2
	{
		float x, y;

		Vector2() : x(0), y(0) {}
		Vector2(float x_, float y_) : x(x_), y(y_) {}

		// --- 演算子 ---
		Vector2 operator+(const Vector2& rhs) const { return Vector2(x + rhs.x, y + rhs.y); }
		Vector2 operator-(const Vector2& rhs) const { return Vector2(x - rhs.x, y - rhs.y); }
		Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
		Vector2 operator/(float s) const { return Vector2(x / s, y / s); }

		Vector2& operator+=(const Vector2& rhs) { x += rhs.x; y += rhs.y; return *this; }
		Vector2& operator-=(const Vector2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
		Vector2& operator*=(float s) { x *= s; y *= s; return *this; }
		Vector2& operator/=(float s) { x /= s; y /= s; return *this; }

		Vector2 operator-() const { return Vector2(-x, -y); }

		float Length() const { return std::sqrt(x * x + y * y); }
		float LengthSquared() const { return x * x + y * y; }

		float Dot(const Vector2& rhs) const { return x * rhs.x + y * rhs.y; }
	};

	// スカラー * ベクトル
	inline Vector2 operator*(float s, const Vector2& v) { return v * s; }

	struct Vector3
	{
		float x, y, z;

		// --- コンストラクタ ---
		Vector3() : x(0), y(0), z(0) {}
		Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

		// --- 演算子 ---
		Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
		Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
		Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
		Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }

		Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
		Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
		Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
		Vector3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

		// --- 単項演算 ---
		Vector3 operator-() const { return Vector3(-x, -y, -z); }

		// --- ベクトル計算 ---
		float Length(const Vector3& v)
		{ 
			return std::sqrt(x * x + y * y + z * z);
		}
		
		float LengthSquared() const { return x * x + y * y + z * z; }
		
		struct Vector3 Normalize(const Vector3& v)
		{
			float length = Length(v);

			Vector3 result;

			if (length != 0)
			{
				result.x = v.x / length;
				result.y = v.y / length;
				result.z = v.z / length;
			}
			else
			{
				result.x = 0;
				result.y = 0;
				result.z = 0;
			}

			return result;
		}

		 float Dot(const Vector3& rhs) 
		 { 
			 return x * rhs.x + y * rhs.y + z * rhs.z;
		 }
		
		struct Vector3 Cross(const Vector3& rhs) const
		{
			return Vector3(
				y * rhs.z - z * rhs.y,
				z * rhs.x - x * rhs.z,
				x * rhs.y - y * rhs.x
			);
		}
	};

	// スカラー * ベクトル
	inline Vector3 operator*(float s, const Vector3& v) { return v * s; }

	struct Vector4
	{
		float x, y, z, w;

		Vector4() : x(0), y(0), z(0), w(0) {}
		Vector4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

		// --- 演算子 ---
		Vector4 operator+(const Vector4& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
		Vector4 operator-(const Vector4& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
		Vector4 operator*(float s) const { return Vector4(x * s, y * s, z * s, w * s); }
		Vector4 operator/(float s) const { return Vector4(x / s, y / s, z / s, w / s); }

		Vector4& operator+=(const Vector4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
		Vector4& operator-=(const Vector4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
		Vector4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
		Vector4& operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }

		Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }

		float Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
		float LengthSquared() const { return x * x + y * y + z * z + w * w; }

		float Dot(const Vector4& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
	};

	// スカラー * ベクトル
	inline Vector4 operator*(float s, const Vector4& v) { return v * s; }


	struct Matrix4x4
	{
		float m[4][4];

		Matrix4x4()
		{
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					m[i][j] = 0.0f;
		}

		// 演算子
		Matrix4x4 operator+(const Matrix4x4& rhs) const
		{
			Matrix4x4 res;
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					res.m[i][j] = m[i][j] + rhs.m[i][j];
			return res;
		}

		Matrix4x4 operator-(const Matrix4x4& rhs) const
		{
			Matrix4x4 res;
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					res.m[i][j] = m[i][j] - rhs.m[i][j];
			return res;
		}

		Matrix4x4 operator*(float s) const
		{
			Matrix4x4 res;
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					res.m[i][j] = m[i][j] * s;
			return res;
		}

		Matrix4x4 operator*(const Matrix4x4& rhs) const
		{
			Matrix4x4 res;
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					res.m[i][j] = 0;
					for (int k = 0; k < 4; k++)
						res.m[i][j] += m[i][k] * rhs.m[k][j];
				}
			}
			return res;
		}

		Matrix4x4& operator+=(const Matrix4x4& rhs) { *this = *this + rhs; return *this; }
		Matrix4x4& operator-=(const Matrix4x4& rhs) { *this = *this - rhs; return *this; }
		Matrix4x4& operator*=(float s) { *this = *this * s; return *this; }
		Matrix4x4& operator*=(const Matrix4x4& rhs) { *this = *this * rhs; return *this; }

		// Vector4 との掛け算
		Vector4 operator*(const Vector4& v) const
		{
			Vector4 res;
			res.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
			res.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
			res.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
			res.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
			return res;
		}

		// 転置
		Matrix4x4 Transpose(const Matrix4x4& m)
		{
			Matrix4x4 result;

			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					result.m[i][j] = m.m[j][i];
				}
			}

			return result;
		}

		// 単位行列を返す
		static Matrix4x4 MakeIdentity()
		{
			Matrix4x4 mat;
			for (int i = 0; i < 4; i++)
			{
				mat.m[i][i] = 1.0f;
			}

			return mat;
		}

		// 逆行列
		static Matrix4x4 Inverse(const Matrix4x4& m)
		{
			Matrix4x4 result;

			// 行列式を計算 → 逆行列が存在するか確認
			float det =
				m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] -
				m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0] + m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0] +
				m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] -
				m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] +
				m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] -
				m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] + m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] +
				m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2] -
				m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2] +
				m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2] - m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2] -
				m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] + m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] +
				m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] - m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] -
				m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] + m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3];

			// 行列式が0の場合、逆行列は存在しないのでそのままresultを返す
			if (det == 0.0f)
			{
				return result;
			}

			// 行列式が0でない場合、逆行列を計算

			float invDet = 1.0f / det;

			// 逆行列の計算式に従って各要素を計算	
			result.m[0][0] = invDet * (
				m.m[1][2] * m.m[2][3] * m.m[3][1] - m.m[1][3] * m.m[2][2] * m.m[3][1] +
				m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][3] * m.m[3][2] -
				m.m[1][2] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][2] * m.m[3][3]);

			result.m[0][1] = invDet * (
				m.m[0][3] * m.m[2][2] * m.m[3][1] - m.m[0][2] * m.m[2][3] * m.m[3][1] -
				m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][3] * m.m[3][2] +
				m.m[0][2] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][2] * m.m[3][3]);

			result.m[0][2] = invDet * (
				m.m[0][2] * m.m[1][3] * m.m[3][1] - m.m[0][3] * m.m[1][2] * m.m[3][1] +
				m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][3] * m.m[3][2] -
				m.m[0][2] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][2] * m.m[3][3]);

			result.m[0][3] = invDet * (
				m.m[0][3] * m.m[1][2] * m.m[2][1] - m.m[0][2] * m.m[1][3] * m.m[2][1] -
				m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][3] * m.m[2][2] +
				m.m[0][2] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][2] * m.m[2][3]);

			result.m[1][0] = invDet * (
				m.m[1][3] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][3] * m.m[3][0] -
				m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][3] * m.m[3][2] +
				m.m[1][2] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][2] * m.m[3][3]);

			result.m[1][1] = invDet * (
				m.m[0][2] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][2] * m.m[3][0] +
				m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][3] * m.m[3][2] -
				m.m[0][2] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][2] * m.m[3][3]);

			result.m[1][2] = invDet * (
				m.m[0][3] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[3][0] -
				m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][3] * m.m[3][2] +
				m.m[0][2] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][2] * m.m[3][3]);

			result.m[1][3] = invDet * (
				m.m[0][2] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][2] * m.m[2][0] +
				m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] -
				m.m[0][2] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][2] * m.m[2][3]);

			result.m[2][0] = invDet * (
				m.m[1][1] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][1] * m.m[3][0] +
				m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][0] * m.m[2][3] * m.m[3][1] -
				m.m[1][1] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][1] * m.m[3][3]);

			result.m[2][1] = invDet * (
				m.m[0][3] * m.m[2][1] * m.m[3][0] - m.m[0][1] * m.m[2][3] * m.m[3][0] -
				m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][0] * m.m[2][3] * m.m[3][1] +
				m.m[0][1] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][1] * m.m[3][3]);

			result.m[2][2] = invDet * (
				m.m[0][1] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][1] * m.m[3][0] +
				m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][0] * m.m[1][3] * m.m[3][1] -
				m.m[0][1] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][1] * m.m[3][3]);

			result.m[2][3] = invDet * (
				m.m[0][3] * m.m[1][1] * m.m[2][0] - m.m[0][1] * m.m[1][3] * m.m[2][0] -
				m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][0] * m.m[1][3] * m.m[2][1] +
				m.m[0][1] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][1] * m.m[2][3]);

			result.m[3][0] = invDet * (
				m.m[1][2] * m.m[2][1] * m.m[3][0] - m.m[1][1] * m.m[2][2] * m.m[3][0] -
				m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][0] * m.m[2][2] * m.m[3][1] +
				m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[1][0] * m.m[2][1] * m.m[3][2]);

			result.m[3][1] = invDet * (
				m.m[0][1] * m.m[2][2] * m.m[3][0] - m.m[0][2] * m.m[2][1] * m.m[3][0] +
				m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][0] * m.m[2][2] * m.m[3][1] -
				m.m[0][1] * m.m[2][0] * m.m[3][2] + m.m[0][0] * m.m[2][1] * m.m[3][2]);

			result.m[3][2] = invDet * (
				m.m[0][2] * m.m[1][1] * m.m[3][0] - m.m[0][1] * m.m[1][2] * m.m[3][0] -
				m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][0] * m.m[1][2] * m.m[3][1] +
				m.m[0][1] * m.m[1][0] * m.m[3][2] - m.m[0][0] * m.m[1][1] * m.m[3][2]);

			result.m[3][3] = invDet * (
				m.m[0][1] * m.m[1][2] * m.m[2][0] - m.m[0][2] * m.m[1][1] * m.m[2][0] +
				m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][0] * m.m[1][2] * m.m[2][1] -
				m.m[0][1] * m.m[1][0] * m.m[2][2] + m.m[0][0] * m.m[1][1] * m.m[2][2]);

			return result;
		}

		// --- 平行移動行列 ---
		static Matrix4x4 MakeTranslationMatrix(const Vector3& translate)
		{
			Matrix4x4 result;
			result.m[0][0] = 1.0f;
			result.m[0][1] = 0.0f;
			result.m[0][2] = 0.0f;
			result.m[0][3] = 0.0f;
			result.m[1][0] = 0.0f;
			result.m[1][1] = 1.0f;
			result.m[1][2] = 0.0f;
			result.m[1][3] = 0.0f;
			result.m[2][0] = 0.0f;
			result.m[2][1] = 0.0f;
			result.m[2][2] = 1.0f;
			result.m[2][3] = 0.0f;
			result.m[3][0] = translate.x;
			result.m[3][1] = translate.y;
			result.m[3][2] = translate.z;
			result.m[3][3] = 1.0f;

			return result;
		}

		// --- スケーリング行列 ---
		static Matrix4x4 MakeScaleMatrix(const Vector3& scale)
		{
			Matrix4x4 result;
			result.m[0][0] = scale.x;
			result.m[0][1] = 0.0f;
			result.m[0][2] = 0.0f;
			result.m[0][3] = 0.0f;
			result.m[1][0] = 0.0f;
			result.m[1][1] = scale.y;
			result.m[1][2] = 0.0f;
			result.m[1][3] = 0.0f;
			result.m[2][0] = 0.0f;
			result.m[2][1] = 0.0f;
			result.m[2][2] = scale.z;
			result.m[2][3] = 0.0f;
			result.m[3][0] = 0.0f;
			result.m[3][1] = 0.0f;
			result.m[3][2] = 0.0f;
			result.m[3][3] = 1.0f;

			return result;
		}

		// --- 回転行列 ---
		static Matrix4x4 MakeRotateXMatrix(float  radian)
		{
			Matrix4x4 result;

			// cos(radian)とsin(radian)を計算
			float cosRadian = std::cosf(radian);
			float sinRadian = std::sinf(radian);

			// X軸回転行列を作成
			result.m[0][0] = 1.0f;
			result.m[0][1] = 0.0f;
			result.m[0][2] = 0.0f;
			result.m[0][3] = 0.0f;

			result.m[1][0] = 0.0f;
			result.m[1][1] = cosRadian;
			result.m[1][2] = sinRadian;
			result.m[1][3] = 0.0f;

			result.m[2][0] = 0.0f;
			result.m[2][1] = -sinRadian;
			result.m[2][2] = cosRadian;
			result.m[2][3] = 0.0f;

			result.m[3][0] = 0.0f;
			result.m[3][1] = 0.0f;
			result.m[3][2] = 0.0f;
			result.m[3][3] = 1.0f;

			// 行列を返す
			return result;
		}

		static Matrix4x4 MakeRotateYMatrix(float radian)
		{
			Matrix4x4 result;

			// cos(radian)とsin(radian)を計算
			float cosRadian = std::cosf(radian);
			float sinRadian = std::sinf(radian);

			// Y軸回転行列を作成
			result.m[0][0] = cosRadian;
			result.m[0][1] = 0.0f;
			result.m[0][2] = -sinRadian;
			result.m[0][3] = 0.0f;

			result.m[1][0] = 0.0f;
			result.m[1][1] = 1.0f;
			result.m[1][2] = 0.0f;
			result.m[1][3] = 0.0f;

			result.m[2][0] = sinRadian;
			result.m[2][1] = 0.0f;
			result.m[2][2] = cosRadian;
			result.m[2][3] = 0.0f;

			result.m[3][0] = 0.0f;
			result.m[3][1] = 0.0f;
			result.m[3][2] = 0.0f;
			result.m[3][3] = 1.0f;

			// 行列を返す
			return result;
		}

		static Matrix4x4 MakeRotateZMatrix(float radian)
		{
			Matrix4x4 result;

			// cos(radian)とsin(radian)を計算
			float cosRadian = std::cosf(radian);
			float sinRadian = std::sinf(radian);

			// Z軸回転行列を作成
			result.m[0][0] = cosRadian;
			result.m[0][1] = sinRadian;
			result.m[0][2] = 0.0f;
			result.m[0][3] = 0.0f;

			result.m[1][0] = -sinRadian;
			result.m[1][1] = cosRadian;
			result.m[1][2] = 0.0f;
			result.m[1][3] = 0.0f;

			result.m[2][0] = 0.0f;
			result.m[2][1] = 0.0f;
			result.m[2][2] = 1.0f;
			result.m[2][3] = 0.0f;

			result.m[3][0] = 0.0f;
			result.m[3][1] = 0.0f;
			result.m[3][2] = 0.0f;
			result.m[3][3] = 1.0f;

			// 行列を返す
			return result;
		}

		//アフェイン変換行列
		static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
		{
			Matrix4x4 result;
			// 拡大縮小行列
			Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
			// 回転行列
			Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
			Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
			Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
			// 平行移動行列
			Matrix4x4 translationMatrix = MakeTranslationMatrix(translate);

			// 拡大縮小行列と回転行列を掛け算
			result = scaleMatrix * rotateXMatrix;
			// さらにY軸回転行列を掛け算
			result = result * rotateYMatrix;
			// さらにZ軸回転行列を掛け算
			result = result * rotateZMatrix;
			// 最後に平行移動行列を掛け算
			result = result * translationMatrix;

			// 結果を返す
			return result;
		}

		// --- 透視射影行列 ---
		static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
		{
			Matrix4x4 result;

			float cot = 1.0f / std::tanf(fovY / 2.0f);

			//行列の作成
			result.m[0][0] = cot / aspectRatio;
			result.m[0][1] = 0.0f;
			result.m[0][2] = 0.0f;
			result.m[0][3] = 0.0f;

			result.m[1][0] = 0.0f;
			result.m[1][1] = cot;
			result.m[1][2] = 0.0f;
			result.m[1][3] = 0.0f;

			result.m[2][0] = 0.0f;
			result.m[2][1] = 0.0f;
			result.m[2][2] = farClip / (farClip - nearClip);
			result.m[2][3] = 1.0f;

			result.m[3][0] = 0.0f;
			result.m[3][1] = 0.0f;
			result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
			result.m[3][3] = 0.0f;


			// 結果を返す
			return result;
		}

		// --- 正射影行列 ---
		static Matrix4x4 MakeOrthographicmatrix(float left, float top, float  right, float bottom, float nearClip, float farClip)
		{
			Matrix4x4 result;

			result.m[0][0] = 2.0f / (right - left);
			result.m[0][1] = 0.0f;
			result.m[0][2] = 0.0f;
			result.m[0][3] = 0.0f;

			result.m[1][0] = 0.0f;
			result.m[1][1] = 2.0f / (top - bottom);
			result.m[1][2] = 0.0f;
			result.m[1][3] = 0.0f;

			result.m[2][0] = 0.0f;
			result.m[2][1] = 0.0f;
			result.m[2][2] = 1.0f / (farClip - nearClip);
			result.m[2][3] = 0.0f;

			result.m[3][0] = -(right + left) / (right - left);
			result.m[3][1] = -(top + bottom) / (top - bottom);
			result.m[3][2] = -nearClip / (farClip - nearClip);
			result.m[3][3] = 1.0f;

			// 結果を返す
			return result;
		}

		static Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
		{
			Matrix4x4 result;

			result.m[0][0] = width / 2.0f;
			result.m[0][1] = 0.0f;
			result.m[0][2] = 0.0f;
			result.m[0][3] = 0.0f;

			result.m[1][0] = 0.0f;
			result.m[1][1] = -height / 2.0f;
			result.m[1][2] = 0.0f;
			result.m[1][3] = 0.0f;

			result.m[2][0] = 0.0f;
			result.m[2][1] = 0.0f;
			result.m[2][2] = maxDepth - minDepth;
			result.m[2][3] = 0.0f;

			result.m[3][0] = left + width / 2.0f;
			result.m[3][1] = top + height / 2.0f;
			result.m[3][2] = minDepth;
			result.m[3][3] = 1.0f;

			// 結果を返す
			return result;

		}

	};







	inline Matrix4x4 operator*(float s, const Matrix4x4& m) { return m * s; }

} // namespace Math
