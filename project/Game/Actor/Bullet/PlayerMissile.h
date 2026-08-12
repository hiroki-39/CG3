#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>

class Enemy;

class PlayerMissile {
public:
    enum class Phase {
        DROP,    // 発射直後、自機から落下・分離するフェーズ
        FLIGHT   // スラスターを点火し、ターゲットに向かって飛行するフェーズ
    };

    void Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& rotation, const Vector3& velocity, Object3d* parent, Enemy* targetEnemy = nullptr);
    void Update(float gameSpeed = 1.0f);
    void Update3DObjectOnly() { if (object_) object_->Update(); }
    void Draw();
    void DrawCollider();

    bool IsDead() const { return isDead_; }
    const Vector3& GetPosition() const { return object_ ? object_->GetTranslate() : position_; }
    const Vector3& GetPreviousPosition() const { return previousPosition_; }
    
    // 衝突時の処理
    void OnCollision() { isDead_ = true; }

    Phase GetCurrentPhase() const { return currentPhase_; }

private:
    std::unique_ptr<Object3d> object_ = nullptr;
    std::unique_ptr<Object3d> colliderObject_ = nullptr;
    
    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    Vector3 previousPosition_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };

    bool isDead_ = false;
    float deathTimer_ = 300.0f; // 寿命（フレーム）
    
    Enemy* targetEnemy_ = nullptr;

    Phase currentPhase_ = Phase::DROP;
    float phaseTimer_ = 0.0f;
    const float dropDuration_ = 12.0f; // 落下フェーズの長さ (フレーム数。12.0f = 0.2秒)
};
