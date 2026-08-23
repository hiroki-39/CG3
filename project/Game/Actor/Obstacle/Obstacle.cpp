#include "Obstacle.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"

void Obstacle::Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const Vector3& rot, const std::string& fileName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo, bool isDestructible) {
    position_ = pos;
    collider_ = colliderInfo;
    isDestructible_ = isDestructible; // パラメータからフラグをセット

    // Blender側でコライダー設定が省略された場合のデフォルト処理
    if (collider_.type.empty()) {
        collider_.type = "BOX";
        collider_.center = {0.0f, 0.0f, 0.0f};
        collider_.size = scale; // スケールをそのままBOXのサイズ(半辺長)として扱う
    }

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetTranslate(position_);
    
    rotation_ = rot;
    baseScale_ = scale;
    object_->SetScale(scale);
    object_->SetRotation(rotation_);

    std::string modelName = fileName;
    if (modelName.find("ColliderOnly") != std::string::npos || modelName.find("Invisible") != std::string::npos) {
        // 見えない壁（当たり判定専用）の場合はモデルを読み込まない
        isVisible_ = false;
    } else {
        if (modelName.empty()) {
            modelName = "cube.obj"; // デフォルト
        } else if (modelName.find(".obj") == std::string::npos) {
            modelName += ".obj";
        }
        
        ModelManager::GetInstance()->LoadModel(modelName);
        if (ModelManager::GetInstance()->FindModel(modelName)) {
            object_->SetModel(modelName);
            isVisible_ = true;
        }
    }
    
    if (isVisible_) {
        object_->SetEnvironmentTextureIndex(skyboxTexIndex);
    }

    // デバッグ用コライダー表示の初期化
    if (collider_.type == "SPHERE" || collider_.type == "BOX") {
        colliderObject_ = std::make_unique<Object3d>();
        colliderObject_->Initialize(object3dCommon);
        
        if (collider_.type == "SPHERE") {
            colliderObject_->SetModel("collider_sphere.obj");
            colliderObject_->SetScale({ collider_.radius, collider_.radius, collider_.radius });
        } else if (collider_.type == "BOX") {
            colliderObject_->SetModel("collider_box.obj"); // 境界線のみの箱モデルを想定
            colliderObject_->SetScale({ collider_.size.x, collider_.size.y, collider_.size.z });
        }
        
        if (colliderObject_->GetModel()) {
            colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 1.0f, 1.0f }); // 水色
        }
    }
    
    Update();
}

void Obstacle::Update() {
    if (isShrinking_) {
        shrinkScale_ -= 0.1f;
        if (shrinkScale_ <= 0.0f) {
            shrinkScale_ = 0.0f;
            isDead_ = true;
        }
        Vector3 newScale = { baseScale_.x * shrinkScale_, baseScale_.y * shrinkScale_, baseScale_.z * shrinkScale_ };
        object_->SetScale(newScale);
    }
    // 将来的なアニメーションや移動処理をここに追加する
    // 例: position_.y -= 0.1f; // 落下など
    
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
        // OBBとして機能させる場合、親と同じ回転を適用する
        colliderObject_->SetRotation(rotation_);
        colliderObject_->Update();
    }
}

void Obstacle::Draw() {
    if (!isVisible_) return;
    
    if (object_ && !isDead_) {
        object_->Draw();
    }
}

void Obstacle::DrawCollider() {
    if (colliderObject_ && !isDead_) {
        colliderObject_->Draw();
    }
}

void Obstacle::OnCollision() {
    if (isDestructible_) {
        isDead_ = true;
    }
}

void Obstacle::Kill() {
    if (isDestructible_) {
        isDead_ = true;
    }
}

void Obstacle::SetTexturePath(const std::string& path) {
    texturePath_ = path;
    if (!texturePath_.empty() && object_ && object_->GetModel()) {
        TextureManager::GetInstance()->LoadTexture(texturePath_);
        uint32_t texIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(texturePath_);
        if (texIndex != TextureManager::GetInstance()->GetDefaultTextureIndex()) {
            object_->GetModel()->SetTextureIndex(texIndex);
        }
    }
}

bool Obstacle::CheckCollision(const Sphere& bulletSphere) const {
    if (isDead_) return false;

    Vector3 centerPos = {
        position_.x + collider_.center.x,
        position_.y + collider_.center.y,
        position_.z + collider_.center.z
    };

    if (collider_.type == "SPHERE") {
        Sphere mySphere = { centerPos, collider_.radius };
        return CollisionMath::IsCollision(bulletSphere, mySphere);
    } 
    else if (collider_.type == "BOX") {
        // 回転がある場合はOBB、ない場合はAABBとして扱う
        if (rotation_.x == 0.0f && rotation_.y == 0.0f && rotation_.z == 0.0f) {
            AABB aabb = {
                { centerPos.x - collider_.size.x, centerPos.y - collider_.size.y, centerPos.z - collider_.size.z },
                { centerPos.x + collider_.size.x, centerPos.y + collider_.size.y, centerPos.z + collider_.size.z }
            };
            return CollisionMath::IsCollision(bulletSphere, aabb);
        } else {
            // OBBを生成
            Matrix4x4 rotMat = Matrix4x4::RotateX(rotation_.x) * Matrix4x4::RotateY(rotation_.y) * Matrix4x4::RotateZ(rotation_.z);
            OBB obb = CollisionMath::CreateOBB(centerPos, collider_.size, rotMat);
            return CollisionMath::IsCollision(bulletSphere, obb);
        }
    }

    return false;
}

bool Obstacle::CheckRaycast(const Ray& ray, float* outDist) const {
    if (isDead_) return false;

    Vector3 centerPos = {
        position_.x + collider_.center.x,
        position_.y + collider_.center.y,
        position_.z + collider_.center.z
    };

    if (collider_.type == "SPHERE") {
        Sphere mySphere = { centerPos, collider_.radius };
        return CollisionMath::Raycast(ray, mySphere, outDist);
    } 
    else if (collider_.type == "BOX") {
        if (rotation_.x == 0.0f && rotation_.y == 0.0f && rotation_.z == 0.0f) {
            AABB aabb = {
                { centerPos.x - collider_.size.x, centerPos.y - collider_.size.y, centerPos.z - collider_.size.z },
                { centerPos.x + collider_.size.x, centerPos.y + collider_.size.y, centerPos.z + collider_.size.z }
            };
            return CollisionMath::Raycast(ray, aabb, outDist);
        } else {
            Matrix4x4 rotMat = Matrix4x4::RotateX(rotation_.x) * Matrix4x4::RotateY(rotation_.y) * Matrix4x4::RotateZ(rotation_.z);
            OBB obb = CollisionMath::CreateOBB(centerPos, collider_.size, rotMat);
            return CollisionMath::Raycast(ray, obb, outDist);
        }
    }

    return false;
}

void Obstacle::StartShrink() {
    isShrinking_ = true;
}