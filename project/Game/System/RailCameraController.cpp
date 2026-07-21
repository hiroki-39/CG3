#include "RailCameraController.h"

void RailCameraController::Initialize(const std::vector<Rail*>& rails, Camera* camera, Object3d* parentObject) {
    rails_ = rails;
    camera_ = camera;
    parentObject_ = parentObject;
    progress_ = 0.0f;
    currentRailIndex_ = 0;

    ApplyTransform({0.0f, 0.0f, 0.0f});
}

void RailCameraController::Update(float gameSpeed, const Vector3& playerLocalPos) {
    if (rails_.empty() || currentRailIndex_ >= rails_.size() || !rails_[currentRailIndex_]->IsValid()) {
        ApplyTransform(playerLocalPos);
        return;
    }
    
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

    ApplyTransform(playerLocalPos);
}

void RailCameraController::Reset() {
    progress_ = 0.0f;
    currentRailIndex_ = 0;
    ApplyTransform({0.0f, 0.0f, 0.0f});
}

void RailCameraController::ApplyTransform(const Vector3& playerLocalPos) {
    Vector3 eye = {0.0f, 0.2f, 0.0f};
    Vector3 anchorRot = {0.0f, 0.0f, 0.0f};
    float railTilt = 0.0f;
    float targetPitch = 0.0f;
    float targetYaw = 0.0f;

    if (!rails_.empty() && currentRailIndex_ < rails_.size() && rails_[currentRailIndex_]->IsValid()) {
        Rail* currentRail = rails_[currentRailIndex_];

        // カメラをレールから少し浮かせる
        Vector3 baseEye = currentRail->GetPosition(progress_);
        eye = baseEye;
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
        targetYaw = std::atan2(forward.x, forward.z);
        targetPitch = std::asin(-forward.y);
        railTilt = currentRail->GetTilt(progress_);

        anchorRot = Vector3(targetPitch, targetYaw, railTilt);
    }

    if (parentObject_) {
        // 大元の親（アンカー）はレールに完全に沿わせる
        parentObject_->SetTranslate(eye);
        parentObject_->SetRotation(anchorRot);
        parentObject_->Update();
    }

    if (camera_) {
        // レール基準のワールド行列（親オブジェクトの回転行列）を計算
        // 回転の順番は Z -> X -> Y （エンジン仕様による）
        Matrix4x4 rotMatrix = Matrix4x4::RotateZ(railTilt) * Matrix4x4::RotateX(targetPitch) * Matrix4x4::RotateY(targetYaw);
        
        // カメラの基本ローカル位置（アンカーから後ろ、少し上）
        // さらにカメラをプレイヤーに近づける（Zを-10.0fから0.0fへ、Yを2.0fから1.5fへ変更）
        Vector3 baseCameraLocalPos = { 0.0f, 1.5f, 0.0f };
        
        // プレイヤーの移動量に対するカメラの並行移動追従率
        float cameraFollowRateX = 1.0f;
        float cameraFollowRateY = 1.0f;
        
        Vector3 cameraLocalPos = {
            baseCameraLocalPos.x + playerLocalPos.x * cameraFollowRateX,
            baseCameraLocalPos.y + playerLocalPos.y * cameraFollowRateY,
            baseCameraLocalPos.z
        };

        // ローカル位置をワールド位置に変換
        Vector3 cameraWorldPos = {
            cameraLocalPos.x * rotMatrix.m[0][0] + cameraLocalPos.y * rotMatrix.m[1][0] + cameraLocalPos.z * rotMatrix.m[2][0] + eye.x,
            cameraLocalPos.x * rotMatrix.m[0][1] + cameraLocalPos.y * rotMatrix.m[1][1] + cameraLocalPos.z * rotMatrix.m[2][1] + eye.y,
            cameraLocalPos.x * rotMatrix.m[0][2] + cameraLocalPos.y * rotMatrix.m[1][2] + cameraLocalPos.z * rotMatrix.m[2][2] + eye.z
        };

        // 首振り（パン）を無くし、レールアンカーと完全に同じ角度（平行）にする
        Vector3 finalCameraRot = anchorRot;

        camera_->SetTranslate(cameraWorldPos);
        camera_->SetRotation(finalCameraRot);
        camera_->Update();
    }
}
