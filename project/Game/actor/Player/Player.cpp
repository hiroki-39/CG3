#include "Player.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include <algorithm>

void Player::Initialize(Object3dCommon* object3dCommon) {
    object3dCommon_ = object3dCommon;
    input_ = EngineServices::GetInstance()->GetInput();

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("suzanne.obj"); // プレイヤーモデルとしてCubeを使用
    object_->SetTranslate({ 0.0f, 0.0f, 0.0f });
}

void Player::Update(std::list<std::unique_ptr<PlayerBullet>>& bullets) {
    Move();
    Attack(bullets);

    object_->Update();
}

void Player::Draw() {
    if (object_) {
        object_->Draw();
    }
}

void Player::Move() {
    if (!input_) return;

    Vector3 pos = object_->GetTranslate();

    // 移動入力
    if (input_->PushKey(DIK_W) || input_->PushKey(DIK_UP)) {
        pos.y += speed_;
    }
    if (input_->PushKey(DIK_S) || input_->PushKey(DIK_DOWN)) {
        pos.y -= speed_;
    }
    if (input_->PushKey(DIK_A) || input_->PushKey(DIK_LEFT)) {
        pos.x -= speed_;
    }
    if (input_->PushKey(DIK_D) || input_->PushKey(DIK_RIGHT)) {
        pos.x += speed_;
    }

    // 移動制限
    pos.x = std::clamp(pos.x, -kMoveLimitX, kMoveLimitX);
    pos.y = std::clamp(pos.y, -kMoveLimitY, kMoveLimitY);

    object_->SetTranslate(pos);
}

void Player::Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets) {
    if (!input_) return;

    // スペースキーまたはマウス左クリックで発射
    if (input_->TriggerKey(DIK_SPACE) || input_->TriggerMouseButton(0)) {
        // 弾を生成
        std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerBullet>();
        newBullet->Initialize(object3dCommon_, object_->GetTranslate());
        bullets.push_back(std::move(newBullet));
    }
}
