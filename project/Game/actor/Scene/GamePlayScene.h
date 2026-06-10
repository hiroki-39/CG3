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
#include "Game/actor/Player/Player.h"
#include "Game/actor/Bullet/PlayerBullet.h"
#include "Game/System/Rail.h"
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
    void Finalize() override;

private:
    // ゲーム固有メンバ
    std::vector<std::unique_ptr<Object3d>> modelInstances;

    std::unique_ptr<Camera> camera;
    std::unique_ptr<Camera> debugCamera_;
    Camera* activeCamera_ = nullptr;
    std::unique_ptr<Object3d> cameraObject_;
    
    // レールシステム
    std::unique_ptr<Rail> mainRail_;
    float railProgress_ = 0.0f;
    std::unique_ptr<Model> railModel_;
    std::vector<std::unique_ptr<Object3d>> railVisualizers_;
    bool isDrawRail_ = true;
    std::unique_ptr<Skybox> skybox_;
    bool isPlaying_ = false;

    float kDeltaTime_local_override = 0.0f;

    // 乱数
    std::random_device seedGenerator;
    std::mt19937 randomEngine{ seedGenerator() };

    // 複合エフェクト（Plane, Ring, Cylinder の共存）
    ParticleEffect particleEffect_;

    // プレイヤー
    std::unique_ptr<Player> player_;
    // 弾リスト
    std::list<std::unique_ptr<PlayerBullet>> bullets_;
};