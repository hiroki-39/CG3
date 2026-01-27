#pragma once
#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Sound/Core/Sound.h"

class Application
{
public:

	enum class BlendMode
	{
		Alpha = 0,
		Additive,
		Multiply,
		PreMultiplied,
		None,
		Count
	};

public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 毎フレーム更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

private:

	// windowsアプリケーションのポインタ
	WinApp* winApp = nullptr;

	// DirectX共通部分のポインタ
	DirectXCommon* dxCommon = nullptr;

	// 入力のポインタ
	Input* input = nullptr;

	// スプライトの共通部分のポインタ
	SpriteCommon* spriteCommon = nullptr;

	// 3Dオブジェクトの共通部分のポインタ
	Object3dCommon* object3dCommon = nullptr;

	std::vector<Sprite*> sprites;

	std::vector<Object3d*> modelInstances;

	// 再生用オブジェクト
	Sound sound;

	Camera* camera;

	ParticleSystem particleSystem;

	ImGuiManager* imguiManager = nullptr;

	ParticleForGPU* instancingData = nullptr;

	const float kDeltaTime = 1.0f / 60.0f;

	const uint32_t kNumMaxInstance = 100;

	static SoundManager::SoundData Data;

	int currentBlendModeIndex;

	ParticleEffect currentEffect;

	bool update = true;

	// ビルボード（カメラ目線）
	bool useBillboard = true;

	bool isDisplaySprite = true;
};

