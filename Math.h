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

	/// <summary>
	/// Vector3の加算
	/// </summary>
	/// <param name="v1">1つ目のベクトル</param>
	/// <param name="v2">2つ目のベクトル</param>
	/// <returns>加算されたベクトル</returns>
	Vector3 Vector3Add(const Vector3& v1, const Vector3& v2);

	/// <summary>
	/// Vector3の減算
	/// </summary>
	/// <param name="v1">1つ目のベクトル</param>
	/// <param name="v2">2つ目のベクトル</param>
	/// <returns>減算されたベクトル</returns>
	Vector3 Vector3Subtract(const Vector3& v1, const Vector3& v2);

	/// <summary>
	/// Vector3のスカラー乗算
	/// </summary>
	/// <param name="v">ベクトル</param>
	/// <param name="scalar">スカラー値</param>
	/// <returns>乗算されたベクトル</returns>
	Vector3 Vector3Multiply(const Vector3& v, float scalar);

	/// <summary>
	/// Vector3の内積
	/// </summary>
	/// <param name="v1">1つ目のベクトル</param>
	/// <param name="v2">2つ目のベクトル</param>
	/// <returns>内積のスカラー値</returns>
	float Vector3Dot(const Vector3& v1, const Vector3& v2);

	/// <summary>
	/// 長さ(ノルム)
	/// </summary>
	/// <param name="v">対象ベクトル</param>
	/// <returns>ベクトルの長さ</returns>
	float Length(const Vector3& v);

	/// <summary>
	/// 正規化
	/// </summary>
	/// <param name="v">対象ベクトル</param>
	/// <returns>正規化されたベクトル</returns>
	Vector3 Normalize(const Vector3& v);

#pragma endregion

#pragma region "行列の計算"

	/// <summary>
	/// 行列の加算
	/// </summary>
	/// <param name="m1">1つ目の行列</param>
	/// <param name="m2">2つ目の行列</param>
	/// <returns>加算結果の行列</returns>
	Matrix4x4 Matrix4x4Add(const Matrix4x4& m1, const Matrix4x4& m2);

	/// <summary>
	/// 行列の減算
	/// </summary>
	/// <param name="m1">1つ目の行列</param>
	/// <param name="m2">2つ目の行列</param>
	/// <returns>減算結果の行列</returns>
	Matrix4x4 Matrix4x4Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

	/// <summary>
	/// 行列の積
	/// </summary>
	/// <param name="m1">左辺の行列</param>
	/// <param name="m2">右辺の行列</param>
	/// <returns>積の結果行列</returns>
	Matrix4x4 Matrix4x4Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	/// <summary>
	/// 逆行列
	/// </summary>
	/// <param name="m">対象の行列</param>
	/// <returns>逆行列</returns>
	Matrix4x4 Matrix4x4Inverse(const Matrix4x4& m);

	/// <summary>
	///転置行列
	/// </summary>
	/// <param name="m">対象の行列</param>
	/// <returns>転置された行列</returns>
	Matrix4x4 Transpose(const Matrix4x4& m);

	/// <summary>
	/// 単位行列の作成
	/// </summary>
	/// <returns>単位行列</returns>
	Matrix4x4 MakeIdentity();

#pragma endregion

#pragma region"変換行列"

	/// <summary>
	/// 拡大縮小行列
	/// </summary>
	/// <param name="scale">スケール値</param>
	/// <returns>拡大縮小行列</returns>
	Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	/// <summary>
	/// 平行移動行列
	/// </summary>
	/// <param name="translate">移動量</param>
	/// <returns>平行移動行列</returns>
	Matrix4x4 MakeTranslationMatrix(const Vector3& translate);

	/// <summary>
	/// X軸回転行列
	/// </summary>
	/// <param name="radian">回転角（ラジアン）</param>
	/// <returns>X軸回転行列</returns>
	Matrix4x4 MakeRotateXMatrix(float radian);

	/// <summary>
	/// Y軸回転行列
	/// </summary>
	/// <param name="radian">回転角（ラジアン）</param>
	/// <returns>Y軸回転行列</returns>
	Matrix4x4 MakeRotateYMatrix(float radian);

	/// <summary>
	/// Z軸回転行列
	/// </summary>
	/// <param name="radian">回転角（ラジアン）</param>
	/// <returns>Z軸回転行列</returns>
	Matrix4x4 MakeRotateZMatrix(float radian);

	/// <summary>
	/// 3次元のアフィン変換行列
	/// </summary>
	/// <param name="scale">拡大縮小</param>
	/// <param name="rotate">回転</param>
	/// <param name="translate">平行移動</param>
	/// <returns>アフィン変換行列</returns>
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

#pragma endregion

#pragma region"レンダリングパイプライン"

	/// <summary>
	/// 透視投影行列を作成
	/// </summary>
	/// <param name="fovY">垂直方向の視野角（ラジアン）</param>
	/// <param name="aspectRatio">アスペクト比</param>
	/// <param name="nearClip">近クリップ面</param>
	/// <param name="farClip">遠クリップ面</param>
	/// <returns>透視投影行列</returns>
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
	
	/// <summary>
	/// 正射影行列を作成
	/// </summary>
	/// <param name="left">左端座標</param>
	/// <param name="top">上端座標</param>
	/// <param name="right">右端座標</param>
	/// <param name="bottom">下端座標</param>
	/// <param name="nearClip">近クリップ面</param>
	/// <param name="farClip">遠クリップ面</param>
	/// <returns>正射影行列</returns>
	Matrix4x4 MakeOrthographicmatrix(float left, float top, float  right, float bottom, float nearClip, float farClip);

	/// <summary>
	/// ビューポート行列を作成
	/// </summary>
	/// <param name="left">ビューポートの左端</param>
	/// <param name="top">ビューポートの上端</param>
	/// <param name="width">ビューポートの幅</param>
	/// <param name="height">ビューポートの高さ</param>
	/// <param name="minDepth">最小深度</param>
	/// <param name="maxDepth">最大深度</param>
	/// <returns>ビューポート変換行列</returns>
	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

#pragma endregion

};

