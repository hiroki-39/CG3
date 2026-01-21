#include "Object3d.h"
#include "KHEngine/Graphics/Light/LightManager.h" ｘ

void Object3d::Initialize(Object3dCommon* object3dCommon)
{
	// 引数で受け取ってメンバ変数に記録
	assert(object3dCommon != nullptr);
	this->object3dCommon = object3dCommon;

	// DirectXCommonを取得して保存
	this->dxCommon = object3dCommon->GetDirectXCommon();
	assert(this->dxCommon != nullptr);

	//座標変換行列データの作成
	CreateTransformationMatrixResource();

	// NOTE:
	// 平行光の作成は Object3d 側で行わず、LightManager に任せる。
	// LightManager はシーン側（main 等）で生成して Object3dCommon::SetLightManager() に渡してください。

	this->camera = object3dCommon->GetDefaultCamera();


	transform.translate = { 0.0f,0.0f,0.0f };
	transform.rotation = { 0.0f,0.0f,0.0f };
	transform.scale = { 1.0f,1.0f,1.0f };

	cameraTransform.translate = { 0.0f,0.0f,-15.0f };
	cameraTransform.rotation = { 0.0f,0.0f,0.0f };
	cameraTransform.scale = { 1.0f,1.0f,1.0f };
}

void Object3d::Update()
{
	//Transformの更新
	Matrix4x4 worldMatrix = transform.GetWorldMatrix();

	Matrix4x4 worldViewProjectionMatrix;
	if (camera)
	{
		const Matrix4x4& ViewProjectionMatrix = camera->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Matrix4x4::Multiply(worldMatrix, ViewProjectionMatrix);
	}
	else
	{
		worldViewProjectionMatrix = worldMatrix;
	}

	transformationMatrixData_->WVP = worldViewProjectionMatrix;
	transformationMatrixData_->World = worldMatrix;

	// 平行光源の向きの正規化（LightManager の最初の Directional を使用）
	auto lm = object3dCommon ? object3dCommon->GetLightManager() : nullptr;
	if (lm)
	{
		auto dir = lm->GetFirstDirectional();
		if (dir)
		{
			// Normalize 実装が Vector3::Normalize() でない場合は適宜変えてください
			Vector3 d = dir->direction;
			// 正規化（簡易）
			float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
			if (len > 1e-6f)
			{
				dir->direction = Vector3{ d.x / len, d.y / len, d.z / len };
			}
		}
	}

	if (camera && cameraData_)
	{
		// カメラのワールド位置を CameraGpu に書く（GetTranslate は参照を返す）
		Vector3 camPos = camera->GetTranslate();
		cameraData_->worldPosition = camPos;
	}
}

void Object3d::Draw()
{
	//wvp用のCBufferの場所を設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

	// NOTE:
	// 平行光源の CBV は Object3d ではセットしない（ライトのアップロード／バインドは別責務へ移行）
	// もし既存シェーダが root 3 にライトを期待しているなら、レンダラ側で LightManager のデータを GPU バッファへ詰めて root 3 をセットしてください。

	if (cameraResource_)
	{
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
	}

	//モデルの描画
	if (model)
	{
		model->Draw();
	}
}

void Object3d::SetModel(const std::string& filePath)
{
	// 指定ファイルが未ロードならモデルをロードする（安全策）
	if (ModelManager::GetInstance()->FindModel(filePath) == nullptr)
	{
		ModelManager::GetInstance()->LoadModel(filePath);
	}

	// モデルポインタを取得
	model = ModelManager::GetInstance()->FindModel(filePath);

	// デバッグ用にアサート（実運用ならログ出力に変更してもよい）
	assert(model != nullptr);
}

void Object3d::CreateTransformationMatrixResource()
{
	//WVP用のリソースを作る
	transformationMatrixResource_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));

	//書き込むためのアドレス取得
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	//単位行列を書き込む
	transformationMatrixData_->WVP = Matrix4x4::Identity();
	transformationMatrixData_->World = Matrix4x4::Identity();
	transformationMatrixData_->WorldInverseTranspose = Matrix4x4::Identity();

	// カメラ用CBufferを作成 
	cameraResource_ = dxCommon->CreateBufferResource(sizeof(CameraForGPU));
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

	if (cameraData_)
	{
		cameraData_->worldPosition = Vector3{ 0.0f, 0.0f, 0.0f };
		cameraData_->padding = 0.0f;
	}
}

// --- ライトの getter/setter は LightManager を参照する実装 ---

Vector4 Object3d::GetDirectionalLightColor() const
{
	auto lm = object3dCommon ? object3dCommon->GetLightManager() : nullptr;
	if (lm)
	{
		auto dir = lm->GetFirstDirectional();
		if (dir) return dir->color;
	}
	return Vector4{1.0f,1.0f,1.0f,1.0f};
}

Vector3 Object3d::GetDirectionalLightDirection() const
{
	auto lm = object3dCommon ? object3dCommon->GetLightManager() : nullptr;
	if (lm)
	{
		auto dir = lm->GetFirstDirectional();
		if (dir) return dir->direction;
	}
	return Vector3{1.0f,0.0f,0.0f};
}

float Object3d::GetDirectionalLightIntensity() const
{
	auto lm = object3dCommon ? object3dCommon->GetLightManager() : nullptr;
	if (lm)
	{
		auto dir = lm->GetFirstDirectional();
		if (dir) return dir->intensity;
	}
	return 1.0f;
}

void Object3d::SetDirectionalLightColor(const Vector4& color)
{
	auto lm = object3dCommon ? object3dCommon->GetLightManager() : nullptr;
	if (lm)
	{
		auto dir = lm->GetFirstDirectional();
		if (dir) dir->color = color;
	}
}

void Object3d::SetDirectionalLightDirection(const Vector3& direction)
{
	auto lm = object3dCommon ? object3dCommon->GetLightManager() : nullptr;
	if (lm)
	{
		auto dir = lm->GetFirstDirectional();
		if (dir) dir->direction = direction;
	}
}

void Object3d::SetDirectionalLightIntensity(float intensity)
{
	auto lm = object3dCommon ? object3dCommon->GetLightManager() : nullptr;
	if (lm)
	{
		auto dir = lm->GetFirstDirectional();
		if (dir) dir->intensity = intensity;
	}
}
