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
		
		nextScene_->Finalize();
		nextScene_.reset();
	}
}

void SceneManager::Update()
{
	
	if (nextScene_)
	{
		
		if (scene_)
		{
			scene_->Finalize();
			scene_.reset();
		}

		
		scene_ = std::move(nextScene_);

		
		scene_->SetSceneManager(this);

		
		if (scene_)
		{
			scene_->Initialize();
		}
	}
	
	if (scene_)
	{
		scene_->Update();
	}
}

void SceneManager::Draw()
{
	
	if (scene_)
	{
		scene_->Draw();
	}
}

void SceneManager::DrawUI()
{
	if (scene_)
	{
		scene_->DrawUI();
	}
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	assert(!nextScene_);

	
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}
