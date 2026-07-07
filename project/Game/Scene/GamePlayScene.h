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
#include "Game/System/Rail.h"
#include "Game/System/RailCameraController.h"
#include "Game/Actor/Enemy/Enemy.h"
#include "Game/Actor/Obstacle/Obstacle.h"
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

    /// <summary>
    /// 繝ｬ繝吶Ν繝・・繧ｿ繧貞・隱ｭ縺ｿ霎ｼ縺ｿ縺吶ｋ
    /// </summary>
    void ReloadLevel();

    /// <summary>
    /// 謨ｵ縺縺代ｒ蜀崎ｪｭ縺ｿ霎ｼ縺ｿ・医Μ繧ｹ繝昴・繝ｳ・峨☆繧・
    /// </summary>
    void ReloadEnemiesOnly();

private:
    // 繧ｲ繝ｼ繝蝗ｺ譛峨Γ繝ｳ繝・
    std::vector<std::unique_ptr<Object3d>> modelInstances;

    std::unique_ptr<Camera> camera;
    std::unique_ptr<Camera> debugCamera_;
    Camera* activeCamera_ = nullptr;
    std::unique_ptr<Object3d> cameraObject_;
    
    // 繝ｬ繝ｼ繝ｫ繧ｷ繧ｹ繝・Β
    std::vector<std::unique_ptr<Rail>> mainRails_;
    std::unique_ptr<RailCameraController> railCameraController_;
    float gameSpeed_ = 1.0f;
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

    // 繧ｫ繝｡繝ｩ縺ｮ陬憺俣逕ｨ
    Vector3 currentCameraRot_ = {0.0f, 0.0f, 0.0f};
    float lastCameraYaw_ = 0.0f;
    float currentCameraBank_ = 0.0f;

    // 荵ｱ謨ｰ
    std::random_device seedGenerator;
    std::mt19937 randomEngine{ seedGenerator() };

    // 繧ｨ繝輔ぉ繧ｯ繝育ｾ､
    ParticleEffect thrusterEffect_;   // 繝励Ξ繧､繝､繝ｼ繧ｹ繝ｩ繧ｹ繧ｿ繝ｼ逕ｨ
    ParticleEffect explosionEffect_;  // 謨ｵ謦・ｴ譎ゅ・辷・匱逕ｨ
    ParticleEffect hitEffect_;        // 蠑ｾ逹蠑ｾ譎ゅ・繝偵ャ繝育畑
    ParticleEffect dodgeEffect_;        // 蠑ｾ逹蠑ｾ譎ゅ・繝偵ャ繝育畑

    // ImGui縺ｮ繧ｨ繝・ぅ繧ｿ縺ｧ邱ｨ髮・☆繧九お繝輔ぉ繧ｯ繝医・繧､繝ｳ繝・ャ繧ｯ繧ｹ (0:Thruster, 1:Explosion, 2:Hit)
    int currentEditEffectIndex_ = 0;

    // 繝励Ξ繧､繝､繝ｼ
    std::unique_ptr<Player> player_;
    // 蠑ｾ繝ｪ繧ｹ繝・
    std::list<std::unique_ptr<PlayerBullet>> bullets_;

    // 謨ｵ繝ｪ繧ｹ繝・
    std::list<std::unique_ptr<Enemy>> enemies_;
    // 障害物リスト
    std::list<std::unique_ptr<Obstacle>> obstacles_;

    // 繧ｳ繝ｩ繧､繝€繝ｼ縺ｮ繝ｯ繧､繝､繝ｼ繝輔Ξ繝ｼ繝謠冗判
#ifdef USE_IMGUI
    bool isDrawCollider_ = true;
#else
    bool isDrawCollider_ = false;
#endif
};