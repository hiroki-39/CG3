#pragma once
#include "Vector3.h"
#include <cmath>

class Matrix4x4
{
public:
	float m[4][4]{};

	// --- 基本行列 ---

	/// <summary>
	/// 単位行列を取得 
	/// </summary>
	/// <returns>単位行列</returns>
	static Matrix4x4 Identity();

	/// <summary>
	/// スケーリング行列を取得 
	/// </summary>
	/// <param name="scale">スケール</param>
	/// <returns>スケーリング行列</returns>
	static Matrix4x4 Scale(const Vector3& scale);

	/// <summary>
	/// 平行移動行列を取得
	/// </summary>
	/// <param name="trans">移動量</param>
	/// <returns>平行移動行列</returns>
	static Matrix4x4 Translation(const Vector3& trans);

	/// <summary>
	/// X軸回転行列を取得 
	/// </summary>
	/// <param name="rad">回転角（ラジアン）</param>
	/// <returns>X軸回転行列</returns>
	static Matrix4x4 RotateX(float rad);

	/// <summary>
	/// Y軸回転行列を取得 
	/// </summary>
	/// <param name="rad">回転角（ラジアン）</param>
	/// <returns>Y軸回転行列</returns>
	static Matrix4x4 RotateY(float rad);

	/// <summary>
	/// Z軸回転行列を取得 
	/// </summary>
	/// <param name="rad">回転角（ラジアン）</param>
	/// <returns>Z軸回転行列</returns>
	static Matrix4x4 RotateZ(float rad);

	/// <summary>
	/// アフェイン変換行列
	/// </summary>
	static Matrix4x4 MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	// --- 行列計算 ---

	/// <summary>
	/// 行列の掛け算 
	/// </summary>
	/// <param name="a">行列A</param>
	/// <param name="b">行列B</param>
    /// <returns>掛け算結果の行列</returns>
	static Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b);

	/// <summary>
	/// 転置行列を取得
	/// </summary>
	/// <param name="m">元の行列</param>
	/// <returns>転置行列</returns>
	static Matrix4x4 Transpose(const Matrix4x4& m);

	/// <summary>
	/// 逆行列を取得
	/// </summary>
	/// <param name="m">元の行列</param>
	/// <returns>逆行列</returns>
	static Matrix4x4 Inverse(const Matrix4x4& m);

	// --- 投影関連 ---

	/// <summary>
	/// 透視投影行列を取得
	/// </summary>
	/// <param name="fovY">垂直視野角（ラジアン）</param>
	/// <param name="aspect">アスペクト比</param>
	/// <param name="nearZ">前端</param>
	/// <param name="farZ">後端</param>
	/// <returns>透視投影行列</returns>
	static Matrix4x4 Perspective(float fovY, float aspect, float nearZ, float farZ);

	/// <summary>
	/// 正射影行列を取得
	/// </summary>
	/// <param name="left">左端</param>
	/// <param name="top">上端</param>
	/// <param name="right">右端</param>
	/// <param name="bottom">下端</param>
	/// <param name="nearZ">前端</param>
	/// <param name="farZ">後端</param>
	/// <returns>正射影行列</returns>
	static Matrix4x4 Orthographic(float left, float top, float right, float bottom, float nearZ, float farZ);
	
	/// <summary>
	/// ビューポート変換行列を取得
	/// </summary>
	/// <param name="left">ビューポートの左端座標</param>
	/// <param name="top">ビューポートの上端座標</param>
	/// <param name="width">ビューポートの幅</param>
	/// <param name="height">ビューポートの高さ</param>
	/// <param name="minDepth">最小深度値</param>
	/// <param name="maxDepth">最大深度値</param>
	/// <returns>ビューポート変換行列</returns>
	static Matrix4x4 Viewport(float left, float top, float width, float height, float minDepth, float maxDepth);

	// --- 演算子オーバーロード ---

	/// <summary>
	/// 行列乗算 (this * rhs)
	/// </summary>
	Matrix4x4 operator*(const Matrix4x4& rhs) const
	{
		return Multiply(*this, rhs);
	}

	/// <summary>
	/// 行列自己代入乗算 (this = this * rhs)
	/// </summary>
	Matrix4x4& operator*=(const Matrix4x4& rhs)
	{
		*this = (*this) * rhs;
		return *this;
	}

	/// <summary>
	/// スカラー乗算 (this * s)
	/// </summary>
	Matrix4x4 operator*(float s) const
	{
		Matrix4x4 r;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				r.m[i][j] = m[i][j] * s;
		return r;
	}

	/// <summary>
	/// ベクトル変換 (同次座標 w=1 として扱う)
	/// w によって正規化（w != 0 の場合）
	/// </summary>
	Vector3 operator*(const Vector3& v) const
	{
		float x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * 1.0f;
		float y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * 1.0f;
		float z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * 1.0f;
		float w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * 1.0f;

		constexpr float EPS = 1e-6f;
		if (std::fabs(w) > EPS)
		{
			return Vector3(x / w, y / w, z / w);
		}
		else
		{
			// w が 0 に近い場合は方向ベクトル扱いとして正規化を行わずそのまま返す
			return Vector3(x, y, z);
		}
	}

	/// <summary>
	/// 等価比較（小さな誤差を許容）
	/// </summary>
	bool operator==(const Matrix4x4& rhs) const
	{
		constexpr float EPS = 1e-6f;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				if (std::fabs(m[i][j] - rhs.m[i][j]) > EPS)
					return false;
		return true;
	}

	/// <summary>
	/// 非等価
	/// </summary>
	bool operator!=(const Matrix4x4& rhs) const
	{
		return !(*this == rhs);
	}
};

// スカラー左乗算 (s * m)
inline Matrix4x4 operator*(float s, const Matrix4x4& m)
{
	return m * s;
}
