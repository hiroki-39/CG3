#include "PlayerBullet.h"
#include "Game/Actor/Enemy/Enemy.h"
#include <cmath>

void PlayerBullet::Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, Object3d* parent, Enemy* targetEnemy) {
    velocity_ = velocity;
    targetEnemy_ = targetEnemy;
    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("cube.obj");
    object_->GetModel()->SetColor({ 0.5f, 1.0f, 0.0f, 1.0f }); // 弾を黄緑色にする
    object_->SetTranslate(position);
    object_->SetScale({ 4.0f, 4.0f, 4.0f }); // 見やすいようにモデルを大きくする
    // 弾はワールド座標系で飛ばすため、親(カメラ)は設定しない
    // if (parent) {
    //     object_->SetParent(parent);
    // }

    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_sphere.obj"); // 弾の当たり判定と同じ球
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
    colliderObject_->SetTranslate(position);
    colliderObject_->SetScale({ 4.0f, 4.0f, 4.0f }); // 当たり判定を大きくする
    previousPosition_ = position;
}

void PlayerBullet::Update() {
    previousPosition_ = object_->GetTranslate();
    Vector3 pos = previousPosition_;

    // ホーミング処理
    if (targetEnemy_ && !targetEnemy_->IsDead()) {
        Vector3 targetPos = targetEnemy_->GetColliderCenter();
        Vector3 toTarget = {
            targetPos.x - pos.x,
            targetPos.y - pos.y,
            targetPos.z - pos.z
        };

        // ターゲットへの方向を正規化
        float length = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        if (length > 0.0f) {
            toTarget.x /= length;
            toTarget.y /= length;
            toTarget.z /= length;
        }

        // 現在の速度ベクトルの長さ（スピード）を取得
        float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);

        // 現在の速度ベクトルを少しずつターゲット方向へ向ける（ホーミングの強さ：0.15f 程度）
        float homingStrength = 0.15f;
        velocity_.x += (toTarget.x * speed - velocity_.x) * homingStrength;
        velocity_.y += (toTarget.y * speed - velocity_.y) * homingStrength;
        velocity_.z += (toTarget.z * speed - velocity_.z) * homingStrength;

        // 再度長さをspeedに合わせる（速度が変わらないようにする）
        float newLength = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
        if (newLength > 0.0f) {
            velocity_.x = (velocity_.x / newLength) * speed;
            velocity_.y = (velocity_.y / newLength) * speed;
            velocity_.z = (velocity_.z / newLength) * speed;
        }
    }

    // 速度ベクトルに従って移動
    pos.x += velocity_.x;
    pos.y += velocity_.y;
    pos.z += velocity_.z;
    object_->SetTranslate(pos);

    // 寿命
    if (--deathTimer_ <= 0) {
        isDead_ = true;
    }

    object_->Update();
    if (colliderObject_) {
        colliderObject_->SetTranslate(pos);
        colliderObject_->Update();
    }
}

void PlayerBullet::Draw() {
    if (object_) {
        object_->Draw();
    }
}

void PlayerBullet::DrawCollider() {
    if (colliderObject_) {
        colliderObject_->Draw();
    }
}
