#pragma once
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Graphics/PostProcess/PostProcess.h"

class AbstractSceneFactory;

class EngineServices
{
public:
	static EngineServices* GetInstance()
	{
		static EngineServices instance;
		return &instance;
	}

	void SetObject3dCommon(Object3dCommon* obj) { object3dCommon_ = obj; }
	void SetDirectXCommon(DirectXCommon* dx) { dxCommon_ = dx; }
	void SetSrvManager(SrvManager* srv) { srvManager_ = srv; }
	void SetSpriteCommon(SpriteCommon* sprite) { spriteCommon_ = sprite; }
	void SetInput(Input* input) { input_ = input; }
	void SetImGuiManager(ImGuiManager* imgui) { imguiManager_ = imgui; }

	// シーンファクトリーの Setter/Getter を追加
	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }
	AbstractSceneFactory* GetSceneFactory() const { return sceneFactory_; }

	Object3dCommon* GetObject3dCommon() const { return object3dCommon_; }
	DirectXCommon* GetDirectXCommon() const { return dxCommon_; }
	SrvManager* GetSrvManager() const { return srvManager_; }
	SpriteCommon* GetSpriteCommon() const { return spriteCommon_; }
	Input* GetInput() const { return input_; }
	ImGuiManager* GetImGuiManager() const { return imguiManager_; }

	void SetPostProcess(PostProcess* postProcess) { postProcess_ = postProcess; }
	PostProcess* GetPostProcess() const { return postProcess_; }

	void SetDeltaTime(float dt) { deltaTime_ = dt; }
	float GetDeltaTime() const { return deltaTime_; }

	void SetEditorMode(bool mode) { isEditorMode_ = mode; }
	bool GetEditorMode() const { return isEditorMode_; }

private:
	EngineServices() = default;
	~EngineServices() = default;

	// キャッシュするフレームワーク側ポインタ
	Object3dCommon* object3dCommon_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;
	Input* input_ = nullptr;
	ImGuiManager* imguiManager_ = nullptr;

	// 追加: シーンファクトリー参照
	AbstractSceneFactory* sceneFactory_ = nullptr;

	PostProcess* postProcess_ = nullptr;

	float deltaTime_ = 1.0f / 60.0f;

#ifdef USE_IMGUI
	bool isEditorMode_ = true;
#else
	bool isEditorMode_ = false;
#endif
};