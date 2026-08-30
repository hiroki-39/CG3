#include "EnhanceRing.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "Game/Actor/Player/Player.h"
#include <cmath>

void EnhanceRing::Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const Vector3& rotation, const std::string& fileName, RingType type, uint32_t skyboxTexIndex) {
    position_ = pos;
    baseScale_ = scale;
    rotation_ = rotation;
    type_ = type;

    // スケールの中の最大のものを半径とする
    radius_ = scale.x > scale.y ? (scale.x > scale.z ? scale.x : scale.z) : (scale.y > scale.z ? scale.y : scale.z);

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetTranslate(position_);
    object_->SetScale(baseScale_);
    object_->SetRotation(rotation_);

    // モデルの読み込み。タイプによって色やモデルを変える
    std::string modelName = fileName;
    if (modelName.empty()) {
        modelName = "Ring.obj";
    } else if (modelName.find(".obj") == std::string::npos) {
        modelName += ".obj";
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
    
    Update();
}

void EnhanceRing::Update() {
    if (isShrinking_) {
        shrinkScale_ -= 0.1f;
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
}

void EnhanceRing::Draw() {
    if (object_ && !isDead_) {
        object_->Draw();
    }
}

void EnhanceRing::StartShrink() {
    isShrinking_ = true;
}

bool EnhanceRing::CheckPassThrough(Player* player) {
    if (!player || isDead_ || isShrinking_) return false;

    // ワールド行列の逆行列を使い、プレイヤーの座標をリングのローカル空間に変換する
    Matrix4x4 ringWorldMat = object_->GetmatWorld();
    Matrix4x4 invRingMat = Matrix4x4::Inverse(ringWorldMat);

    Vector3 pPrev = player->GetPreviousTranslate();
    Vector3 pCurr = player->GetTranslate();

    // プレイヤーのワールド座標をリングのローカル座標系に変換
    Vector3 localPrev = invRingMat * pPrev;
    Vector3 localCurr = invRingMat * pCurr;

    // Z軸（リングの平面はローカルのXY平面）をまたいだか判定
    if ((localPrev.z <= 0.0f && localCurr.z >= 0.0f) || (localPrev.z >= 0.0f && localCurr.z <= 0.0f)) {
        // Zがまたいだ場合、Z=0となる交点のローカルX, Yを線形補間で求める
        float t = 0.0f;
        float diffZ = localCurr.z - localPrev.z;
        if (std::abs(diffZ) > 0.0001f) {
            t = (0.0f - localPrev.z) / diffZ;
        }

        float intersectX = localPrev.x + (localCurr.x - localPrev.x) * t;
        float intersectY = localPrev.y + (localCurr.y - localPrev.y) * t;

        // 交点がリングの半径内にあるか（※ローカル空間でのスケールは1になっているので、radiusとの比較ではなくローカルでの距離が1以内かで判定する）
        // スケールは ringWorldMat に含まれているため、local座標の距離が1.0以下ならスケール内の円柱内を通ったことになる
        // リングの厚み等を考慮し、大まかに判定（内側の穴の大きさを考慮。ドーナツ型なら外径1.0、内径0.5など。ここでは0.8以内なら通ったとする等）
        float distSq = intersectX * intersectX + intersectY * intersectY;
        
        // 判定半径のしきい値。必要に応じて調整。
        if (distSq <= 1.0f * 1.0f) {
            return true;
        }
    }

    return false;
}
