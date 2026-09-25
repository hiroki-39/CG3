#include "EnemyBullet.h"
#include "Game/Actor/Player/Player.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include <cmath>

void EnemyBullet::Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, bool isHoming, Player* targetPlayer) {
    velocity_ = velocity;
    isHoming_ = isHoming;
    targetPlayer_ = targetPlayer;
    
    // 進行方向への初期向きを計算
    float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    if (speed > 0.0001f) {
        rotation_.y = std::atan2(velocity_.x, velocity_.z);
        float xzLen = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
        rotation_.x = std::atan2(-velocity_.y, xzLen);
    }

    // 敵弾専用モデルをロード
    ModelManager::GetInstance()->LoadModel("beam_enemy.obj");

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("beam_enemy.obj"); // 敵弾専用ビームモデルを使用
    object_->SetSelectLightings(6); // ビーム発光モード（中心ハイライト + 外縁プラズマ）
    object_->SetEnableLighting(false); // 自発光
    object_->SetColor({ 1.0f, 0.08f, 0.05f, 1.0f }); // 鮮烈な発光レッドビーム
    object_->SetTranslate(position);
    object_->SetRotation(rotation_);
    object_->SetScale({ 0.6f, 0.6f, 4.2f }); // プレイヤー弾より少し太めで視認性の高いシャープなレーザー形状

    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_sphere_enemy.obj"); 
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); 
    colliderObject_->SetTranslate(position);
    colliderObject_->SetScale({ 3.0f, 3.0f, 3.0f }); 
    previousPosition_ = position;
}

void EnemyBullet::Update(float gameSpeed) {
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

        float homingStrength = 0.08f * gameSpeed; // プレイヤーのホーミングより少し弱めにする
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
