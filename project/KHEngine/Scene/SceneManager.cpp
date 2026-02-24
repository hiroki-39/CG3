#include "SceneManager.h"
#include <cassert>

SceneManager::~SceneManager()
{
	if (scene_)
	{
		scene_->Finalize();
		scene_.reset();
	}
	if (nextScene_)
	{
		// 未使用で残った予約シーンがあれば解放
		nextScene_->Finalize();
		nextScene_.reset();
	}
}

void SceneManager::Update()
{
	// シーン切り替えが予約されている場合
	if (nextScene_)
	{
		// 既存シーンの終了処理
		if (scene_)
		{
			scene_->Finalize();
			scene_.reset();
		}

		// シーンを切り替え
		scene_ = std::move(nextScene_);

		// シーンマネージャーをセット
		scene_->SetSceneManager(this);

		// 新シーンの初期化
		if (scene_)
		{
			scene_->Initialize();
		}
	}
	// 現在のシーン更新
	if (scene_)
	{
		scene_->Update();
	}
}

void SceneManager::Draw()
{
	// 現在のシーン描画
	if (scene_)
	{
		scene_->Draw();
	}
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	assert(!nextScene_);

	// 次のシーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}