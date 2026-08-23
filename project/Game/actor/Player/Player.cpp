#include "Player.h"
#include "Game/Actor/Bullet/PlayerMissile.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "externals/nlohmann/json.hpp"
#include <fstream>
#include <algorithm>
#include "Game/Actor/Enemy/Enemy.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"

void Player::Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex) {
    // プレイヤーと関連オブジェクトの初期化
    object3dCommon_ = object3dCommon;
    input_ = EngineServices::GetInstance()->GetInput();

    skyboxTexIndex_ = skyboxTexIndex;

    // プレイヤー本体のモデル設定
    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName_);
    object_->GetModel()->SetColor(color_);
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);
    object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
    object_->SetScale(playerScale_);
     
    
    // 奥側のレティクル（照準）設定
    reticle_ = std::make_unique<Object3d>();
    reticle_->Initialize(object3dCommon);
    reticle_->SetModel("crossHair.obj");
    reticle_->GetModel()->SetColor(reticleColor_);
    reticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    reticle_->SetEnvironmentCoefficient(0.0f);
    reticle_->SetEnableLighting(false);
    reticle_->SetSelectLightings(0);
    reticle_->SetScale(Vector3(1.6f, 1.6f, 1.6f));
    
    
    frontReticle_ = std::make_unique<Object3d>();
    frontReticle_->Initialize(object3dCommon);
    frontReticle_->SetModel("crossHair.obj");
    
    Vector4 frontColor = reticleColor_;
    frontColor.w = 1.0f;
    frontReticle_->GetModel()->SetColor(frontColor);
    frontReticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    frontReticle_->SetEnvironmentCoefficient(0.0f);
    frontReticle_->SetEnableLighting(false);
    frontReticle_->SetSelectLightings(0);
    frontReticle_->SetScale(Vector3(1.7f, 1.7f, 1.7f)); 
    
    
    reticlePosition_ = { 0.0f, 0.0f, 40.0f }; 

    
    if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
        pp->SetEffectActive("Grayscale", false);
    }
    
    ModelManager::GetInstance()->LoadModel("collider_cube_player.obj");
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_cube_player.obj"); 
    colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); 
    colliderObject_->SetEnvironmentCoefficient(0.0f);
    colliderObject_->SetEnableLighting(false);
    colliderObject_->SetScale(colliderSize_); 

    
    ModelManager::GetInstance()->LoadModel("missile.obj");
    ModelManager::GetInstance()->LoadModel("RockOn.obj");
    for (int i = 0; i < MAX_MISSILES; ++i) {
        mountedMissiles_[i] = std::make_unique<Object3d>();
        mountedMissiles_[i]->Initialize(object3dCommon);
        mountedMissiles_[i]->SetModel("missile.obj");
        mountedMissiles_[i]->SetParent(object_.get());
        float xOffset = 0.0f;
        float zOffset = 0.0f;
        if (i == 0) { xOffset = -1.5f; zOffset = 0.0f; } 
        else if (i == 1) { xOffset = 1.5f; zOffset = 0.0f; } 
        else if (i == 2) { xOffset = -2.8f; zOffset = 0.0f; } 
        else if (i == 3) { xOffset = 2.8f; zOffset = 0.0f; } 
        
        float yOffset = -0.2f;
        mountedMissiles_[i]->SetTranslate({xOffset, yOffset, zOffset});
        mountedMissiles_[i]->SetScale({1.0f, 1.0f, 1.0f}); 
        
        lockOnReticles_[i] = std::make_unique<Object3d>();
        lockOnReticles_[i]->Initialize(object3dCommon);
        lockOnReticles_[i]->SetModel("RockOn.obj");
        lockOnReticles_[i]->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        lockOnReticles_[i]->SetScale({0.001f, 0.001f, 0.001f}); 
        lockOnReticles_[i]->SetEnableLighting(false);
        
        mountedMissiles_[i]->Update();
        lockOnReticles_[i]->Update();
    }
}

// プレイヤー被弾時の処理
void Player::OnCollision() {
    if (invincibilityTimer_ > 0.0f || isDead_ || isRolling_) return; 
    
    hp_ -= 1000;
    isDoubleShot_ = false;
    if (hp_ <= 0) {
        isDead_ = true;
    } else {
        invincibilityTimer_ = 60.0f; 
    }
}

// プレイヤーの毎フレームの更新処理
void Player::Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, std::list<std::unique_ptr<PlayerMissile>>& missiles, const std::list<std::unique_ptr<Enemy>>& enemies, Object3d* parentCamera, float gameSpeed) {
    Move(gameSpeed);
    Attack(bullets, missiles, enemies, parentCamera, gameSpeed);

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

    for (int i = 0; i < MAX_MISSILES; ++i) {
        if (mountedMissiles_[i]) mountedMissiles_[i]->Update();
        if (lockOnReticles_[i]) lockOnReticles_[i]->Update();
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
        for (int i = 0; i < MAX_MISSILES; ++i) {
            if (mountedMissiles_[i]) {
                
                if (currentWeapon_ == WeaponType::MISSILE && missileReloadTimer_ <= 0.0f) {
                    mountedMissiles_[i]->Draw();
                }
            }
        }
    }
    
    for (int i = 0; i < MAX_MISSILES; ++i) {
        if (lockOnReticles_[i]) lockOnReticles_[i]->Draw();
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
        frontColor.w = 1.0f; 
        frontReticle_->GetModel()->SetColor(frontColor);
    }
}

Vector3 Player::GetReticleWorldPosition() const {
    if (!reticle_) return {0,0,0};
    const Matrix4x4& mat = reticle_->GetmatWorld();
    return { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
}

// プレイヤーの移動とカメラ追従処理
void Player::Move(float gameSpeed) {
    if (!input_) return;

    
    if (lastQPressTime_ < 1000.0f) lastQPressTime_ += gameSpeed;
    if (lastEPressTime_ < 1000.0f) lastEPressTime_ += gameSpeed;

    
    if (!isRolling_) {
        if (input_->TriggerKey(DIK_Q)) {
            if (lastQPressTime_ <= doubleTapThreshold_) {
                isRolling_ = true;
                rollTimer_ = 0.0f;
                rollDirection_ = -1.0f; 
                isDodgeTriggered_ = true;
                lastQPressTime_ = 1000.0f;
            } else {
                lastQPressTime_ = 0.0f;
            }
        } else if (input_->TriggerKey(DIK_E)) {
            if (lastEPressTime_ <= doubleTapThreshold_) {
                isRolling_ = true;
                rollTimer_ = 0.0f;
                rollDirection_ = 1.0f; 
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

    
    isBoosting_ = input_->PushKey(DIK_LSHIFT);
    bool isBraking = input_->PushKey(DIK_LCONTROL);
    
    float targetZ = 20.0f; 
    if (isBoosting_) {
        targetZ = 35.0f; 
    } else if (isBraking) {
        targetZ = 5.0f;  
    }
    logicalPosition_.z += (targetZ - logicalPosition_.z) * 0.1f * gameSpeed;

    
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

    
    float screenPitch = currentPitch_ * 2.0f;
    float screenYaw   = currentYaw_ * 3.5f;

    
    
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

// プレイヤーの攻撃（弾・ミサイル発射）処理
void Player::Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets, std::list<std::unique_ptr<PlayerMissile>>& missiles, const std::list<std::unique_ptr<Enemy>>& enemies, Object3d* parentCamera, float gameSpeed) {
    if (!input_) return;

    
    if (input_->TriggerKey(DIK_R)) {
        currentWeapon_ = (currentWeapon_ == WeaponType::NORMAL) ? WeaponType::MISSILE : WeaponType::NORMAL;
    }

    
    if (missileReloadTimer_ > 0.0f) {
        missileReloadTimer_ -= gameSpeed;
    }

    if (attackTimer_ > 0.0f) {
        attackTimer_ -= gameSpeed;
    }

    
    if (currentWeapon_ == WeaponType::MISSILE) {
        lockOnAnimTimer_ += 0.1f * gameSpeed; 
        if (lockOnDelayTimer_ > 0.0f) {
            lockOnDelayTimer_ -= gameSpeed;
        }

        if (input_->PushKey(DIK_SPACE) && missileReloadTimer_ <= 0.0f) {
            const Matrix4x4& mat = object_->GetmatWorld();
            Vector3 playerPos = { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
            
            
            for (const auto& enemy : enemies) {
                if (enemy->IsDead()) continue;
                
                
                auto it = std::find_if(multiLockedEnemies_.begin(), multiLockedEnemies_.end(),
                    [&](const LockOnTarget& target) { return target.enemy == enemy.get(); });
                if (it != multiLockedEnemies_.end()) {
                    continue;
                }

                
                if (multiLockedEnemies_.size() >= MAX_MISSILES) {
                    break;
                }

                
                Vector3 ePos = enemy->GetColliderCenter();
                float distSq = (ePos.x - playerPos.x) * (ePos.x - playerPos.x) + 
                               (ePos.y - playerPos.y) * (ePos.y - playerPos.y) + 
                               (ePos.z - playerPos.z) * (ePos.z - playerPos.z);
                
                if (distSq < 1500.0f * 1500.0f && ePos.z > playerPos.z) { 
                    if (lockOnDelayTimer_ <= 0.0f) {
                        multiLockedEnemies_.push_back({enemy.get(), 0.0f});
                        lockOnDelayTimer_ = 60.0f; 
                        break; 
                    }
                }
            }
        }

        
        for (int i = 0; i < MAX_MISSILES; ++i) {
            if (i < multiLockedEnemies_.size() && !multiLockedEnemies_[i].enemy->IsDead()) {
                multiLockedEnemies_[i].lockedTime += gameSpeed;
                float lockTime = multiLockedEnemies_[i].lockedTime;
                bool isLockCompleted = lockTime >= 20.0f; 

                
                Vector3 ePos = multiLockedEnemies_[i].enemy->GetColliderCenter();
                const Matrix4x4& mat = object_->GetmatWorld();
                Vector3 pPos = { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
                float dz = ePos.z - pPos.z;
                float baseScale = 2.0f + (std::max)(0.0f, dz) * 0.02f;
                float scale = baseScale + (baseScale * 0.15f) * std::sinf(lockOnAnimTimer_ * 2.0f);
                lockOnReticles_[i]->SetScale({scale, scale, scale});
                lockOnReticles_[i]->SetTranslate(multiLockedEnemies_[i].enemy->GetColliderCenter());
                
                
                if (!isLockCompleted) {
                    float rotationAngle = (lockTime / 20.0f) * 6.2831853f; 
                    lockOnReticles_[i]->SetRotation({0.0f, 0.0f, rotationAngle});
                } else {
                    lockOnReticles_[i]->SetRotation({0.0f, 0.0f, 0.0f}); 
                }
            } else {
                lockOnReticles_[i]->SetScale({0.001f, 0.001f, 0.001f});
            }
        }

        
        if (!input_->PushKey(DIK_SPACE) && !multiLockedEnemies_.empty() && missileReloadTimer_ <= 0.0f) {
            for (size_t i = 0; i < multiLockedEnemies_.size(); ++i) {
                if (multiLockedEnemies_[i].enemy->IsDead()) continue;
                
                const Matrix4x4& mountedMat = mountedMissiles_[i]->GetmatWorld();
                Vector3 spawnPos = { mountedMat.m[3][0], mountedMat.m[3][1], mountedMat.m[3][2] };
                Vector3 spawnRot = object_->GetRotation(); 
                
                auto newMissile = std::make_unique<PlayerMissile>();
                newMissile->Initialize(object3dCommon_, spawnPos, spawnRot, {0,0,0}, parentCamera, multiLockedEnemies_[i].enemy);
                missiles.push_back(std::move(newMissile));

                
                lockOnReticles_[i]->SetScale({0.001f, 0.001f, 0.001f});
            }
            
            multiLockedEnemies_.clear();
            missileReloadTimer_ = missileReloadTime_;
            attackTimer_ = attackInterval_;
        }
        
        
        multiLockedEnemies_.erase(
            std::remove_if(multiLockedEnemies_.begin(), multiLockedEnemies_.end(),
                [](const LockOnTarget& t) { return t.enemy->IsDead(); }),
            multiLockedEnemies_.end()
        );
    } 
    
    else if (attackTimer_ <= 0.0f && (input_->PushKey(DIK_SPACE))) {
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

        if (isDoubleShot_) {
            // 2連装: プレイヤーの機体の姿勢（右方向ベクトル）に合わせて左右から発射
            Vector3 rightDir = { mat.m[0][0], mat.m[0][1], mat.m[0][2] };
            float offsetAmount = 2.0f;
            Vector3 rightOffset = { playerPos.x + rightDir.x * offsetAmount, playerPos.y + rightDir.y * offsetAmount, playerPos.z + rightDir.z * offsetAmount };
            Vector3 leftOffset = { playerPos.x - rightDir.x * offsetAmount, playerPos.y - rightDir.y * offsetAmount, playerPos.z - rightDir.z * offsetAmount };

            std::unique_ptr<PlayerBullet> rightBullet = std::make_unique<PlayerBullet>();
            rightBullet->Initialize(object3dCommon_, rightOffset, velocity, parentCamera, isLockOn_ ? lockOnTargetEnemy_ : nullptr);
            bullets.push_back(std::move(rightBullet));

            std::unique_ptr<PlayerBullet> leftBullet = std::make_unique<PlayerBullet>();
            leftBullet->Initialize(object3dCommon_, leftOffset, velocity, parentCamera, isLockOn_ ? lockOnTargetEnemy_ : nullptr);
            bullets.push_back(std::move(leftBullet));
        } else {
            // 通常の1発発射
            std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerBullet>();
            newBullet->Initialize(object3dCommon_, playerPos, velocity, parentCamera, isLockOn_ ? lockOnTargetEnemy_ : nullptr);
            bullets.push_back(std::move(newBullet));
        }
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

// プレイヤー関連のUI描画処理
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