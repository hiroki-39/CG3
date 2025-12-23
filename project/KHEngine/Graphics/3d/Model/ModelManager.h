#pragma once
#include <map>
#include <string>
#include <memory>
#include "KHEngine/Graphics/3d/Model/Model.h"
#include "KHEngine/Graphics/3d/Model/ModelCommon.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"

class ModelManager
{
public:

	// シングルトンインスタンスの取得
	static ModelManager* GetInstance();
	
	// 終了
	void Finalize();

	// 初期化
	void Initialize(DirectXCommon* dxCommon);


	/// <summary>
	/// モデルファイルの読み込み 
	/// </summary>
	/// <param name="filePath">モデルファイル</param>
	void LoadModel(const std::string& filePath);

	/// <summary>
	/// モデルの検索 
	/// </summary>
	/// <param name="filePath">モデルファイルのパス</param>
	/// <returns>モデル</returns>
	Model* FindModel(const std::string& filePath);

private:

	// シングルトンインスタンス
	static ModelManager* instance_;

	// プライベートコンストラクタ
	ModelManager() = default;

	// プライベートデストラクタ
	~ModelManager() = default;

	// コピーコンストラクタ
	ModelManager(const ModelManager&) = delete;

	// コピー代入演算子
	ModelManager& operator=(const ModelManager&) = delete;

private: // メンバ変数

	// モデルデータ
	std::map<std::string, std::unique_ptr<Model>>models;
	
	ModelCommon* modelCommon = nullptr;
};

