#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>

class PlayerBullet {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, Object3d* parent);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    void Update3DObjectOnly() { if (object_) object_->Update(); }

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    // デッドフラグ
    bool IsDead() const { return isDead_; }

private:
    std::unique_ptr<Object3d> object_ = nullptr;
    Vector3 velocity_ = { 0.0f, 0.0f, 1.5f }; // 速度ベクトル
    bool isDead_ = false;
    int deathTimer_ = 120; // 寿命（フレーム）
};
