#include "RailCameraController.h"

void RailCameraController::Initialize(Rail* rail, Camera* camera, Object3d* parentObject) {
    rail_ = rail;
    camera_ = camera;
    parentObject_ = parentObject;
    progress_ = 0.0f;

    ApplyTransform();
}

void RailCameraController::Update(float deltaProgress) {
    if (!rail_ || !rail_->IsValid()) return;

    progress_ += deltaProgress;
    progress_ = std::clamp(progress_, 0.0f, 1.0f);

    ApplyTransform();
}

void RailCameraController::Reset() {
    progress_ = 0.0f;
    ApplyTransform();
}

void RailCameraController::ApplyTransform() {
    if (!rail_ || !rail_->IsValid()) return;

    // カメラをレールから少し浮かせる
    Vector3 baseEye = rail_->GetPosition(progress_);
    Vector3 eye = baseEye;
    eye.y += 0.2f;

    // 注視点は少し進んだ地点
    float targetProgress = std::min(progress_ + 0.01f, 1.0f);
    Vector3 baseTarget = rail_->GetPosition(targetProgress);

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
    float railTilt = rail_->GetTilt(progress_);

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
