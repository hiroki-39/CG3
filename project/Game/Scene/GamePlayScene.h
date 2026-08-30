#pragma once
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Sound/Core/Sound.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Graphics/3d/Particle/Particle.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"
#include "KHEngine/Math/Matrix4x4.h"
#include "KHEngine/Graphics/Billboard/Billboard.h"
#include "KHEngine/Core/Framework/BaseScene.h"
#include "KHEngine/Graphics/3d/Skybox/Skybox.h"
#include "KHEngine/Graphics/3d/Particle/ParticleEffect.h"
#include "Game/Actor/Player/Player.h"
#include "Game/Actor/Bullet/PlayerBullet.h"
#include "Game/Actor/Bullet/PlayerMissile.h"
#include "Game/Actor/Bullet/EnemyBullet.h"
#include "Game/System/Rail.h"
#include "Game/System/RailCameraController.h"
#include "Game/Actor/Enemy/Enemy.h"
#include "Game/Actor/Obstacle/Obstacle.h"
#include "Game/Actor/Item/EnhanceRing.h"
#include <vector>
#include <list>
#include <random>
#include <memory>

class GamePlayScene : public BaseScene
{
public:
    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DrawUI() override;
    void Finalize() override;

    
    
    
    void ReloadLevel();

    
    
    
    void ReloadEnemiesOnly();

private:
    
    std::vector<std::unique_ptr<Object3d>> modelInstances;

    std::unique_ptr<Camera> camera;
    std::unique_ptr<Camera> debugCamera_;
    Camera* activeCamera_ = nullptr;
    std::unique_ptr<Object3d> cameraObject_;
    
    
    std::vector<std::unique_ptr<Rail>> mainRails_;
    std::unique_ptr<RailCameraController> railCameraController_;
    float baseGameSpeed_ = 1.0f; 
    float gameSpeed_ = 1.0f;     
    
    
    bool isJustDodgeActive_ = false;
    float justDodgeTimer_ = 0.0f;
    float justDodgeMaxTime_ = 60.0f; 
    float justDodgeSlowSpeed_ = 0.2f; 
    std::vector<std::unique_ptr<Model>> railModels_;
    std::unique_ptr<Model> enemyRailModel_;
    std::vector<std::unique_ptr<Object3d>> railVisualizers_;
    std::vector<std::unique_ptr<Object3d>> enemyRailVisualizers_;
#ifdef USE_IMGUI
    bool isDrawRail_ = true;
#else
    bool isDrawRail_ = false;
#endif
    std::unique_ptr<Skybox> skybox_;
#ifdef USE_IMGUI
    bool isPlaying_ = false;
#else
    bool isPlaying_ = true;
#endif

    
    Vector3 currentCameraRot_ = {0.0f, 0.0f, 0.0f};
    float lastCameraYaw_ = 0.0f;
    float currentCameraBank_ = 0.0f;

    
    std::random_device seedGenerator;
    std::mt19937 randomEngine{ seedGenerator() };

    
    ParticleEffect thrusterEffect_;   
    ParticleEffect explosionEffect_;  
    ParticleEffect hitEffect_;        
    ParticleEffect dodgeEffect_;      
    ParticleEffect trailEffect_;      
    ParticleEffect missileSmokeEffect_; 

    
    int currentEditEffectIndex_ = 0;

    
    std::unique_ptr<Player> player_;

    
    std::unique_ptr<Sprite> hpBarBgSprite_;
    std::unique_ptr<Sprite> hpBarSprite_;
    uint32_t whiteTexIndex_ = 0;
    
    std::list<std::unique_ptr<PlayerBullet>> bullets_;
    
    std::list<std::unique_ptr<PlayerMissile>> missiles_;

    
    std::list<std::unique_ptr<Enemy>> enemies_;
    bool hasEnemySpawned_ = false; 
    
    std::list<std::unique_ptr<EnemyBullet>> enemyBullets_;
    
    std::list<std::unique_ptr<Obstacle>> obstacles_;
    std::list<std::unique_ptr<EnhanceRing>> enhanceRings_;

    
#ifdef USE_IMGUI
    bool isDrawCollider_ = true;
#else
    bool isDrawCollider_ = false;
#endif
    float cameraShakeTimer_ = 0.0f;
};
