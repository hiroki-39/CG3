#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>
#include <string>
#include "KHEngine/Scene/LevelLoader.h"

class Enemy {
public:
    void Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const std::string& typeName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo);
    void Update();
    void Update3DObjectOnly() {
        if (object_) object_->Update();
        if (colliderObject_) colliderObject_->Update();
    }
    void Draw();
    void DrawCollider(); // デバッグ描画用
    void OnCollision(); // 弾が当たった時の処理

    // Getter / Setter
    const Vector3& GetPosition() const { return position_; }
    const LevelCollider& GetCollider() const { return collider_; }
    bool IsDead() const { return isDead_; }

private:
    std::unique_ptr<Object3d> object_;
    std::unique_ptr<Object3d> colliderObject_; // デバッグ描画用オブジェクト
    Vector3 position_;
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    LevelCollider collider_; // コライダー情報
    int hp_ = 3;
    bool isDead_ = false;
    std::string typeName_;
};
