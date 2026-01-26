#include "KHEngine/Graphics/Billboard/Billboard.h"
#include <numbers>

Matrix4x4 Billboard::Create(const Matrix4x4& cameraWorld, bool useBillboard)
{
	if (!useBillboard)
	{
		return Matrix4x4::Identity();
	}

	// ƒJƒƒ‰‚Ì‰ñ“]¬•ª‚Ì‚İ‚ğæ‚èo‚µ‚ÄˆÊ’u‚ğœŠO
	Matrix4x4 camRotOnly = cameraWorld;
	camRotOnly.m[3][0] = 0.0f;
	camRotOnly.m[3][1] = 0.0f;
	camRotOnly.m[3][2] = 0.0f;

	// ‹ts—ñi‰ñ“]‚Ì‹tj
	Matrix4x4 invCam = Matrix4x4::Inverse(camRotOnly);

	// — –Ê‰ñ“]‚ğæ‚è“ü‚ê‚ÄŠ®¬
	Matrix4x4 billboard = BackToFrontMatrix() * invCam;

	// translation ¬•ª‚ğƒ[ƒ‚É‚µ‚ÄˆÊ’uî•ñ‚ğ‚½‚¹‚È‚¢
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
	// Šù‘¶‚ÌÀ‘•‚Æ‡‚í‚¹‚Ä Y ‰ñ“]‚Å— –Ê‚ğ³–Ê‚ÉŒü‚¯‚é
	return Matrix4x4::RotateY(std::numbers::pi_v<float>);
}