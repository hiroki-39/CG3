#include "PlayerBullet.h"
#include "Game/Actor/Enemy/Enemy.h"
#include <cmath>

void PlayerBullet::Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, Object3d* parent, Enemy* targetEnemy) {
    velocity_ = velocity;
    targetEnemy_ = targetEnemy;
    
    // 進行方向への初期向きを計算
    float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    if (speed > 0.0001f) {
        rotation_.y = std::atan2(velocity_.x, velocity_.z);
        float xzLen = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
        rotation_.x = std::atan2(-velocity_.y, xzLen);
    }

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("beam.obj"); // 専用レーザービームモデル
    object_->SetSelectLightings(6); // ビーム発光モード（中心ハイライト + 外縁プラズマ）
    object_->SetEnableLighting(false); // 自発光
    object_->SetColor({ 0.1f, 0.9f, 1.0f, 1.0f }); // 鮮烈なプラズマシアン
    object_->SetTranslate(position);
    object_->SetRotation(rotation_);
    object_->SetScale({ 0.45f, 0.45f, 4.0f }); // シャープで高速感あふれる長細いビーム形状

    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_sphere_player.obj");
    colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f });
    colliderObject_->SetTranslate(position);
    colliderObject_->SetScale({ 2.5f, 2.5f, 2.5f });
    previousPosition_ = position;
}

void PlayerBullet::Update(float gameSpeed) {
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
        float homingStrength = 0.15f * gameSpeed;
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
    pos.x += velocity_.x * gameSpeed;
    pos.y += velocity_.y * gameSpeed;
    pos.z += velocity_.z * gameSpeed;
    object_->SetTranslate(pos);

    // 進行方向に向きを同期
    float curSpeed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    if (curSpeed > 0.0001f) {
        rotation_.y = std::atan2(velocity_.x, velocity_.z);
        float xzLen = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
        rotation_.x = std::atan2(-velocity_.y, xzLen);
    }
    object_->SetRotation(rotation_);

    // 寿命
    deathTimer_ -= gameSpeed;
    if (deathTimer_ <= 0.0f) {
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
