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
};
