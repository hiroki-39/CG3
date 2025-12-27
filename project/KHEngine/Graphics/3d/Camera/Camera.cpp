#include "Camera.h"

Camera::Camera()
	: transform({ {0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f},{1.0f,1.0f,1.0f} })
	, fovY(0.45f)
	, aspectRatio(static_cast<float>(WinApp::kClientWidth) / static_cast<float>(WinApp::kClientHeight))
	, nearClip(0.1f)
	, farClip(100.0f)
	, worldMatrix(transform.GetWorldMatrix())
	, viewMatrix(Matrix4x4::Inverse(worldMatrix))
	, projectionMatrix(Matrix4x4::Perspective(fovY, aspectRatio, nearClip, farClip))
	, viewProjectionMatrix(Matrix4x4::Multiply(viewMatrix, projectionMatrix))
{}

void Camera::Update()
{
	//Transformの更新
	worldMatrix = transform.GetWorldMatrix();

	viewMatrix = Matrix4x4::Inverse(worldMatrix);

	projectionMatrix = Matrix4x4::Perspective(fovY, aspectRatio, nearClip, farClip);
}
