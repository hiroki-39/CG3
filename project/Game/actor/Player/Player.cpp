#include "Player.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "externals/nlohmann/json.hpp"
#include <fstream>
#include <algorithm>
#include "Game/Actor/Enemy/Enemy.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"

void Player::Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex) {
    object3dCommon_ = object3dCommon;
    input_ = EngineServices::GetInstance()->GetInput();

    skyboxTexIndex_ = skyboxTexIndex;

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName_);
    object_->GetModel()->SetColor(color_);
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);
    object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
    object_->SetScale(playerScale_);
     
    //// 繧｢繧ｯ繧ｻ繧ｵ繝ｪ縺ｮ蛻晄悄蛹・
    //accessory_ = std::make_unique<Object3d>();
    //accessory_->Initialize(object3dCommon);
    //accessory_->SetModel("suzanne.obj");
    //accessory_->GetModel()->SetColor({0.8f, 0.8f, 0.0f, 1.0f});
    //accessory_->SetEnvironmentTextureIndex(skyboxTexIndex);
    //accessory_->SetEnvironmentCoefficient(0.0f);
    //accessory_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    //accessory_->SetTranslate(Vector3(1.5f, 1.0f, 0.0f)); // 繝励Ξ繧､繝､繝ｼ縺ｮ蜿ｳ荳翫↓驟咲ｽｮ
    //accessory_->SetParent(object_.get()); // 隕ｪ蟄宣未菫ゅｒ險ｭ螳・

    // 螂･縺ｮ辣ｧ貅厄ｼ亥ｰ擾ｼ峨・蛻晄悄蛹・
    reticle_ = std::make_unique<Object3d>();
    reticle_->Initialize(object3dCommon);
    reticle_->SetModel("crossHair.obj");
    reticle_->GetModel()->SetColor(reticleColor_);
    reticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    reticle_->SetEnvironmentCoefficient(0.0f);
    reticle_->SetEnableLighting(false);
    reticle_->SetSelectLightings(0);
    reticle_->SetScale(Vector3(1.6f, 1.6f, 1.6f));
    
    // 謇句燕縺ｮ辣ｧ貅厄ｼ亥､ｧ・峨・蛻晄悄蛹・
    frontReticle_ = std::make_unique<Object3d>();
    frontReticle_->Initialize(object3dCommon);
    frontReticle_->SetModel("crossHair.obj");
    // 謇句燕縺ｮ辣ｧ貅悶・蟆代＠騾乗・蠎ｦ繧剃ｸ九￡縺ｦ驍ｪ鬲斐↓縺ｪ繧峨↑縺・ｈ縺・↓縺吶ｋ
    Vector4 frontColor = reticleColor_;
    frontColor.w = 1.0f;
    frontReticle_->GetModel()->SetColor(frontColor);
    frontReticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    frontReticle_->SetEnvironmentCoefficient(0.0f);
    frontReticle_->SetEnableLighting(false);
    frontReticle_->SetSelectLightings(0);
    frontReticle_->SetScale(Vector3(1.7f, 1.7f, 1.7f)); 
    
    // 辣ｧ貅悶・蛻晄悄菴咲ｽｮ・医き繝｡繝ｩ縺ｮ螂･・・
    reticlePosition_ = { 0.0f, 0.0f, 40.0f }; 

    // 蛻晄悄迥ｶ諷九→縺励※繧ｰ繝ｬ繝ｼ繧ｹ繧ｱ繝ｼ繝ｫ繧丹FF縺ｫ縺励※縺翫￥
    if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
        pp->SetEffectActive("Grayscale", false);
    }
    // 繧ｳ繝ｩ繧､繝繝ｼ蜿ｯ隕門喧逕ｨ繧ｪ繝悶ず繧ｧ繧ｯ繝・
    ModelManager::GetInstance()->LoadModel("collider_cube_player.obj");
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_cube_player.obj"); // 繝励Ξ繧､繝､繝ｼ蟆ら畑縺ｮ遶区婿菴薙Δ繝・Ν
    colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 邱題牡
    colliderObject_->SetEnvironmentCoefficient(0.0f);
    colliderObject_->SetEnableLighting(false);
    colliderObject_->SetScale(colliderSize_); // 繝励Ξ繧､繝､繝ｼ縺ｮ蠖薙◆繧雁愛螳壹・繧ｵ繧､繧ｺ
}

void Player::OnCollision() {
    if (invincibilityTimer_ > 0 || isDead_ || isRolling_) return; // 辟｡謨ｵ荳ｭ縲∵ｭｻ莠｡譎ゅ√∪縺溘・繝ｭ繝ｼ繝ｪ繝ｳ繧ｰ荳ｭ・亥屓驕ｿ・峨・辟｡蜉ｹ
    
    hp_--;
    if (hp_ <= 0) {
        isDead_ = true;
    } else {
        invincibilityTimer_ = 60; // 1遘帝俣辟｡謨ｵ
        
        // 陲ｫ蠑ｾ譎ゅき繝｡繝ｩ繧ｷ繧ｧ繧､繧ｯ繧・・繧ｹ繝医お繝輔ぉ繧ｯ繝医↑縺ｩ繧貞・繧後ｋ縺ｮ繧り憶縺・
    }
}

void Player::Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera) {
    Move();
    Attack(bullets, parentCamera);

    if (invincibilityTimer_ > 0) {
        invincibilityTimer_--;
    }

    object_->Update();
    
    if (accessory_) {
        // 繧｢繧ｯ繧ｻ繧ｵ繝ｪ繧貞屓霆｢縺輔○繧九ョ繝｢
        Vector3 rot = accessory_->GetRotation();
        rot.y += 0.05f;
        accessory_->SetRotation(rot);
        accessory_->Update();
    }
    
    if (colliderObject_) {
        // 隕ｪ縺ｮ險ｭ螳壹・GamePlayScene蛛ｴ縺ｧ陦後≧
        colliderObject_->SetTranslate(object_->GetTranslate());
        colliderObject_->SetRotation(object_->GetRotation());
        colliderObject_->Update();
    }
}

void Player::Draw() {
    if (reticle_) {
        reticle_->Draw();
    }
    if (frontReticle_) {
        frontReticle_->Draw();
    }
    
    // 辟｡謨ｵ譎る俣荳ｭ縺ｯ轤ｹ貊・＆縺帙ｋ (4繝輔Ξ繝ｼ繝�縺ｫ1蝗樣撼陦ｨ遉ｺ)
    bool shouldDrawPlayer = true;
    if (invincibilityTimer_ > 0) {
        if ((invincibilityTimer_ / 4) % 2 == 0) {
            shouldDrawPlayer = false;
        }
    }
    
    if (shouldDrawPlayer) {
        if (object_) {
            object_->Draw();
        }
        if (accessory_) {
            accessory_->Draw();
        }
    }
}

void Player::DrawCollider() {
    if (colliderObject_) {
        colliderObject_->Draw();
    }
}

void Player::SetReticleColor(const Vector4& color) {
    reticleColor_ = color;
    if (reticle_) {
        reticle_->GetModel()->SetColor(reticleColor_);
    }
    if (frontReticle_) {
        Vector4 frontColor = reticleColor_;
        frontColor.w = 1.0f; // 謇句燕繧る城℃縺輔○縺ｪ縺・ｼ医Δ繝・Ν蜈ｱ譛牙撫鬘悟屓驕ｿ・・
        frontReticle_->GetModel()->SetColor(frontColor);
    }
}

Vector3 Player::GetReticleWorldPosition() const {
    if (!reticle_) return {0,0,0};
    // 辣ｧ貅悶が繝悶ず繧ｧ繧ｯ繝医・繝ｯ繝ｼ繝ｫ繝牙ｺｧ讓吶ｒ繝槭ヨ繝ｪ繝・け繧ｹ縺九ｉ蜿門ｾ・
    const Matrix4x4& mat = reticle_->GetmatWorld();
    return { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
}

void Player::Move() {
    if (!input_) return;

    // --- ダブルタップタイマー更新 ---
    if (lastQPressTime_ < 1000) lastQPressTime_++;
    if (lastEPressTime_ < 1000) lastEPressTime_++;

    // --- ローリング（回避）判定 ---
    if (!isRolling_) {
        if (input_->TriggerKey(DIK_Q)) {
            if (lastQPressTime_ <= doubleTapThreshold_) {
                isRolling_ = true;
                rollTimer_ = 0;
                rollDirection_ = -1.0f; // 左
                isDodgeTriggered_ = true;
                lastQPressTime_ = 1000;
            } else {
                lastQPressTime_ = 0;
            }
        } else if (input_->TriggerKey(DIK_E)) {
            if (lastEPressTime_ <= doubleTapThreshold_) {
                isRolling_ = true;
                rollTimer_ = 0;
                rollDirection_ = 1.0f; // 右
                isDodgeTriggered_ = true;
                lastEPressTime_ = 1000;
            } else {
                lastEPressTime_ = 0;
            }
        }
    }

    if (isRolling_) {
        rollTimer_++;
        // ローリング中の自動横移動を削除（その場にとどまるようにする）
        if (rollTimer_ >= rollMaxTime_) {
            isRolling_ = false;
        }
    }

    // --- ブーストとブレーキ（Z軸方向移動） ---
    isBoosting_ = input_->PushKey(DIK_LSHIFT);
    bool isBraking = input_->PushKey(DIK_LCONTROL);
    
    float targetZ = 20.0f; // ニュートラル位置
    if (isBoosting_) {
        targetZ = 35.0f; // ブースト時の前進限界
    } else if (isBraking) {
        targetZ = 5.0f;  // ブレーキ時の後退限界
    }
    logicalPosition_.z += (targetZ - logicalPosition_.z) * 0.1f;

    // --- 自機（XY座標）の直接移動（スターフォックス風・滑らか補間） ---
    float currentSpeed = speed_;
    if (isBoosting_) currentSpeed *= 1.5f;

    float targetVelX = 0.0f;
    float targetVelY = 0.0f;

    if (input_->PushKey(DIK_W) || input_->PushKey(DIK_UP)) {
        targetVelY = currentSpeed;
    }
    if (input_->PushKey(DIK_S) || input_->PushKey(DIK_DOWN)) {
        targetVelY = -currentSpeed;
    }
    float currentSpeedX = currentSpeed;
    // QまたはEで機体を傾けている間は、左右の旋回（移動）速度を上げる
    if (input_->PushKey(DIK_Q) || input_->PushKey(DIK_E)) {
        currentSpeedX *= 1.8f; 
    }

    if (input_->PushKey(DIK_A) || input_->PushKey(DIK_LEFT)) {
        targetVelX = -currentSpeedX;
    }
    if (input_->PushKey(DIK_D) || input_->PushKey(DIK_RIGHT)) {
        targetVelX = currentSpeedX;
    }

    // 加速度（補間割合：0.15 なら毎フレーム15%ずつ目標速度に近づく）
    float accel = 0.15f;
    velocity_.x += (targetVelX - velocity_.x) * accel;
    velocity_.y += (targetVelY - velocity_.y) * accel;

    logicalPosition_.x += velocity_.x;
    logicalPosition_.y += velocity_.y;

    // 自機がカメラ外に出ないように制限
    logicalPosition_.x = std::clamp(logicalPosition_.x, -playerLimitX_, playerLimitX_);
    logicalPosition_.y = std::clamp(logicalPosition_.y, playerLimitYMin_, playerLimitYMax_);

    // --- 機体の傾き計算（照準用の論理的な角度） ---
    float targetPitch = 0.0f;
    float targetYaw = 0.0f;
    float targetBank = 0.0f;

    if (input_->PushKey(DIK_W) || input_->PushKey(DIK_UP)) {
        if (logicalPosition_.y < playerLimitYMax_) targetPitch = -0.4f; 
    }
    if (input_->PushKey(DIK_S) || input_->PushKey(DIK_DOWN)) {
        if (logicalPosition_.y > playerLimitYMin_) targetPitch = 0.4f;
    }
    
    if (input_->PushKey(DIK_A) || input_->PushKey(DIK_LEFT)) {
        if (logicalPosition_.x > -playerLimitX_) targetYaw = -0.2f; 
    }
    if (input_->PushKey(DIK_D) || input_->PushKey(DIK_RIGHT)) {
        if (logicalPosition_.x < playerLimitX_) targetYaw = 0.2f;
    }

    // Q/Eでさらに深く（90度）傾ける
    if (!isRolling_) {
        if (input_->PushKey(DIK_Q)) {
            targetBank = 1.5708f;
        } else if (input_->PushKey(DIK_E)) {
            targetBank = -1.5708f;
        }
    }

    // 傾きの反応速度を下げて、より滑らか（スムーズ）に傾き・戻りが行われるようにする
    float lerpSpeed = 0.12f; 
    currentPitch_ += (targetPitch - currentPitch_) * lerpSpeed;
    currentYaw_ += (targetYaw - currentYaw_) * lerpSpeed;
    currentBank_ += (targetBank - currentBank_) * lerpSpeed;

    float finalBank = currentBank_;
    if (isRolling_) {
        float rollAngle = (static_cast<float>(rollTimer_) / rollMaxTime_) * 3.14159265f * 2.0f;
        finalBank += rollAngle * -rollDirection_;
    }
    
    // --- 照準（レティクル）の配置（滑らか補間） ---
    float reticleDistance = 40.0f;
    Vector3 targetReticlePos;
    targetReticlePos.x = logicalPosition_.x + std::sin(currentYaw_) * reticleDistance;
    targetReticlePos.y = logicalPosition_.y - std::sin(currentPitch_) * reticleDistance;
    targetReticlePos.z = logicalPosition_.z + reticleDistance;

    // 照準がカクカク動かないように、少し遅れて滑らかに追従させる
    float reticleLerp = 0.2f;
    reticlePosition_.x += (targetReticlePos.x - reticlePosition_.x) * reticleLerp;
    reticlePosition_.y += (targetReticlePos.y - reticlePosition_.y) * reticleLerp;
    reticlePosition_.z += (targetReticlePos.z - reticlePosition_.z) * reticleLerp;
    
    reticle_->SetTranslate(reticlePosition_);
    reticle_->Update();

    if (frontReticle_) {
        Vector3 frontReticlePos = {
            logicalPosition_.x + (reticlePosition_.x - logicalPosition_.x) * 0.4f,
            logicalPosition_.y + (reticlePosition_.y - logicalPosition_.y) * 0.4f,
            logicalPosition_.z + (reticlePosition_.z - logicalPosition_.z) * 0.4f
        };
        frontReticle_->SetTranslate(frontReticlePos);
        frontReticle_->Update();
    }

    // 3D空間のパースペクティブ（遠近感）により、自機が照準を向いていないように見えてしまう問題の対策
    // 論理的な角度とは別に、モデルの見た目（描画用）の角度を大げさに回す
    
    // Q/Eキーによるバンク角（最大90度=1.5708）の進行度合い（-1.0 ～ 1.0）
    // バグ修正: ローリング中（finalBankが360度回る）に回転が暴れないよう、currentBank_（最大90度の傾き）のみを基準にする
    float bankRatio = currentBank_ / 1.5708f;
    float absBankRatio = std::abs(bankRatio);
    
    // バンク角に応じて、見た目上のPitchとYawを滑らかに入れ替える（ブレンドする）
    // バンク0の時は通常の誇張、バンク90度の時は左右移動がPitchになり、上下移動がYawになる
    float visualPitch = std::lerp(currentPitch_ * 2.0f, currentYaw_ * 4.0f * bankRatio, absBankRatio);
    float visualYaw   = std::lerp(currentYaw_   * 3.5f, currentPitch_ * -1.5f * bankRatio, absBankRatio);

    object_->SetScale(playerScale_);
    object_->SetRotation(Vector3(
        baseRotation_.x + visualPitch + modelRotOffset_.x,
        baseRotation_.y + visualYaw   + modelRotOffset_.y,
        baseRotation_.z + finalBank   + modelRotOffset_.z
    ));
    object_->SetTranslate(Vector3(
        logicalPosition_.x + modelPosOffset_.x, 
        logicalPosition_.y + modelPosOffset_.y, 
        logicalPosition_.z + modelPosOffset_.z
    ));
}void Player::Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera) {
    if (!input_) return;

    // 逋ｺ蟆・俣髫斐・繧ｿ繧､繝槭・譖ｴ譁ｰ
    if (attackTimer_ > 0) {
        attackTimer_--;
    }

    // 繧ｿ繧､繝槭・縺・莉･荳九↑繧臥匱蟆・庄閭ｽ
    // 繧ｹ繝壹・繧ｹ繧ｭ繝ｼ縺ｾ縺溘・繝槭え繧ｹ蟾ｦ繧ｯ繝ｪ繝・け縺ｧ逋ｺ蟆・
    if (attackTimer_ <= 0 && (input_->PushKey(DIK_SPACE))) {
        attackTimer_ = attackInterval_; // 繧ｿ繧､繝槭・繝ｪ繧ｻ繝・ヨ
        
        // 繝励Ξ繧､繝､繝ｼ縺ｮ繝ｯ繝ｼ繝ｫ繝牙ｺｧ讓吶ｒ蜿門ｾ励☆繧・(object_縺ｯ繧ｫ繝｡繝ｩ縺ｮ蟄舌↑縺ｮ縺ｧGetTranslate縺ｯ繝ｭ繝ｼ繧ｫ繝ｫ蠎ｧ讓吶↓縺ｪ縺｣縺ｦ縺励∪縺・
        const Matrix4x4& mat = object_->GetmatWorld();
        Vector3 playerPos = { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
        
        Vector3 targetPos = GetReticleWorldPosition();
        
        // --- 辣ｧ貅悶い繧ｷ繧ｹ繝茨ｼ亥ｼｱ繧・ｼ・---
        // 繧ｿ繝ｼ繧ｲ繝・ヨ縺瑚ｿ代￥縺ｫ縺・ｋ蝣ｴ蜷医∫匱蟆・ｧ貞ｺｦ縺�縺代ｒ謨ｵ縺ｮ譁ｹ縺ｸ蟆代＠陬懈ｭ｣縺吶ｋ・医・繝ｼ繝溘Φ繧ｰ蠑ｾ縺ｫ縺ｯ縺励↑縺・ｼ・
        if (assistTarget_ && !assistTarget_->IsDead()) {
            targetPos = assistTarget_->GetColliderCenter();
        }
        
        // 辣ｧ貅・縺ｾ縺溘・繝ｭ繝・け繧ｪ繝ｳ蠎ｧ讓・縺ｫ蜷代°縺・・繧ｯ繝医Ν繧定ｨ育ｮ・
        Vector3 direction = {
            targetPos.x - playerPos.x,
            targetPos.y - playerPos.y,
            targetPos.z - playerPos.z
        };
        
        // 豁｣隕丞喧
        float length = std::sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (length > 0.0f) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        }
        
        Vector3 velocity = {
            direction.x * bulletSpeed_,
            direction.y * bulletSpeed_,
            direction.z * bulletSpeed_
        };

        // 蠑ｾ繧堤函謌・
        std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerBullet>();
        newBullet->Initialize(object3dCommon_, playerPos, velocity, parentCamera, isLockOn_ ? lockOnTargetEnemy_ : nullptr);
        bullets.push_back(std::move(newBullet));
    }
}

void Player::LoadSettings(const std::string& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        if (j.contains("speed")) speed_ = j["speed"];
        if (j.contains("reticleSpeed")) reticleSpeed_ = j["reticleSpeed"];
        if (j.contains("moveLimitX")) moveLimitX_ = j["moveLimitX"];
        if (j.contains("moveLimitY")) moveLimitY_ = j["moveLimitY"];
        if (j.contains("attackInterval")) attackInterval_ = j["attackInterval"];
        if (j.contains("rollMaxTime")) rollMaxTime_ = j["rollMaxTime"];
        if (j.contains("playerLimitX")) playerLimitX_ = j["playerLimitX"];
        if (j.contains("playerLimitYMin")) playerLimitYMin_ = j["playerLimitYMin"];
        if (j.contains("playerLimitYMax")) playerLimitYMax_ = j["playerLimitYMax"];
        if (j.contains("followSpeed")) followSpeed_ = j["followSpeed"];
        if (j.contains("bulletSpeed")) bulletSpeed_ = j["bulletSpeed"];
        if (j.contains("modelName")) modelName_ = j["modelName"];
        if (j.contains("reflection")) reflection_ = j["reflection"];
        if (j.contains("playerScale")) {
            playerScale_ = { j["playerScale"][0], j["playerScale"][1], j["playerScale"][2] };
        }
        if (j.contains("colliderSize")) {
            colliderSize_ = { j["colliderSize"][0], j["colliderSize"][1], j["colliderSize"][2] };
            if (colliderObject_) {
                colliderObject_->SetScale(colliderSize_);
            }
        }
        if (j.contains("hp")) {
            hp_ = j["hp"];
        }
        if (j.contains("maxHp")) {
            maxHp_ = j["maxHp"];
        }
        if (j.contains("color")) {
            auto c = j["color"];
            if (c.is_array() && c.size() == 4) {
                color_ = { c[0], c[1], c[2], c[3] };
            }
        }
        if (j.contains("modelPosOffset")) {
            auto arr = j["modelPosOffset"];
            if (arr.is_array() && arr.size() == 3) modelPosOffset_ = { arr[0], arr[1], arr[2] };
        }
        if (j.contains("modelRotOffset")) {
            auto arr = j["modelRotOffset"];
            if (arr.is_array() && arr.size() == 3) modelRotOffset_ = { arr[0], arr[1], arr[2] };
        }
        file.close();
        
        // 隱ｭ縺ｿ霎ｼ繧薙□險ｭ螳壹ｒ蜿肴丐
        if (object_) {
            object_->SetModel(modelName_);
            object_->GetModel()->SetColor(color_);
            object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
            if (accessory_) {
                accessory_->SetParent(object_.get());
            }
        }
    }
}

void Player::SaveSettings(const std::string& filepath) {
    nlohmann::json j;
    j["speed"] = speed_;
    j["reticleSpeed"] = reticleSpeed_;
    j["moveLimitX"] = moveLimitX_;
    j["moveLimitY"] = moveLimitY_;
    j["attackInterval"] = attackInterval_;
    j["rollMaxTime"] = rollMaxTime_;
    j["playerLimitX"] = playerLimitX_;
    j["playerLimitYMin"] = playerLimitYMin_;
    j["playerLimitYMax"] = playerLimitYMax_;
    j["followSpeed"] = followSpeed_;
    j["bulletSpeed"] = bulletSpeed_;
    j["modelName"] = modelName_;
    j["reflection"] = reflection_;
    j["color"] = { color_.x, color_.y, color_.z, color_.w };
    j["modelPosOffset"] = { modelPosOffset_.x, modelPosOffset_.y, modelPosOffset_.z };
    j["modelRotOffset"] = { modelRotOffset_.x, modelRotOffset_.y, modelRotOffset_.z };
    j["playerScale"] = { playerScale_.x, playerScale_.y, playerScale_.z };
    j["colliderSize"] = { colliderSize_.x, colliderSize_.y, colliderSize_.z };
    j["hp"] = hp_;
    j["maxHp"] = maxHp_;

    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
}

void Player::DrawUI() {
#ifdef USE_IMGUI
    ImGui::Begin("Player Editor");
    
    if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 繝｢繝・Ν螟画峩
        const char* models[] = { "cube.obj", "player.obj", "monsterBall.obj","suzanne.obj"};
        int currentModel = 0;
        for (int i = 0; i < 3; ++i) {
            if (modelName_ == models[i]) {
                currentModel = i;
                break;
            }
        }
        if (ImGui::Combo("Model", &currentModel, models, 3)) {
            modelName_ = models[currentModel];
            if (object_) {
                object_->SetModel(modelName_);
                // 繝｢繝・Ν繧貞､峨∴縺溷ｴ蜷医€∝・蠎ｦ濶ｲ縺ｨ蜿榊ｰ・ｿよ焚繧偵そ繝・ヨ縺吶ｋ
                object_->GetModel()->SetColor(color_);
                object_->SetEnvironmentTextureIndex(skyboxTexIndex_);
                object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
                if (accessory_) {
                    accessory_->SetParent(object_.get());
                }
            }
        }
        
        // 濶ｲ螟画峩
        float col[4] = { color_.x, color_.y, color_.z, color_.w };
        if (ImGui::ColorEdit4("Color", col)) {
            color_ = { col[0], col[1], col[2], col[3] };
            if (object_) {
                object_->GetModel()->SetColor(color_);
            }
        }
        
        // 譏繧願ｾｼ縺ｿ蛻・ｊ譖ｿ縺・
        if (ImGui::Checkbox("Reflection", &reflection_)) {
            if (object_) {
                object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Transform Offset & Scale");
        float posOffset[3] = { modelPosOffset_.x, modelPosOffset_.y, modelPosOffset_.z };
        if (ImGui::DragFloat3("Position Offset", posOffset, 0.1f)) {
            modelPosOffset_ = { posOffset[0], posOffset[1], posOffset[2] };
        }
        float rotOffset[3] = { modelRotOffset_.x, modelRotOffset_.y, modelRotOffset_.z };
        if (ImGui::DragFloat3("Rotation Offset", rotOffset, 0.05f)) {
            modelRotOffset_ = { rotOffset[0], rotOffset[1], rotOffset[2] };
        }
        float scale[3] = { playerScale_.x, playerScale_.y, playerScale_.z };
        if (ImGui::DragFloat3("Player Scale", &playerScale_.x, 0.01f)) {
            object_->SetScale(playerScale_);
        }
        if (ImGui::DragFloat3("Collider Size", &colliderSize_.x, 0.1f)) {
            if (colliderObject_) colliderObject_->SetScale(colliderSize_);
        }
        ImGui::DragInt("HP", &hp_);
        ImGui::DragInt("Max HP", &maxHp_);
    }
    
    if (ImGui::CollapsingHeader("Movement Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Reticle Speed", &reticleSpeed_, 0.01f, 0.1f, 5.0f);
        ImGui::DragFloat("Follow Speed", &followSpeed_, 0.001f, 0.01f, 1.0f);
        ImGui::DragFloat("Move Limit X", &moveLimitX_, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("Move Limit Y", &moveLimitY_, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("Player Limit X", &playerLimitX_, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("Player Limit Y Min", &playerLimitYMin_, 0.1f, -10.0f, 50.0f);
        ImGui::DragFloat("Player Limit Y Max", &playerLimitYMax_, 0.1f, 1.0f, 50.0f);
        ImGui::DragInt("Roll Max Time", &rollMaxTime_, 1, 1, 60);
    }
    if (ImGui::Button("Save Settings")) {
        SaveSettings("resources/json/player/player_settings.json");
    }
    
    ImGui::End();
#endif
}

bool Player::IsBanking() const {
    // 軽く移動しただけでは出さず、深く旋回（バンク）・急上昇急降下（ピッチ）した時のみトレイルを出す
    // しきい値を厳しめに設定 (バンク: 0.3 -> 0.65, ピッチ: 0.2 -> 0.35)
    return std::abs(currentBank_) > 0.65f || isRolling_ || std::abs(currentPitch_) > 0.35f;
}

Vector3 Player::GetLeftWingPosition() const {
    const Matrix4x4& mat = object_->GetmatWorld();
    float wingSpan = 3.8f;
    Vector3 leftWingLocal = { -wingSpan, 0.0f, -0.5f };
    return Vector3{
        leftWingLocal.x * mat.m[0][0] + leftWingLocal.y * mat.m[1][0] + leftWingLocal.z * mat.m[2][0] + mat.m[3][0],
        leftWingLocal.x * mat.m[0][1] + leftWingLocal.y * mat.m[1][1] + leftWingLocal.z * mat.m[2][1] + mat.m[3][1],
        leftWingLocal.x * mat.m[0][2] + leftWingLocal.y * mat.m[1][2] + leftWingLocal.z * mat.m[2][2] + mat.m[3][2]
    };
}

Vector3 Player::GetRightWingPosition() const {
    const Matrix4x4& mat = object_->GetmatWorld();
    float wingSpan = 3.8f;
    Vector3 rightWingLocal = { wingSpan, 0.0f, -0.5f };
    return Vector3{
        rightWingLocal.x * mat.m[0][0] + rightWingLocal.y * mat.m[1][0] + rightWingLocal.z * mat.m[2][0] + mat.m[3][0],
        rightWingLocal.x * mat.m[0][1] + rightWingLocal.y * mat.m[1][1] + rightWingLocal.z * mat.m[2][1] + mat.m[3][1],
        rightWingLocal.x * mat.m[0][2] + rightWingLocal.y * mat.m[1][2] + rightWingLocal.z * mat.m[2][2] + mat.m[3][2]
    };
}

