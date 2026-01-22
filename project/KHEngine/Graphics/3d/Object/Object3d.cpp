#include "Object3d.h"
#include "Object3dCommon.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <cassert>

// 既存の実装はそのまま残し、PointLight 関連を追加しています。

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

	//平行光源の作成
	CreateDirectionalLight();

	// 点光源を作成（追加）
	CreatePointLight();

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

	// 平行光源の向きの正規化
	if (directionalLightData_)
	{
		directionalLightData_->direction = directionalLightData_->direction.Normalize();
	}

	// 点光源のデータ（必要ならここで更新。現状は位置は外部で SetPointLightPosition から設定される想定）

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

	//平行光源用のCBufferの場所を設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResouerce_->GetGPUVirtualAddress());

	// 点光源用のCBufferの場所を設定（ルートインデックスは既存のバインド規則に合わせて 5 を使用）
	if (pointLightResource_)
	{
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());
	}

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

void Object3d::CreateDirectionalLight()
{
	//平行光源用のリソースを作成
	directionalLightResouerce_ = dxCommon->CreateBufferResource(sizeof(DirectionlLight));

	//書き込むためのアドレス取得
	directionalLightResouerce_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

	//ライトの色
	directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
	//向き
	directionalLightData_->direction = { 1.0f,0.0f,0.0f };
	//輝度
	directionalLightData_->intensity = 1.0f;
}

void Object3d::CreatePointLight()
{
	// 点光源用のリソースを作成
	pointLightResource_ = dxCommon->CreateBufferResource(sizeof(PointLight));

	// 書き込むためのアドレス取得
	pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

	// デフォルト値を設定（透明化の原因を防ぐため必ず初期化）
	if (pointLightData_)
	{
		pointLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		pointLightData_->direction = { 0.0f, 5.0f, 0.0f };
		pointLightData_->intensity = 1.0f;

		// デバッグ出力
		char buf[256];
		sprintf_s(buf, "PointLight init: color=(%f,%f,%f,%f) pos=(%f,%f,%f) intensity=%f\n",
			pointLightData_->color.x, pointLightData_->color.y, pointLightData_->color.z, pointLightData_->color.w,
			pointLightData_->direction.x, pointLightData_->direction.y, pointLightData_->direction.z,
			pointLightData_->intensity);
		OutputDebugStringA(buf);
	}
}

Vector4 Object3d::GetDirectionalLightColor() const
{
	if (directionalLightData_) return directionalLightData_->color;
	return Vector4{ 1.0f,1.0f,1.0f,1.0f };
}

Vector3 Object3d::GetDirectionalLightDirection() const
{
	if (directionalLightData_) return directionalLightData_->direction;
	return Vector3{ 1.0f,0.0f,0.0f };
}

float Object3d::GetDirectionalLightIntensity() const
{
	if (directionalLightData_) return directionalLightData_->intensity;
	return 1.0f;
}

void Object3d::SetDirectionalLightColor(const Vector4& color)
{
	if (directionalLightData_) directionalLightData_->color = color;
}

void Object3d::SetDirectionalLightDirection(const Vector3& direction)
{
	if (directionalLightData_) directionalLightData_->direction = direction;
}

void Object3d::SetDirectionalLightIntensity(float intensity)
{
	if (directionalLightData_) directionalLightData_->intensity = intensity;
}

// --- PointLight getters / setters ---

Vector4 Object3d::GetPointLightColor() const
{
	if (pointLightData_) return pointLightData_->color;
	return Vector4{ 1.0f,1.0f,1.0f,1.0f };
}

Vector3 Object3d::GetPointLightPosition() const
{
	if (pointLightData_) return pointLightData_->direction; // HLSL 側フィールド名に合わせる
	return Vector3{ 0.0f, 0.0f, 0.0f };
}

float Object3d::GetPointLightIntensity() const
{
	if (pointLightData_) return pointLightData_->intensity;
	return 1.0f;
}

void Object3d::SetPointLightColor(const Vector4& color)
{
	if (pointLightData_) pointLightData_->color = color;

	if (pointLightData_)
	{
		pointLightData_->color = color;
		// デバッグ出力
		char buf[128];
		sprintf_s(buf, "SetPointLightColor: (%f,%f,%f,%f)\n", color.x, color.y, color.z, color.w);
		OutputDebugStringA(buf);
	}
}

void Object3d::SetPointLightPosition(const Vector3& position)
{
	if (pointLightData_) pointLightData_->direction = position; // HLSL 側フィールド名に合わせる
}

void Object3d::SetPointLightIntensity(float intensity)
{
	if (pointLightData_) pointLightData_->intensity = intensity;
}