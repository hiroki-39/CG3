#include "ModelManager.h"

ModelManager* ModelManager::instance_ = nullptr;

// シングルトンインスタンスの取得
ModelManager* ModelManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = new ModelManager();
	}
	return instance_;
}

// 終了
void ModelManager::Finalize()
{
	if (instance_ != nullptr)
	{
		delete instance_;
		instance_ = nullptr;
	}
}

// 初期化
void ModelManager::Initialize(DirectXCommon* dxCommon)
{
	modelCommon = new ModelCommon();
	modelCommon->Initialize(dxCommon);
}

void ModelManager::LoadModel(const std::string& filePath)
{
	// 読み込み済みのモデル検索
	if (models.contains(filePath))
	{
		// 既に読み込み済みなら返す
		return;
	}

	// モデルの生成とファイル読み込み、初期化
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelCommon,"resources",filePath);

	// モデルをマップに格納
	models.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
	// 読み込み済みのモデル検索
	if (models.contains(filePath))
	{
		// 読み込みモデルを戻り値として返す
		return models.at(filePath).get();
	}

	// ファイル名一致なし
	return nullptr;
}
