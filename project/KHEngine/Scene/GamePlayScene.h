#pragma once
#include <vector>
#include <random>
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Sound/Core/Sound.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include "KHEngine/Graphics/3d/Particle/ParticleSystem.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Graphics/3d/Particle/Particle.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"
#include "KHEngine/Math/Matrix4x4.h"
#include "KHEngine/Graphics/Billboard/Billboard.h"

class GamePlayScene
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
	void Initialize();
	void Update();
	void Draw();
	void Finalize();

private:
	// ゲーム固有メンバ（Application から移動）
	std::vector<Sprite*> sprites;
	std::vector<Object3d*> modelInstances;
	
	Sound sound;
	
	Camera* camera = nullptr;
	
	ParticleSystem particleSystem;
	ParticleForGPU* instancingData = nullptr;
	
	const uint32_t kNumMaxInstance = 100;
	
	SoundManager::SoundData Data = {};
	
	int currentBlendModeIndex = 0;
	
	ParticleRenderer particleRenderer;
	ParticleEffect currentEffect = ParticleEffect::Wind;
	
	uint32_t numInstance = 0;
	uint32_t particleSrvIndex = 0;
	
	bool update = true;
	bool useBillboard = true;
	bool isDisplaySprite = true;

	 float kDeltaTime_ = 1.0f / 60.0f;

	// 乱数
	std::random_device seedGenerator;
	std::mt19937 randomEngine{ seedGenerator() };
};