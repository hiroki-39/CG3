#include "ModelManager.h"
#include "KHEngine/Core/Resource/ResourceLocator.h"
#include <filesystem>
#include <cassert>

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
	// 既に読み込み済みか（論理名で管理）
	if (models.contains(filePath))
	{
		return;
	}

	// 論理名から実パスを解決
	std::string resolved = ResourceLocator::Resolve(filePath, ResourceLocator::AssetType::Model3D);
	assert(!resolved.empty() && "Model file not found via ResourceLocator");

	// 実パスをディレクトリとファイル名に分割して Model::Initialize に渡す
	std::filesystem::path rp(resolved);
	std::string directory = rp.parent_path().string();
	std::string filename = rp.filename().string();

	// モデルの生成とファイル読み込み、初期化
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelCommon.get(), directory, filename);

	// モデルをマップに格納（キーは呼び出し元が使っている論理名を維持）
	models.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
	// 読み込み済みのモデル検索（論理名で検索）
	if (models.contains(filePath))
	{
		return models.at(filePath).get();
	}
	return nullptr;
}