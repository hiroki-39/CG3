#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>

class Player;

class EnemyBullet {
public:
    void Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, bool isHoming = false, Player* targetPlayer = nullptr);

    void Update();
    void Update3DObjectOnly() { if (object_) object_->Update(); }
    void Draw();
    void DrawCollider();

    bool IsDead() const { return isDead_; }
    const Vector3& GetPosition() const { return object_ ? object_->GetTranslate() : velocity_; }
    const Vector3& GetPreviousPosition() const { return previousPosition_; }
    void OnCollision() { isDead_ = true; }

private:
    std::unique_ptr<Object3d> object_ = nullptr;
    std::unique_ptr<Object3d> colliderObject_ = nullptr;
    Vector3 velocity_ = { 0.0f, 0.0f, -1.5f };
    Vector3 previousPosition_ = { 0.0f, 0.0f, 0.0f };
    bool isDead_ = false;
    int deathTimer_ = 180;
    
    bool isHoming_ = false;
    Player* targetPlayer_ = nullptr;
};
