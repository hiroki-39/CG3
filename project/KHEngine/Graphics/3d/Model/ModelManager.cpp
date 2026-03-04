#include "ModelManager.h"

// シングルトンインスタンスの取得（Meyers singleton で new を排除）
ModelManager* ModelManager::GetInstance()
{
	static ModelManager instance;
	return &instance;
}

// 終了: シングルトン自体は破棄しない。内部リソースのみ解放する。
void ModelManager::Finalize()
{
	// 保持しているモデルを解放（unique_ptr により自動解放）
	models.clear();

	// ModelCommon を解放
	modelCommon.reset();
}

// 初期化
void ModelManager::Initialize(DirectXCommon* dxCommon)
{
	// ModelCommon をスマートポインタで生成（new は使わない）
	modelCommon = std::make_unique<ModelCommon>();
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
	model->Initialize(modelCommon.get(),"resources",filePath);

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
