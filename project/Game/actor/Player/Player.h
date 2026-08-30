#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Input/Input.h"
#include "Game/Actor/Bullet/PlayerBullet.h"
#include "Game/Actor/Bullet/PlayerMissile.h"
#include <memory>
#include <list>
#include <vector>
#include <array>

class Enemy;
class ParticleEmitter;

class Player {
public:
    enum class WeaponType {
        NORMAL,
        MISSILE
    };

    
    
    
    void Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex);

    void LoadSettings(const std::string& filepath);
    void SaveSettings(const std::string& filepath);
    void DrawUI();

    
    
    
    bool IsRolling() const { return isRolling_; }
    float GetRollTimer() const { return rollTimer_; }
    float GetRollMaxTime() const { return rollMaxTime_; }
    
    
    
    

    
    
    
    void Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, std::list<std::unique_ptr<PlayerMissile>>& missiles, const std::list<std::unique_ptr<Enemy>>& enemies, Object3d* parentCamera = nullptr, float gameSpeed = 1.0f);

    
    
    
    void Draw();

    void DrawCollider();
    
    Object3d* GetColliderObject() const { return colliderObject_.get(); }

    void Update3DObjectOnly() {
        if (object_) {
            object_->SetScale(playerScale_);
            object_->SetRotation(Vector3(
                baseRotation_.x + modelRotOffset_.x,
                baseRotation_.y + modelRotOffset_.y,
                baseRotation_.z + modelRotOffset_.z
            ));
            object_->SetTranslate(Vector3(
                logicalPosition_.x + modelPosOffset_.x, 
                logicalPosition_.y + modelPosOffset_.y, 
                logicalPosition_.z + modelPosOffset_.z
            ));
            object_->Update();
            
            if (colliderObject_) {
                const Matrix4x4& wMat = object_->GetmatWorld();
                colliderObject_->SetTranslate({ wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] });
                colliderObject_->Update();
            }
            for (int i = 0; i < 4; ++i) { 
                if (mountedMissiles_[i]) mountedMissiles_[i]->Update();
                if (lockOnReticles_[i]) lockOnReticles_[i]->Update();
            }
        }
        if (accessory_) accessory_->Update();
        if (reticle_) reticle_->Update();
        if (frontReticle_) frontReticle_->Update();
    }

    
    const Vector3& GetTranslate() const { return logicalPosition_; }
    const Vector3& GetPreviousTranslate() const { return prevLogicalPosition_; }
    const Vector3& GetColliderSize() const { return colliderSize_; }
    void SetTranslate(const Vector3& translate) { logicalPosition_ = translate; }
    void SetRotation(const Vector3& rotation) { baseRotation_ = rotation; }
    Object3d* GetObject3d() const { return object_.get(); }
    Object3d* GetReticle() const { return reticle_.get(); }
    Object3d* GetFrontReticle() const { return frontReticle_.get(); }
    
    
    void SetReticleColor(const Vector4& color);
    const Vector4& GetReticleColor() const { return reticleColor_; }
    Vector3 GetReticleWorldPosition() const;

    void SetLockOn(bool isLockOn, const Vector3& targetPos = { 0, 0, 0 }, Enemy* targetEnemy = nullptr) {
        isLockOn_ = isLockOn;
        lockOnTargetPos_ = targetPos;
        lockOnTargetEnemy_ = targetEnemy;
    }

    bool IsBoosting() const { return isBoosting_; }

    
    bool ConsumeDodgeTrigger() {
        if (isDodgeTriggered_) {
            isDodgeTriggered_ = false;
            return true;
        }
        return false;
    }

    void OnCollision();
    void Heal(int amount) { hp_ += amount; if (hp_ > maxHp_) hp_ = maxHp_; }
    void PowerUp() { isDoubleShot_ = true; }
    bool IsDead() const { return isDead_; }
    int GetHp() const { return hp_; }
    int GetMaxHp() const { return maxHp_; }

    
    void SetAssistTarget(Enemy* enemy) { assistTarget_ = enemy; }

    
    bool IsBanking() const;
    Vector3 GetLeftWingPosition() const;
    Vector3 GetRightWingPosition() const;

private:
    
    
    void Move(float gameSpeed);

    
    
    
    void Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets, std::list<std::unique_ptr<PlayerMissile>>& missiles, const std::list<std::unique_ptr<Enemy>>& enemies, Object3d* parentCamera, float gameSpeed);

private:
    std::unique_ptr<Object3d> object_ = nullptr;
    std::unique_ptr<Object3d> colliderObject_ = nullptr;
    std::unique_ptr<Object3d> reticle_ = nullptr; 
    std::unique_ptr<Object3d> frontReticle_ = nullptr; 
    std::unique_ptr<Object3d> accessory_ = nullptr; 
    Object3dCommon* object3dCommon_ = nullptr; 
    Input* input_ = nullptr;
    
    uint32_t skyboxTexIndex_ = 0; 

    Vector3 reticlePosition_ = { 0.0f, 0.0f, 40.0f }; 
    Vector4 reticleColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; 

    
    float speed_ = 0.3f;
    float reticleSpeed_ = 0.5f;
    float moveLimitX_ = 25.0f;     
    float moveLimitY_ = 12.0f;     
    float attackInterval_ = 15.0f;
    float rollMaxTime_ = 15.0f;
    float playerLimitX_ = 25.0f;   
    float playerLimitYMin_ = -5.0f;
    float playerLimitYMax_ = 12.0f;
    float followSpeed_ = 0.08f;
    float bulletSpeed_ = 3.0f;

    
    std::string modelName_ = "cube.obj";
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool reflection_ = false;
    Vector3 modelPosOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 modelRotOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 playerScale_ = { 0.5f, 0.5f, 0.5f };
    Vector3 colliderSize_ = { 4.0f, 4.0f, 4.0f };

    
    float currentPitch_ = 0.0f;
    float currentYaw_ = 0.0f;
    float currentBank_ = 0.0f;

    // プレイヤーの論理的な位置とベース回転
    Vector3 logicalPosition_ = { 0.0f, -0.0f, 20.0f }; 
    Vector3 prevLogicalPosition_ = { 0.0f, -0.0f, 20.0f }; 
    Vector3 baseRotation_ = { 0.0f, 0.0f, 0.0f };

    // 攻撃タイマー
    float attackTimer_ = 0.0f;

    
    Vector2 velocity_ = { 0.0f, 0.0f };

    
    bool isRolling_ = false;
    float rollTimer_ = 0.0f;
    float rollDirection_ = 0.0f; 
    
    
    float lastQPressTime_ = 0.0f;
    float lastEPressTime_ = 0.0f;
    const float doubleTapThreshold_ = 20.0f; 

    
    bool isLockOn_ = false;
    Vector3 lockOnTargetPos_ = { 0, 0, 0 };
    Enemy* lockOnTargetEnemy_ = nullptr;
    
    
    Enemy* assistTarget_ = nullptr;

    
    bool isBoosting_ = false;

    
    WeaponType currentWeapon_ = WeaponType::NORMAL;
    static const int MAX_MISSILES = 4;
    std::array<std::unique_ptr<Object3d>, MAX_MISSILES> mountedMissiles_;
    std::array<std::unique_ptr<Object3d>, MAX_MISSILES> lockOnReticles_;
    struct LockOnTarget {
        Enemy* enemy;
        float lockedTime;
    };
    std::vector<LockOnTarget> multiLockedEnemies_;
    float missileReloadTimer_ = 0.0f;
    const float missileReloadTime_ = 120.0f; 
    float lockOnAnimTimer_ = 0.0f; 
    float lockOnDelayTimer_ = 0.0f;

    
    bool isDodgeTriggered_ = false;

    
    int hp_ = 10000;
    int maxHp_ = 10000;
    float invincibilityTimer_ = 0.0f;
    bool isDead_ = false;
    bool isDoubleShot_ = false;
};

