#pragma once
#include "KHEngine/Math/MathCommon.h"
#include "KHEngine/Core/OS/WinApp.h"


class Camera
{
public:

	// デフォルトコンストラクタ
	Camera();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	// --- Getter ---
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	const Matrix4x4 GetViewProjectionMatrix() const 
	{ 
		return Matrix4x4::Multiply(viewMatrix, projectionMatrix); 
	}
	Vector3& GetTranslate() { return transform.translate; }
	Vector3& GetRotation() { return transform.rotation; }

	// --- Setter ---
	void SetRotation(const Vector3& rotation) { transform.rotation = rotation; }
	void SetTranslate(const Vector3& translate) { transform.translate = translate; }
	void SetFovY(float fovY) { this->fovY = fovY; }
	void SetAspectRatio(float aspectRatio) { this->aspectRatio = aspectRatio; }
	void SetNearClip(float nearClip) { this->nearClip = nearClip; }
	void SetFarClip(float farClip) { this->farClip = farClip; }

	// --- getter ---
	float GetFovY() const { return fovY; }
	float GetAspectRatio() const { return aspectRatio; }
	float GetNearClip() const { return nearClip; }
	float GetFarClip() const { return farClip; }

private:
	Transform transform;
	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
	Matrix4x4 viewProjectionMatrix;
	// 水平方向視野角
	float fovY;
	// アスペクト比
	float aspectRatio;
	// ニアクリップ距離
	float nearClip;
	// ファークリップ距離
	float farClip;
};

