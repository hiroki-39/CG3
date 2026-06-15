#include "Enemy.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const std::string& typeName, uint32_t skyboxTexIndex) {
    position_ = pos;
    typeName_ = typeName;
    isDead_ = false;

    // タイプごとの設定
    std::string modelName = "cube.obj";
    if (typeName_ == "Asteroid") {
        modelName = "monsterBall.obj"; // とりあえずあるモデルを流用
        hp_ = 1; // 元5
        radius_ = 3.0f;
    } else if (typeName_ == "Fighter") {
        modelName = "suzanne.obj"; // 猿のモデルをFighterの代わりにする
        hp_ = 1; // 元2
        radius_ = 2.0f;
    } else {
        modelName = "cube.obj";
        hp_ = 1;
        radius_ = 1.0f;
    }

    // モデルがロードされているか確認してロード
    ModelManager::GetInstance()->LoadModel(modelName);

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName);
    object_->SetTranslate(position_);
    object_->SetScale({ radius_, radius_, radius_ });
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);
    
    // 黒落ちを回避
    object_->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
}

void Enemy::Update() {
    if (isDead_) return;

    // 簡易的な移動や回転
    if (typeName_ == "Asteroid") {
        Vector3 rot = object_->GetRotation();
        rot.x += 0.01f;
        rot.y += 0.02f;
        object_->SetRotation(rot);
    } else if (typeName_ == "Fighter") {
        // 必要に応じて移動処理を追記
        // position_.z -= 0.1f;
    }

    object_->SetTranslate(position_);
    object_->Update();
}

void Enemy::Draw() {
    if (!isDead_ && object_) {
        object_->Draw();
    }
}

void Enemy::OnCollision() {
    hp_--;
    if (hp_ <= 0) {
        isDead_ = true;
    }
}
