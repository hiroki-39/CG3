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
#include "Game/Actor/Enemy/ArmoredTrainBoss.h"
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
    float lockOnMaxDistance_ = 350.0f; // ロックオン最大射程距離     
    
    
    bool isJustDodgeActive_ = false;
    float justDodgeTimer_ = 0.0f;
    float justDodgeMaxTime_ = 60.0f; 
    float justDodgeSlowSpeed_ = 0.2f; 
    std::vector<std::unique_ptr<Model>> railModels_;
    std::unique_ptr<Model> enemyRailModel_;
    std::vector<std::unique_ptr<Object3d>> railVisualizers_;
    std::vector<std::unique_ptr<Object3d>> enemyRailVisualizers_;
#ifdef ENABLE_EDITOR
    bool isDrawRail_ = true;
#else
    bool isDrawRail_ = false;
#endif
    std::unique_ptr<Skybox> skybox_;
#ifdef ENABLE_EDITOR
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
    ParticleEffect ringEffect_;
    ParticleEffect healRingEffect_; // リング取得時のキラキラエフェクト
    ParticleEffect windEffect_;     // 気流線（風の筋）エフェクト

    
    int currentEditEffectIndex_ = 0;

    
    std::unique_ptr<Player> player_;

    
    std::unique_ptr<Sprite> hpBarBgSprite_;
    std::unique_ptr<Sprite> hpBarSprite_;
    std::unique_ptr<Sprite> boostBarBgSprite_;
    std::unique_ptr<Sprite> boostBarSprite_;
    uint32_t whiteTexIndex_ = 0;

    // 強化リング獲得アイコン（ブーストゲージ上部に横2つ配置）
    static const int kMaxEnhanceRingIcons = 2;
    std::array<std::unique_ptr<Sprite>, kMaxEnhanceRingIcons> ringGetOutlineSprites_;
    std::array<std::unique_ptr<Sprite>, kMaxEnhanceRingIcons> ringGetIconSprites_;
    uint32_t ringIconOutlineTex_ = 0;
    uint32_t ringIconTex_ = 0;
    int acquiredEnhanceRingCount_ = 0;
    Vector4 ringIconColor_ = { 1.0f, 0.85f, 0.2f, 1.0f };    // 中身アイコン色（視認性の高いゴールド）
    Vector4 ringOutlineColor_ = { 1.0f, 1.0f, 1.0f, 0.9f }; // 外枠アウトライン色（ホワイト）
    
    std::list<std::unique_ptr<PlayerBullet>> bullets_;
    
    std::list<std::unique_ptr<PlayerMissile>> missiles_;

    
    std::list<std::unique_ptr<Enemy>> enemies_;
    bool hasEnemySpawned_ = false; 
    
    // 装甲列車ボス
    std::unique_ptr<ArmoredTrainBoss> armoredTrainBoss_;
    bool isBossSpawned_ = false;

    std::list<std::unique_ptr<EnemyBullet>> enemyBullets_;
    
    std::list<std::unique_ptr<Obstacle>> obstacles_;
    std::list<std::unique_ptr<EnhanceRing>> enhanceRings_;

    
#ifdef USE_IMGUI
    bool isDrawCollider_ = false; // デバッグ用コライダーはデフォルトOFF
#else
    bool isDrawCollider_ = false;
#endif
    float cameraShakeTimer_ = 0.0f;
    float lastLoadTimeMs_ = 0.0f;
    int score_ = 0;

    // 進行フェーズ
    enum class GamePhase {
        START_CUTSCENE,
        PLAYING,
        GAMEOVER,
        CLEAR
    };
    GamePhase gamePhase_ = GamePhase::START_CUTSCENE;
    float cutsceneTimer_ = 0.0f;
    float missionStartTextTimer_ = 0.0f;
    const float kCutsceneDuration = 3.0f;

    void StartOpeningCutscene();
    void UpdateOpeningCutscene(float dt);
    void DrawCutsceneUI();

    // ゲームオーバー演出（GameOver Sequence）
    enum class GameOverStep {
        FALLING,        // 被弾・地上へ落下中（カメラ停止、自機キリモミ回転、黒煙放出）
        EXPLODED,       // 地上激突・大爆発（画面揺れ、機体非表示、余韻）
        SHOW_UI         // ゲームオーバーUI表示（「GAME OVER」、リトライ/タイトル選択待ち）
    };
    enum class GameOverMenuOption {
        Retry = 0,
        Title = 1
    };
    GameOverStep gameOverStep_ = GameOverStep::FALLING;
    GameOverMenuOption gameOverSelectedOption_ = GameOverMenuOption::Retry;
    float gameOverTimer_ = 0.0f;
    Vector3 gameOverPlayerFallPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 gameOverPlayerFallRot_ = { 0.0f, 0.0f, 0.0f };
    float gameOverFallVelocity_ = 0.0f;

    void StartGameOverSequence();
    void UpdateGameOverSequence(float dt);
    void DrawGameOverUI();

    // ゲームクリア演出（GameClear Sequence）
    enum class ClearStep {
        ASCENDING,      // 自機が上空へ急上昇・飛び去る（カメラ停止、スラスター噴射）
        SHOW_UI         // 「MISSION COMPLETE」、スコア表示、SPACEでタイトルへ戻る入力待ち
    };
    ClearStep clearStep_ = ClearStep::ASCENDING;
    float clearTimer_ = 0.0f;
    float clearTotalTimer_ = 0.0f;
    Vector3 clearStartPlayerPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 clearPlayerAscentPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 clearPlayerAscentRot_ = { 0.0f, 0.0f, 0.0f };
    float clearAscentSpeed_ = 0.0f;

    void StartClearSequence();
    void UpdateClearSequence(float dt);
    void DrawClearUI();
};

