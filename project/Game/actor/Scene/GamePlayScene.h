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
#include "Game/System/RailCameraController.h"
#include "Game/actor/Enemy/Enemy.h"
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
    /// レベルデータを再読み込みする
    /// </summary>
    void ReloadLevel();

    /// <summary>
    /// 敵だけを再読み込み（リスポーン）する
    /// </summary>
    void ReloadEnemiesOnly();

private:
    // ゲーム固有メンバ
    std::vector<std::unique_ptr<Object3d>> modelInstances;

    std::unique_ptr<Camera> camera;
    std::unique_ptr<Camera> debugCamera_;
    Camera* activeCamera_ = nullptr;
    std::unique_ptr<Object3d> cameraObject_;
    
    // レールシステム
    std::vector<std::unique_ptr<Rail>> mainRails_;
    std::unique_ptr<RailCameraController> railCameraController_;
    float gameSpeed_ = 1.0f;
    std::unique_ptr<Model> railModel_;
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

    // カメラの補間用
    Vector3 currentCameraRot_ = {0.0f, 0.0f, 0.0f};
    float lastCameraYaw_ = 0.0f;
    float currentCameraBank_ = 0.0f;

    float kDeltaTime_local_override = 0.0f;

    // 乱数
    std::random_device seedGenerator;
    std::mt19937 randomEngine{ seedGenerator() };

    // エフェクト群
    ParticleEffect thrusterEffect_;   // プレイヤースラスター用
    ParticleEffect explosionEffect_;  // 敵撃破時の爆発用
    ParticleEffect hitEffect_;        // 弾着弾時のヒット用
    ParticleEffect dodgeEffect_;        // 弾着弾時のヒット用

    // ImGuiのエディタで編集するエフェクトのインデックス (0:Thruster, 1:Explosion, 2:Hit)
    int currentEditEffectIndex_ = 0;

    // プレイヤー
    std::unique_ptr<Player> player_;
    // 弾リスト
    std::list<std::unique_ptr<PlayerBullet>> bullets_;

    // 敵リスト
    std::list<std::unique_ptr<Enemy>> enemies_;

    // コライダーのワイヤーフレーム描画
#ifdef USE_IMGUI
    bool isDrawCollider_ = true;
#else
    bool isDrawCollider_ = false;
#endif
};