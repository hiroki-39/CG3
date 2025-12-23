#include "Object3d.h"

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
	Matrix4x4 cameraMatrix = cameraTransform.GetWorldMatrix();
	Matrix4x4 viewMatrix = Matrix4x4::Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = Matrix4x4::Perspective(0.45f, float(winApp_->kClientWidth) / float(winApp_->kClientHeight), 0.1f, 100.0f);
	//WVPMatrixの作成
	Matrix4x4 worldViewProjectionMatrix = Matrix4x4::Multiply(worldMatrix, Matrix4x4::Multiply(viewMatrix, projectionMatrix));
	transformationMatrixData_->WVP = worldViewProjectionMatrix;
	transformationMatrixData_->World = worldMatrix;

	// 平行光源の向きの正規化
	directionalLightData_->direction = directionalLightData_->direction.Normalize();
}

void Object3d::Draw()
{
	//wvp用のCBufferの場所を設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

	//平行光源用のCBufferの場所を設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResouerce_->GetGPUVirtualAddress());

	//モデルの描画
	if (model)
	{
		model->Draw();
	}

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
