#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>
#include <string>

class Enemy {
public:
    void Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const std::string& typeName, uint32_t skyboxTexIndex);
    void Update();
    void Update3DObjectOnly() { if (object_) object_->Update(); }
    void Draw();
    void OnCollision(); // 弾が当たった時の処理

    // Getter / Setter
    const Vector3& GetPosition() const { return position_; }
    float GetRadius() const { return radius_; }
    bool IsDead() const { return isDead_; }

private:
    std::unique_ptr<Object3d> object_;
    Vector3 position_;
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    float radius_ = 2.0f; // 当たり判定の半径
    int hp_ = 3;
    bool isDead_ = false;
    std::string typeName_;
};
