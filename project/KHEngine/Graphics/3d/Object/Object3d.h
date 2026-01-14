#pragma once
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Math/MathCommon.h"
#include "KHEngine/Graphics/3d/Model/Model.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"

class Object3d
{
public:// 構造体

	struct TransformationMatrix
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
	};


	struct DirectionlLight
	{
		Vector4 color; // ライトの色
		Vector3 direction; //ライトの向き
		float intensity; //輝度
	};

public://メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Object3dCommon* object3dCommon);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	// --- Getter ---
	const Vector3& GetScale() const { return transform.scale; }
	const Vector3& GetRotation() const { return transform.rotation; }
	const Vector3& GetTranslate() const { return transform.translate; }
	Vector4 GetDirectionalLightColor() const;
	Vector3 GetDirectionalLightDirection() const;
	float GetDirectionalLightIntensity() const;

	// --- Setter ---
	void SetModel(Model* model) { this->model = model; }
	void SetModel(const std::string& filePath);
	void SetScale(const Vector3& scale) { transform.scale = scale; }
	void SetRotation(const Vector3& rotation) { transform.rotation = rotation; }
	void SetTranslate(const Vector3& translate) { transform.translate = translate; }
	void SetCamera(Camera* camera) { this->camera = camera; }
	void SetDirectionalLightColor(const Vector4& color);
	void SetDirectionalLightDirection(const Vector3& direction);
	void SetDirectionalLightIntensity(float intensity);

private://メンバ関数

	/// <summary>
	/// 座標変換行列データの作成
	///	</summary>
	void CreateTransformationMatrixResource();

	/// <summary>
	/// 平行光源の作成
	/// </summary>
	void CreateDirectionalLight();



private://メンバ変数

	// 共通部分
	Object3dCommon* object3dCommon = nullptr;

	// DirectXCommon取得
	DirectXCommon* dxCommon = nullptr;

	WinApp* winApp_ = nullptr;

	Model* model = nullptr;

	Camera* camera = nullptr;

	// 変換行列リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	// 変換行列データの仮想アドレス
	TransformationMatrix* transformationMatrixData_ = nullptr;


	//平行光源用のリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResouerce_;
	//データを書き込む

	DirectionlLight* directionalLightData_ = nullptr;

	Transform transform;

	Transform cameraTransform;
};