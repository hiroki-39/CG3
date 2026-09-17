#include "RailCameraController.h"
#include "externals/imgui/imgui.h"

void RailCameraController::Initialize(const std::vector<Rail*>& rails, Camera* camera, Object3d* parentObject) {
    rails_ = rails;
    camera_ = camera;
    parentObject_ = parentObject;
    progress_ = 0.0f;
    currentRailIndex_ = 0;
    speedMultiplier_ = 1.0f;
    currentCameraLocalPos_ = cameraOffset_;

    ApplyTransform({0.0f, 0.0f, 0.0f});
}

void RailCameraController::Update(float gameSpeed, const Vector3& playerLocalPos) {
    if (rails_.empty() || currentRailIndex_ >= rails_.size() || !rails_[currentRailIndex_]->IsValid()) {
        ApplyTransform(playerLocalPos);
        return;
    }
    
    Rail* currentRail = rails_[currentRailIndex_];

    // ゲーム側で指定された基準速度 × 速度倍率（ブースト等）
    float currentSpeed = baseSpeed_ * speedMultiplier_;
    if (!currentRail->IsUsingCustomSpeed()) {
        currentSpeed = currentRail->GetSpeed(progress_) * speedMultiplier_;
    }
    
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
    speedMultiplier_ = 1.0f;
    currentCameraLocalPos_ = cameraOffset_;
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
        
        // スターフォックス仕様: 自機の移動に合わせてカメラも大きくスライド追従（70%）
        Vector3 targetCameraLocalPos = {
            cameraOffset_.x + playerLocalPos.x * cameraFollowRateX_,
            cameraOffset_.y + playerLocalPos.y * cameraFollowRateY_,
            cameraOffset_.z
        };

        // スムーズな遅延補間（自機が動くとカメラが滑らかにスライド追従）
        float followLerp = 0.15f;
        currentCameraLocalPos_.x += (targetCameraLocalPos.x - currentCameraLocalPos_.x) * followLerp;
        currentCameraLocalPos_.y += (targetCameraLocalPos.y - currentCameraLocalPos_.y) * followLerp;
        currentCameraLocalPos_.z = targetCameraLocalPos.z;

        // ローカル位置をワールド位置に変換
        Vector3 cameraWorldPos = {
            currentCameraLocalPos_.x * rotMatrix.m[0][0] + currentCameraLocalPos_.y * rotMatrix.m[1][0] + currentCameraLocalPos_.z * rotMatrix.m[2][0] + eye.x,
            currentCameraLocalPos_.x * rotMatrix.m[0][1] + currentCameraLocalPos_.y * rotMatrix.m[1][1] + currentCameraLocalPos_.z * rotMatrix.m[2][1] + eye.y,
            currentCameraLocalPos_.x * rotMatrix.m[0][2] + currentCameraLocalPos_.y * rotMatrix.m[1][2] + currentCameraLocalPos_.z * rotMatrix.m[2][2] + eye.z
        };

        // 自機とカメラの相対ズレに応じた微小なカメラ首振り（自機を常に中心付近に捉え、見切れを完全防止）
        float relX = playerLocalPos.x - currentCameraLocalPos_.x;
        float relY = playerLocalPos.y - currentCameraLocalPos_.y;
        float cameraLookYaw = relX * 0.015f;
        float cameraLookPitch = -relY * 0.012f;
        Vector3 finalCameraRot = {
            anchorRot.x + cameraLookPitch,
            anchorRot.y + cameraLookYaw,
            anchorRot.z
        };

        camera_->SetTranslate(cameraWorldPos);
        camera_->SetRotation(finalCameraRot);
        camera_->Update();
    }
}

void RailCameraController::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::Begin("Rail & Camera Controller")) {
        ImGui::SliderFloat("Rail Base Speed (m/s)", &baseSpeed_, 5.0f, 150.0f);
        ImGui::SliderFloat("Speed Multiplier", &speedMultiplier_, 0.2f, 3.0f);
        ImGui::SliderFloat("Progress", &progress_, 0.0f, 1.0f);
        ImGui::DragFloat3("Camera Offset", &cameraOffset_.x, 0.1f);
        ImGui::SliderFloat("Camera Follow Rate X", &cameraFollowRateX_, 0.0f, 1.0f);
        ImGui::SliderFloat("Camera Follow Rate Y", &cameraFollowRateY_, 0.0f, 1.0f);
    }
    ImGui::End();
#endif
}
