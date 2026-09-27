#pragma once
#include <vector>
#include <memory>
#include <random>
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Core/Framework/BaseScene.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include "KHEngine/Graphics/3d/Skybox/Skybox.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Graphics/3d/Particle/ParticleEffect.h"
#include "KHEngine/Math/Vector3.h"
#include "KHEngine/Math/Vector4.h"

class TitleScene : public BaseScene
{
public:
    void Initialize();
    void Update();
    void Draw();
    void Finalize();

private:
    // 3D環境
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Skybox> skybox_;
    std::unique_ptr<Object3d> playerObj_;

    // パーティクルエフェクト（スラスターブースト・気流粒子）
    ParticleEffect thrusterEffect_;
    ParticleEffect windEffect_;
    std::mt19937 randomEngine_;

    // スラスターノズル位置オフセット（モデル原点からのローカル座標。後方へ -2.15f）
    Vector3 nozzleOffset_ = { 0.0f, -0.20f, -2.15f };

    // 自機パラメータ（定位置: (0, 0, -3.0)、回転: すべて 0）
    Vector3 targetPos_ = { 0.0f, 0.0f, -3.0f };
    Vector3 playerPos_ = { 0.0f, 0.0f, -3.0f };
    Vector3 playerRot_ = { 0.0f, 0.0f, 0.0f }; // すべて 0
    Vector3 playerScale_ = { 0.5f, 0.5f, 0.5f };
    Vector4 playerColor_ = { 0.155f, 0.155f, 0.155f, 1.0f };
    float envCoefficient_ = 0.6f;

    // カメラパラメータ（定点・固定カメラ）
    Vector3 cameraPos_ = { 0.0f, 0.7f, -8.5f };
    Vector3 cameraRot_ = { 0.05f, 0.0f, 0.0f };

    // 登場（前進）演出パラメータ
    bool isIntro_ = true;
    float introTimer_ = 0.0f;
    float introDuration_ = 1.5f;
    Vector3 startPos_ = { 0.0f, -0.2f, -16.0f };

    // 降下演出パラメータ（決定ボタン押下時: 回転はZ軸のみ、右回転して腹を見せながら大回り下降）
    bool isDiving_ = false;
    float diveTimer_ = 0.0f;
    float diveDuration_ = 1.2f;
    Vector3 diveStartPos_ = { 0.0f, 0.0f, -3.0f };
    Vector3 diveStartRot_ = { 0.0f, 0.0f, 0.0f };
    float diveRollAngle_ = -3.14159265f; // 右回転（180度半回転して完全に腹を見せる）
    float diveArcWidthX_ = 7.0f;        // 右への大回り膨らみ幅
    float diveDropDistanceY_ = 22.0f;   // 下降距離
    float diveForwardDistanceZ_ = 24.0f; // 前進距離

    // ホバリング演出パラメータ
    float idleTimer_ = 0.0f;
    bool isHovering_ = true;
    float hoverAmplitude_ = 0.08f;
    float hoverSpeed_ = 1.6f;
};