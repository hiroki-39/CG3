#include "PlayerBullet.h"

void PlayerBullet::Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, Object3d* parent) {
    velocity_ = velocity;
    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("cube.obj");
    object_->SetTranslate(position);
    object_->SetScale({ 1.0f, 1.0f, 1.0f });
    object_->SetScale({ 1.0f, 1.0f, 1.0f });
    // 弾はワールド座標系で飛ばすため、親(カメラ)は設定しない
    // if (parent) {
    //     object_->SetParent(parent);
    // }

    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_sphere.obj"); // 弾の当たり判定と同じ球
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
    colliderObject_->SetTranslate(position);
    colliderObject_->SetScale({ 1.0f, 1.0f, 1.0f }); // 半径1.0
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
