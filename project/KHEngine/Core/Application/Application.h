#pragma once
#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Sound/Core/Sound.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Graphics/3d/Particle/ParticleSystem.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Core/Framework/KHFramework.h"
#include <random>

class Application : public KHFramework
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
	void Initialize() override;

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 毎フレーム更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

private:

	// スプライトの共通部分のポインタ
	SpriteCommon* spriteCommon = nullptr;

	// 3Dオブジェクトの共通部分のポインタ
	Object3dCommon* object3dCommon = nullptr;

	// スプライト配列
	std::vector<Sprite*> sprites;

	// 3Dモデルインスタンス配列
	std::vector<Object3d*> modelInstances;

	// SRVマネージャー
	SrvManager* srvManager = nullptr;

	// 再生用オブジェクト
	Sound sound;

	// カメラ
	Camera* camera;

	// パーティクルシステム
	ParticleSystem particleSystem;

	// GPU用パーティクルインスタンス配列
	ParticleForGPU* instancingData = nullptr;

	// フレーム時間（60fps 固定想定）
	const float kDeltaTime = 1.0f / 60.0f;

	// パーティクルの最大インスタンス数
	const uint32_t kNumMaxInstance = 100;

	// サウンドデータ
	SoundManager::SoundData Data = {};

	// パーティクル用テクスチャのインデックス
	int currentBlendModeIndex;

	// パーティクルレンダラー
	ParticleRenderer particleRenderer;

	// 現在のパーティクルエフェクト
	ParticleEffect currentEffect;

	// パーティクルのインスタンス数
	uint32_t numInstance;

	// パーティクル用テクスチャのSRVインデックス
	uint32_t particleSrvIndex;

	// パーティクルの更新フラグ
	bool update = true;

	// ビルボード（カメラ目線）
	bool useBillboard = true;

	// スプライト表示フラグ
	bool isDisplaySprite = true;

	// ゲーム終了フラグ
	bool endRequst_ = false;

	// 乱数生成器の初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine{ seedGenerator() };
};

