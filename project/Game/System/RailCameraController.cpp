#include "RailCameraController.h"

void RailCameraController::Initialize(const std::vector<Rail*>& rails, Camera* camera, Object3d* parentObject) {
    rails_ = rails;
    camera_ = camera;
    parentObject_ = parentObject;
    progress_ = 0.0f;
    currentRailIndex_ = 0;

    ApplyTransform();
}

void RailCameraController::Update(float gameSpeed) {
    if (rails_.empty() || currentRailIndex_ >= rails_.size() || !rails_[currentRailIndex_]->IsValid()) return;
    
    Rail* currentRail = rails_[currentRailIndex_];

    // 現在地点での設定スピード（m/s）を取得
    float currentSpeed = currentRail->GetSpeed(progress_);
    
    // 1フレーム（60FPS想定）あたりの移動距離
    float distancePerFrame = (currentSpeed * gameSpeed) / 60.0f;
    
    // レール全長に対する割合（進行度）に変換
    float totalLength = currentRail->GetTotalLength();
    if (totalLength > 0.0001f) {
        float deltaProgress = distancePerFrame / totalLength;
        progress_ += deltaProgress;
        
        // 終点に達した場合の処理
        if (progress_ >= 1.0f) {
            if (currentRailIndex_ < rails_.size() - 1) {
                // 次のレールへ乗り換え（超過分は今のところ単純に0に戻す）
                currentRailIndex_++;
                progress_ = 0.0f;
            } else {
                progress_ = 1.0f; // 最後のレールなら停止
            }
        }
    }

    ApplyTransform();
}

void RailCameraController::Reset() {
    progress_ = 0.0f;
    currentRailIndex_ = 0;
    ApplyTransform();
}

void RailCameraController::ApplyTransform() {
    if (rails_.empty() || currentRailIndex_ >= rails_.size() || !rails_[currentRailIndex_]->IsValid()) return;
    Rail* currentRail = rails_[currentRailIndex_];

    // カメラをレールから少し浮かせる
    Vector3 baseEye = currentRail->GetPosition(progress_);
    Vector3 eye = baseEye;
    eye.y += 0.2f;

    // 注視点は少し進んだ地点
    float targetProgress = std::min(progress_ + 0.01f, 1.0f);
    Vector3 baseTarget = currentRail->GetPosition(targetProgress);

    // 差分ベクトル (forward)
    Vector3 forward = {
        baseTarget.x - baseEye.x,
        baseTarget.y - baseEye.y,
        baseTarget.z - baseEye.z
    };

    // 正規化
    float len = std::sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (len > 1e-6f) {
        forward.x /= len; forward.y /= len; forward.z /= len;
    }

    // 目標の回転角の計算（レールの接線にピッタリ合わせる）
    float targetYaw = std::atan2(forward.x, forward.z);
    float targetPitch = std::asin(-forward.y);
    float railTilt = currentRail->GetTilt(progress_);

    Vector3 cameraRot(targetPitch, targetYaw, railTilt);

    if (parentObject_) {
        // カメラは y+0.2f だが、自機の親オブジェクトのYオフセットが元々どのように使われていたかを考慮。
        // （元の実装では parentObject_ (cameraObject_) も y+0.2f を適用していたためそれに合わせる）
        parentObject_->SetTranslate(eye);
        parentObject_->SetRotation(cameraRot);
        parentObject_->Update();
    }

    if (camera_) {
        camera_->SetTranslate(eye);
        camera_->SetRotation(cameraRot);
        camera_->Update();
    }
}
