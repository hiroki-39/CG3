#include "Enemy.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const std::string& typeName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo) {
    position_ = pos;
    typeName_ = typeName;
    isDead_ = false;
    collider_ = colliderInfo;

    // タイプごとの設定
    std::string modelName = "cube.obj";
    if (typeName_ == "Asteroid") {
        modelName = "monsterBall.obj"; // とりあえずあるモデルを流用
        hp_ = 1; // 元5
    } else if (typeName_ == "Fighter") {
        modelName = "suzanne.obj"; // 猿のモデルをFighterの代わりにする
        hp_ = 1; // 元2
    } else {
        modelName = "cube.obj";
        hp_ = 1;
    }

    // モデルがロードされているか確認してロード
    ModelManager::GetInstance()->LoadModel(modelName);

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName);
    object_->SetTranslate(position_);
    
    // スケールをコライダーから推測して設定
    float scaleVal = (collider_.type == "SPHERE") ? collider_.radius : collider_.size.x;
    if (scaleVal <= 0.0f) scaleVal = 1.0f;
    object_->SetScale({ scaleVal, scaleVal, scaleVal });
    
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);

    // デバッグ用コライダーオブジェクトの初期化
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    if (collider_.type == "SPHERE") {
        colliderObject_->SetModel("monsterBall.obj"); // 球の代用
        colliderObject_->SetScale({collider_.radius, collider_.radius, collider_.radius});
    } else {
        colliderObject_->SetModel("cube.obj");
        colliderObject_->SetScale({collider_.size.x * 0.5f, collider_.size.y * 0.5f, collider_.size.z * 0.5f});
    }
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

    // コライダーオブジェクトも追従させる
    if (colliderObject_) {
        // オフセットを足した位置
        Vector3 colliderPos = {
            position_.x + collider_.center.x,
            position_.y + collider_.center.y,
            position_.z + collider_.center.z
        };
        colliderObject_->SetTranslate(colliderPos);
        // OBBの場合は回転も同期させる（今回はEnemy自身が回転する場合は同期）
        colliderObject_->SetRotation(object_->GetRotation());
        colliderObject_->Update();
    }
}

void Enemy::Draw() {
    if (!isDead_ && object_) {
        object_->Draw();
    }
}

void Enemy::DrawCollider() {
    if (!isDead_ && colliderObject_) {
        colliderObject_->Draw();
    }
}

void Enemy::OnCollision() {
    hp_--;
    if (hp_ <= 0) {
        isDead_ = true;
    }
}
