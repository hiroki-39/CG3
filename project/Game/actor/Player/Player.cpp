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
     
    // 奥の照準（小）の初期化
    reticle_ = std::make_unique<Object3d>();
    reticle_->Initialize(object3dCommon);
    reticle_->SetModel("crossHair.obj");
    reticle_->GetModel()->SetColor(reticleColor_);
    reticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    reticle_->SetEnvironmentCoefficient(0.0f);
    reticle_->SetEnableLighting(false);
    reticle_->SetSelectLightings(0);
    reticle_->SetScale(Vector3(1.6f, 1.6f, 1.6f));
    
    // 手前の照準（大）の初期化
    frontReticle_ = std::make_unique<Object3d>();
    frontReticle_->Initialize(object3dCommon);
    frontReticle_->SetModel("crossHair.obj");
    // 手前の照準は少し透明度を下げて邪魔にならないようにする
    Vector4 frontColor = reticleColor_;
    frontColor.w = 1.0f;
    frontReticle_->GetModel()->SetColor(frontColor);
    frontReticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    frontReticle_->SetEnvironmentCoefficient(0.0f);
    frontReticle_->SetEnableLighting(false);
    frontReticle_->SetSelectLightings(0);
    frontReticle_->SetScale(Vector3(1.7f, 1.7f, 1.7f)); 
    
    // 照準の初期位置（カメラの奥）
    reticlePosition_ = { 0.0f, 0.0f, 40.0f }; 

    // 初期状態としてグレースケールをOFFにしておく
    if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
        pp->SetEffectActive("Grayscale", false);
    }
    // コライダー可視化用オブジェクト
    ModelManager::GetInstance()->LoadModel("collider_cube_player.obj");
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_cube_player.obj"); // プレイヤー専用の立方体モデル
    colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色
    colliderObject_->SetEnvironmentCoefficient(0.0f);
    colliderObject_->SetEnableLighting(false);
    colliderObject_->SetScale(colliderSize_); // プレイヤーの当たり判定のサイズ
}

void Player::OnCollision() {
    if (invincibilityTimer_ > 0.0f || isDead_ || isRolling_) return; // 無敵中、死亡時、またはローリング中（回避）は無効
    
    hp_--;
    if (hp_ <= 0) {
        isDead_ = true;
    } else {
        invincibilityTimer_ = 60.0f; // 1秒間無敵
    }
}

void Player::Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera, float gameSpeed) {
    Move(gameSpeed);
    Attack(bullets, parentCamera, gameSpeed);

    if (invincibilityTimer_ > 0.0f) {
        invincibilityTimer_ -= gameSpeed;
    }

    object_->Update();
    
    if (accessory_) {
        Vector3 rot = accessory_->GetRotation();
        rot.y += 0.05f * gameSpeed;
        accessory_->SetRotation(rot);
        accessory_->Update();
    }
    
    if (colliderObject_) {
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
    
    bool shouldDrawPlayer = true;
    if (invincibilityTimer_ > 0.0f) {
        if (std::fmod(invincibilityTimer_ / 4.0f, 2.0f) < 1.0f) {
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
        frontColor.w = 1.0f; // 手前も透過させない（モデル共有問題回避）
        frontReticle_->GetModel()->SetColor(frontColor);
    }
}

Vector3 Player::GetReticleWorldPosition() const {
    if (!reticle_) return {0,0,0};
    const Matrix4x4& mat = reticle_->GetmatWorld();
    return { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
}

void Player::Move(float gameSpeed) {
    if (!input_) return;

    // --- ダブルタップタイマー更新 ---
    if (lastQPressTime_ < 1000.0f) lastQPressTime_ += gameSpeed;
    if (lastEPressTime_ < 1000.0f) lastEPressTime_ += gameSpeed;

    // --- ローリング（回避）判定 ---
    if (!isRolling_) {
        if (input_->TriggerKey(DIK_Q)) {
            if (lastQPressTime_ <= doubleTapThreshold_) {
                isRolling_ = true;
                rollTimer_ = 0.0f;
                rollDirection_ = -1.0f; // 左
                isDodgeTriggered_ = true;
                lastQPressTime_ = 1000.0f;
            } else {
                lastQPressTime_ = 0.0f;
            }
        } else if (input_->TriggerKey(DIK_E)) {
            if (lastEPressTime_ <= doubleTapThreshold_) {
                isRolling_ = true;
                rollTimer_ = 0.0f;
                rollDirection_ = 1.0f; // 右
                isDodgeTriggered_ = true;
                lastEPressTime_ = 1000.0f;
            } else {
                lastEPressTime_ = 0.0f;
            }
        }
    }

    if (isRolling_) {
        rollTimer_ += gameSpeed;
        if (rollTimer_ >= rollMaxTime_) {
            isRolling_ = false;
        }
    }

    // --- ブーストとブレーキ（Z軸方向移動） ---
    isBoosting_ = input_->PushKey(DIK_LSHIFT);
    bool isBraking = input_->PushKey(DIK_LCONTROL);
    
    float targetZ = 20.0f; 
    if (isBoosting_) {
        targetZ = 35.0f; 
    } else if (isBraking) {
        targetZ = 5.0f;  
    }
    logicalPosition_.z += (targetZ - logicalPosition_.z) * 0.1f * gameSpeed;

    // --- 自機（XY座標）の直接移動 ---
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
    if (input_->PushKey(DIK_Q) || input_->PushKey(DIK_E)) {
        currentSpeedX *= 1.8f; 
    }

    if (input_->PushKey(DIK_A) || input_->PushKey(DIK_LEFT)) {
        targetVelX = -currentSpeedX;
    }
    if (input_->PushKey(DIK_D) || input_->PushKey(DIK_RIGHT)) {
        targetVelX = currentSpeedX;
    }

    float accel = 0.15f * gameSpeed;
    velocity_.x += (targetVelX - velocity_.x) * accel;
    velocity_.y += (targetVelY - velocity_.y) * accel;

    logicalPosition_.x += velocity_.x * gameSpeed;
    logicalPosition_.y += velocity_.y * gameSpeed;

    logicalPosition_.x = std::clamp(logicalPosition_.x, -playerLimitX_, playerLimitX_);
    logicalPosition_.y = std::clamp(logicalPosition_.y, playerLimitYMin_, playerLimitYMax_);

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

    if (!isRolling_) {
        if (input_->PushKey(DIK_Q)) {
            targetBank = 1.5708f;
        } else if (input_->PushKey(DIK_E)) {
            targetBank = -1.5708f;
        }
    }

    float lerpSpeed = 0.12f * gameSpeed; 
    currentPitch_ += (targetPitch - currentPitch_) * lerpSpeed;
    currentYaw_ += (targetYaw - currentYaw_) * lerpSpeed;
    currentBank_ += (targetBank - currentBank_) * lerpSpeed;

    float finalBank = currentBank_;
    if (isRolling_) {
        float rollAngle = (static_cast<float>(rollTimer_) / rollMaxTime_) * 3.14159265f * 2.0f;
        finalBank += rollAngle * -rollDirection_;
    }
    
    float reticleDistance = 40.0f;
    Vector3 targetReticlePos;
    targetReticlePos.x = logicalPosition_.x + std::sin(currentYaw_) * reticleDistance;
    targetReticlePos.y = logicalPosition_.y - std::sin(currentPitch_) * reticleDistance;
    targetReticlePos.z = logicalPosition_.z + reticleDistance;

    float reticleLerp = 0.2f * gameSpeed;
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

    // 画面上での見た目のピッチとヨー（基本の誇張具合）
    float screenPitch = currentPitch_ * 2.0f;
    float screenYaw   = currentYaw_ * 3.5f;

    // ローリングを含めた現在のZ軸回転（finalBank）に応じて、X軸とY軸の回転を2D回転させる
    // これにより、機体がどの角度にロールしていても、移動キーを押した際に「画面上の見た目の上下左右」に正しく機首が向く
    float visualPitch = screenPitch * std::cos(finalBank) + screenYaw * std::sin(finalBank);
    float visualYaw   = -screenPitch * std::sin(finalBank) + screenYaw * std::cos(finalBank);

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
}

void Player::Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera, float gameSpeed) {
    if (!input_) return;

    if (attackTimer_ > 0.0f) {
        attackTimer_ -= gameSpeed;
    }

    if (attackTimer_ <= 0.0f && (input_->PushKey(DIK_SPACE))) {
        attackTimer_ = attackInterval_; 
        
        const Matrix4x4& mat = object_->GetmatWorld();
        Vector3 playerPos = { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
        
        Vector3 targetPos = GetReticleWorldPosition();
        
        if (assistTarget_ && !assistTarget_->IsDead()) {
            targetPos = assistTarget_->GetColliderCenter();
        }
        
        Vector3 direction = {
            targetPos.x - playerPos.x,
            targetPos.y - playerPos.y,
            targetPos.z - playerPos.z
        };
        
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
                object_->GetModel()->SetColor(color_);
                object_->SetEnvironmentTextureIndex(skyboxTexIndex_);
                object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
                if (accessory_) {
                    accessory_->SetParent(object_.get());
                }
            }
        }
        
        float col[4] = { color_.x, color_.y, color_.z, color_.w };
        if (ImGui::ColorEdit4("Color", col)) {
            color_ = { col[0], col[1], col[2], col[3] };
            if (object_) {
                object_->GetModel()->SetColor(color_);
            }
        }
        
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
        ImGui::DragFloat("Roll Max Time", &rollMaxTime_, 1.0f, 1.0f, 60.0f); 
    }
    if (ImGui::Button("Save Settings")) {
        SaveSettings("resources/json/player/player_settings.json");
    }
    
    ImGui::End();
#endif
}

bool Player::IsBanking() const {
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
