#include "KHEngine/Graphics/Billboard/Billboard.h"
#include <numbers>

Matrix4x4 Billboard::Create(const Matrix4x4& cameraWorld, bool useBillboard)
{
	if (!useBillboard)
	{
		return Matrix4x4::Identity();
	}

	// カメラの回転成分のみを取り出して位置を除外
	Matrix4x4 camRotOnly = cameraWorld;
	camRotOnly.m[3][0] = 0.0f;
	camRotOnly.m[3][1] = 0.0f;
	camRotOnly.m[3][2] = 0.0f;

	// 逆行列（回転の逆）
	Matrix4x4 invCam = Matrix4x4::Inverse(camRotOnly);

	// 裏面回転を取り入れて完成
	Matrix4x4 billboard = BackToFrontMatrix() * invCam;

	// translation 成分をゼロにして位置情報を持たせない
	billboard.m[3][0] = 0.0f;
	billboard.m[3][1] = 0.0f;
	billboard.m[3][2] = 0.0f;

	return billboard;
}

Matrix4x4 Billboard::CreateFromCamera(const Camera* camera, bool useBillboard)
{
	if (!camera) return Matrix4x4::Identity();
	return Create(camera->GetWorldMatrix(), useBillboard);
}

Matrix4x4 Billboard::BackToFrontMatrix()
{
	// 既存の実装と合わせて Y 回転で裏面を正面に向ける
	return Matrix4x4::RotateY(std::numbers::pi_v<float>);
}