#include "EnhanceRing.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "Game/Actor/Player/Player.h"
#include <cmath>

void EnhanceRing::Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const Vector3& rotation, const std::string& fileName, RingType type, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo) {
    position_ = pos;
    baseScale_ = scale;
    rotation_ = rotation;
    type_ = type;
    collider_ = colliderInfo;

    // Blenderからの設定値にモデルのスケールを乗算して実際のワールドサイズにする
    collider_.center.x *= scale.x;
    collider_.center.y *= scale.y;
    collider_.center.z *= scale.z;
    collider_.size.x *= scale.x;
    collider_.size.y *= scale.y;
    collider_.size.z *= scale.z;
    // スケールの中の最大のものを半径とする
    radius_ = scale.x > scale.y ? (scale.x > scale.z ? scale.x : scale.z) : (scale.y > scale.z ? scale.y : scale.z);
    collider_.radius *= radius_;

    // Blender側でコライダー設定が省略された場合のデフォルト処理
    if (collider_.type.empty()) {
        collider_.type = "BOX";
        collider_.center = {0.0f, 0.0f, 0.0f};
        collider_.size = scale; // スケールをそのままBOXのサイズ(半辺長)として扱う
    }

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetTranslate(position_);
    object_->SetScale(baseScale_);
    object_->SetRotation(rotation_);

    // タイプに応じて読み込むモデルを分ける（共有されると全て同じ色になってしまうため）
    std::string modelName = "Ring.obj";
    if (type_ == RingType::HEAL) {
        modelName = "Heal.obj";
    }

    ModelManager::GetInstance()->LoadModel(modelName);
    object_->SetModel(modelName);

    // タイプに応じて色を変える
    if (object_->GetModel()) {
        if (type_ == RingType::POWER_UP) {
            object_->GetModel()->SetColor({1.0f, 0.2f, 0.2f, 1.0f}); // 赤系
        } else if (type_ == RingType::HEAL) {
            object_->GetModel()->SetColor({0.2f, 1.0f, 0.2f, 1.0f}); // 緑系
        }
    }

    object_->SetEnvironmentTextureIndex(skyboxTexIndex);
    
    // デバッグ用コライダー表示の初期化
    if (collider_.type == "SPHERE" || (collider_.type == "BOX" || collider_.type == "OBB" || collider_.type == "AABB")) {
        colliderObject_ = std::make_unique<Object3d>();
        colliderObject_->Initialize(object3dCommon);
        
        if (collider_.type == "SPHERE") {
            colliderObject_->SetModel("collider_sphere.obj");
            colliderObject_->SetScale({ collider_.radius, collider_.radius, collider_.radius });
        } else {
            colliderObject_->SetModel("cube.obj"); // 境界線のみの箱モデルを想定
            colliderObject_->SetScale({ collider_.size.x, collider_.size.y, collider_.size.z });
        }
        
        if (colliderObject_->GetModel()) {
            colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 1.0f, 1.0f }); // 水色
        }
    }

    Update();
}

void EnhanceRing::Update() {
    if (isShrinking_) {
        shrinkScale_ -= 0.0333f; // 約0.5秒(30フレーム)で消滅するように調整
        rotation_.z += 0.5f;     // 回転
        
        if (targetPlayer_) {
            // プレイヤーの位置に追従する(機体に取り込まれる演出)
            const Matrix4x4& wMat = targetPlayer_->GetObject3d()->GetmatWorld();
            position_ = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };
        }

        if (shrinkScale_ <= 0.0f) {
            shrinkScale_ = 0.0f;
            isDead_ = true;
        }
        Vector3 newScale = { baseScale_.x * shrinkScale_, baseScale_.y * shrinkScale_, baseScale_.z * shrinkScale_ };
        object_->SetScale(newScale);
    }
    
    if (object_) {
        object_->SetTranslate(position_);
        object_->SetRotation(rotation_);
        object_->Update();
    }
    
    if (colliderObject_) {
        Vector3 colliderPos = {
            position_.x + collider_.center.x,
            position_.y + collider_.center.y,
            position_.z + collider_.center.z
        };
        colliderObject_->SetTranslate(colliderPos);
        colliderObject_->SetRotation(rotation_);
        colliderObject_->Update();
    }
}

void EnhanceRing::Draw() {
    if (object_ && !isDead_) {
        object_->Draw();
    }
}

void EnhanceRing::DrawCollider() {
    if (colliderObject_ && !isDead_) {
        colliderObject_->Draw();
    }
}

void EnhanceRing::StartShrink(Player* player) {
    isShrinking_ = true;
    targetPlayer_ = player;
}

bool EnhanceRing::CheckCollision(Player* player) {
    if (!player || isDead_ || isShrinking_) return false;

    // プレイヤーをSphereとして近似
    const Matrix4x4& wMat = player->GetColliderObject() ? player->GetColliderObject()->GetmatWorld() : player->GetObject3d()->GetmatWorld();
    Vector3 pWorldPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };
    Vector3 pSize = player->GetColliderSize();
    float pRadius = pSize.x > pSize.y ? (pSize.x > pSize.z ? pSize.x : pSize.z) : (pSize.y > pSize.z ? pSize.y : pSize.z);
    Sphere playerSphere = { pWorldPos, pRadius };

    Vector3 centerPos = {
        position_.x + collider_.center.x,
        position_.y + collider_.center.y,
        position_.z + collider_.center.z
    };

    if (collider_.type == "SPHERE") {
        Sphere mySphere = { centerPos, collider_.radius };
        return CollisionMath::IsCollision(playerSphere, mySphere);
    } 
    else if ((collider_.type == "BOX" || collider_.type == "OBB" || collider_.type == "AABB")) {
        Vector3 colSize = collider_.size;
        // 回転がある場合はOBB、ない場合はAABBとして扱う
        if (rotation_.x == 0.0f && rotation_.y == 0.0f && rotation_.z == 0.0f) {
            AABB aabb = {
                { centerPos.x - colSize.x, centerPos.y - colSize.y, centerPos.z - colSize.z },
                { centerPos.x + colSize.x, centerPos.y + colSize.y, centerPos.z + colSize.z }
            };
            return CollisionMath::IsCollision(playerSphere, aabb);
        } else {
            // OBBを生成
            Matrix4x4 rotMat = Matrix4x4::RotateX(rotation_.x) * Matrix4x4::RotateY(rotation_.y) * Matrix4x4::RotateZ(rotation_.z);
            OBB obb = CollisionMath::CreateOBB(centerPos, colSize, rotMat);
            return CollisionMath::IsCollision(playerSphere, obb);
        }
    }

    return false;
}
