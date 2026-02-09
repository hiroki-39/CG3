#include "SceneManager.h"

SceneManager::~SceneManager()
{
	scene_->Finalize();
	delete scene_;
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
			delete scene_;
		}
		
		// シーンを切り替え
		scene_ = nextScene_;
		nextScene_ = nullptr;

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