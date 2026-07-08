#include "EnemyBullet.h"
#include "Game/Actor/Player/Player.h"
#include <cmath>

void EnemyBullet::Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, bool isHoming, Player* targetPlayer) {
    velocity_ = velocity;
    isHoming_ = isHoming;
    targetPlayer_ = targetPlayer;
    
    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("cube.obj");
    object_->GetModel()->SetColor({ 1.0f, 0.5f, 0.0f, 1.0f }); // 敵の弾をオレンジ色にする
    object_->SetTranslate(position);
    object_->SetScale({ 4.0f, 4.0f, 4.0f }); 

    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_sphere.obj"); 
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); 
    colliderObject_->SetTranslate(position);
    colliderObject_->SetScale({ 4.0f, 4.0f, 4.0f }); 
    previousPosition_ = position;
}

void EnemyBullet::Update() {
    previousPosition_ = object_->GetTranslate();
    Vector3 pos = previousPosition_;

    // ホーミング処理
    if (isHoming_ && targetPlayer_) {
        Vector3 targetPos = targetPlayer_->GetTranslate();
        Vector3 toTarget = {
            targetPos.x - pos.x,
            targetPos.y - pos.y,
            targetPos.z - pos.z
        };

        float length = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        if (length > 0.0f) {
            toTarget.x /= length;
            toTarget.y /= length;
            toTarget.z /= length;
        }

        float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);

        float homingStrength = 0.08f; // プレイヤーのホーミングより少し弱めにする
        velocity_.x += (toTarget.x * speed - velocity_.x) * homingStrength;
        velocity_.y += (toTarget.y * speed - velocity_.y) * homingStrength;
        velocity_.z += (toTarget.z * speed - velocity_.z) * homingStrength;

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

void EnemyBullet::Draw() {
    if (object_) {
        object_->Draw();
    }
}

void EnemyBullet::DrawCollider() {
    if (colliderObject_) {
        colliderObject_->Draw();
    }
}
