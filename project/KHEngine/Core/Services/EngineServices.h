#pragma once
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"

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

	Object3dCommon* GetObject3dCommon() const { return object3dCommon_; }
	DirectXCommon* GetDirectXCommon() const { return dxCommon_; }
	SrvManager* GetSrvManager() const { return srvManager_; }
	SpriteCommon* GetSpriteCommon() const { return spriteCommon_; }
	Input* GetInput() const { return input_; }
	ImGuiManager* GetImGuiManager() const { return imguiManager_; }

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
};