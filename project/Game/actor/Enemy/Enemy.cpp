#include "Enemy.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const std::string& typeName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo) {
    position_ = pos;
    typeName_ = typeName;
    isDead_ = false;
    collider_ = colliderInfo;

    // タイプごとの設定
    std::string modelName = "cube.obj";
    if (typeName_ == "Asteroid") {
        modelName = "monsterBall.obj"; // とりあえずあるモデルを流用
        hp_ = 2; 
    } else if (typeName_ == "Fighter") {
        modelName = "suzanne.obj"; // 猿のモデルをFighterの代わりにする
        hp_ = 2; 
    } else {
        modelName = "cube.obj";
        hp_ = 2;
    }

    // モデルがロードされているか確認してロード
    ModelManager::GetInstance()->LoadModel(modelName);

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName);
    object_->SetTranslate(position_);
    
    // 敵本体のスケールには引数で受け取ったBlender上のscaleをそのまま設定する
    object_->SetScale(scale);
    
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);

    // デバッグ用コライダーオブジェクトの初期化
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    if (collider_.type == "SPHERE") {
        ModelManager::GetInstance()->LoadModel("collider_sphere.obj");
        colliderObject_->SetModel("collider_sphere.obj"); // 球の代用
        colliderObject_->SetScale({collider_.radius, collider_.radius, collider_.radius});
    } else {
        ModelManager::GetInstance()->LoadModel("collider_cube.obj");
        colliderObject_->SetModel("collider_cube.obj");
        colliderObject_->SetScale({collider_.size.x, collider_.size.y, collider_.size.z});
    }
    
    // コライダー専用のモデルなので、色を赤にしても他のモデルに影響しない
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    // 初期位置にコライダーオブジェクトを追従させる
    object_->Update();
    if (colliderObject_) {
        Vector3 colliderPos = {
            position_.x + collider_.center.x,
            position_.y + collider_.center.y,
            position_.z + collider_.center.z
        };
        colliderObject_->SetTranslate(colliderPos);
        colliderObject_->SetRotation(object_->GetRotation());
        colliderObject_->Update();
    }
}

void Enemy::SetMovePath(std::unique_ptr<Rail> path) {
    movePath_ = std::move(path);
    pathProgress_ = 0.0f;
}

void Enemy::Update() {
    if (isDead_) return;

    // 簡易的な移動や回転
    if (movePath_ && movePath_->IsValid()) {
        // パスが設定されている場合、パスに沿って移動する
        float speed = movePath_->GetSpeed(pathProgress_);
        if (speed <= 0.0f) speed = 20.0f; // デフォルトスピード
        
        // フレームレートを60FPSと仮定し、レールの長さとスピードから進行度を加算
        float length = movePath_->GetTotalLength();
        if (length > 0.0f) {
            pathProgress_ += (speed / length) * (1.0f / 60.0f);
        }
        
        if (pathProgress_ > 1.0f) {
            pathProgress_ = 1.0f; // 終点で止まる、またはループさせるか（今回は終点停止）
        }

        position_ = movePath_->GetPosition(pathProgress_);
        
        // 進行方向に向ける
        Vector3 forward = movePath_->GetForward(pathProgress_);
        float yaw = std::atan2(forward.x, forward.z);
        float pitch = std::asin(-forward.y);
        object_->SetRotation(Vector3(pitch, yaw, 0.0f));
        
    } else {
        if (typeName_ == "Asteroid") {
            Vector3 rot = object_->GetRotation();
            rot.x += 0.01f;
            rot.y += 0.02f;
            object_->SetRotation(rot);
        } else if (typeName_ == "Fighter") {
            // 必要に応じて移動処理を追記
        }
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

bool Enemy::CheckCollision(const Sphere& bulletSphere) const {
    if (isDead_) return false;

    Vector3 colCenter = { position_.x + collider_.center.x, position_.y + collider_.center.y, position_.z + collider_.center.z };

    if (collider_.type == "SPHERE") {
        Sphere enemySphere = { colCenter, collider_.radius };
        return CollisionMath::IsCollision(bulletSphere, enemySphere);
    } else if (collider_.type == "OBB") {
        Matrix4x4 identity = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
        OBB enemyOBB = CollisionMath::CreateOBB(colCenter, collider_.size, identity);
        return CollisionMath::IsCollision(bulletSphere, enemyOBB);
    } else {
        // AABB
        AABB enemyAABB = {
            { colCenter.x - collider_.size.x * 0.5f, colCenter.y - collider_.size.y * 0.5f, colCenter.z - collider_.size.z * 0.5f },
            { colCenter.x + collider_.size.x * 0.5f, colCenter.y + collider_.size.y * 0.5f, colCenter.z + collider_.size.z * 0.5f }
        };
        return CollisionMath::IsCollision(bulletSphere, enemyAABB);
    }
}

bool Enemy::CheckRaycast(const Ray& ray, float* outDist) const {
    if (isDead_) return false;

    Vector3 colCenter = { position_.x + collider_.center.x, position_.y + collider_.center.y, position_.z + collider_.center.z };

    if (collider_.type == "SPHERE") {
        Sphere enemySphere = { colCenter, collider_.radius };
        return CollisionMath::Raycast(ray, enemySphere, outDist);
    } else if (collider_.type == "OBB") {
        Matrix4x4 identity = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
        OBB enemyOBB = CollisionMath::CreateOBB(colCenter, collider_.size, identity);
        return CollisionMath::Raycast(ray, enemyOBB, outDist);
    } else {
        // AABB
        AABB enemyAABB = {
            { colCenter.x - collider_.size.x * 0.5f, colCenter.y - collider_.size.y * 0.5f, colCenter.z - collider_.size.z * 0.5f },
            { colCenter.x + collider_.size.x * 0.5f, colCenter.y + collider_.size.y * 0.5f, colCenter.z + collider_.size.z * 0.5f }
        };
        return CollisionMath::Raycast(ray, enemyAABB, outDist);
    }
}
