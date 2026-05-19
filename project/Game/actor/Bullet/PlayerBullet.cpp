#include "PlayerBullet.h"

void PlayerBullet::Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity) {
    velocity_ = velocity;
    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("monsterBall.obj"); // 既存のモデルを使用
    object_->SetTranslate(position);
    object_->SetScale({ 0.5f, 0.5f, 0.5f }); // 弾なので小さく
}

void PlayerBullet::Update() {
    // 速度ベクトルに従って移動
    Vector3 pos = object_->GetTranslate();
    pos.x += velocity_.x;
    pos.y += velocity_.y;
    pos.z += velocity_.z;
    object_->SetTranslate(pos);

    // 寿命
    if (--deathTimer_ <= 0) {
        isDead_ = true;
    }

    object_->Update();
}

void PlayerBullet::Draw() {
    if (object_) {
        object_->Draw();
    }
}
