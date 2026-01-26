#pragma once
#include "KHEngine/Math/MathCommon.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"

class Billboard
{
public:
	// カメラのワールド行列からビルボード行列を作成する
	// useBillboard == false のときは単位行列を返す
	static Matrix4x4 Create(const Matrix4x4& cameraWorld, bool useBillboard);

	// Camera オブジェクトから直接ビルボード行列を作るオーバーロード
	static Matrix4x4 CreateFromCamera(const Camera* camera, bool useBillboard);

private:
	// 裏面回転（必要ならパラメータ化可能）
	static Matrix4x4 BackToFrontMatrix();
};