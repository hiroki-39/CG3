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

    ApplyTransform(playerLocalPos);
}

void RailCameraController::Reset() {
    progress_ = 0.0f;
    currentRailIndex_ = 0;
    ApplyTransform({0.0f, 0.0f, 0.0f});
}

void RailCameraController::ApplyTransform(const Vector3& playerLocalPos) {
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
        // プレイヤーの親（アンカー）はレールに完全に沿わせる（傾きやパンは入れない）
        parentObject_->SetTranslate(eye);
        parentObject_->SetRotation(cameraRot);
        parentObject_->Update();
    }

    if (camera_) {
        // カメラ（実際の視点）にはプレイヤーの位置に応じた首振り（パン・ピッチ）を入れる
        // プレイヤーが大きく動いても画面内に収まるようにする
        // カメラがプレイヤーの方を向きすぎると正面（進行方向）が見えなくなるため、
        // 首振り（パン）はごく僅かに抑え、平行移動（パララックス）で画面内に収める
        float panStrengthX = 0.002f; // ほぼ正面を向いたままにする
        float panStrengthY = 0.002f;

        // パララックス強度（プレイヤーの移動の何割をカメラがついていくか）
        // 高め（0.6〜0.7）にすると、プレイヤーが画面外に出にくく、かつ正面を向き続けられる
        float parallaxStrengthX = 0.7f; 
        float parallaxStrengthY = 0.6f;

        Vector3 cameraEye = eye;
        
        // レールの右方向と上方向ベクトルを計算（近似）
        Vector3 right = {
            std::cos(-railTilt) * std::cos(targetYaw),
            std::sin(-railTilt),
            -std::cos(-railTilt) * std::sin(targetYaw)
        };
        Vector3 up = {
            -std::sin(targetPitch) * std::sin(targetYaw),
            std::cos(targetPitch),
            std::sin(targetPitch) * std::cos(targetYaw)
        };
        
        // パララックス：プレイヤーの移動にカメラがしっかり並行移動でついていく
        cameraEye.x += right.x * playerLocalPos.x * parallaxStrengthX;
        cameraEye.y += right.y * playerLocalPos.x * parallaxStrengthX;
        cameraEye.z += right.z * playerLocalPos.x * parallaxStrengthX;

        cameraEye.x += up.x * playerLocalPos.y * parallaxStrengthY;
        cameraEye.y += up.y * playerLocalPos.y * parallaxStrengthY;
        cameraEye.z += up.z * playerLocalPos.y * parallaxStrengthY;
        
        float panYaw = playerLocalPos.x * panStrengthX;
        float panPitch = playerLocalPos.y * panStrengthY;

        Vector3 finalCameraRot = cameraRot;
        finalCameraRot.y += panYaw;
        finalCameraRot.x -= panPitch; // Yが上のとき、見上げるにはX回転がマイナス方向

        camera_->SetTranslate(cameraEye);
        camera_->SetRotation(finalCameraRot);
        camera_->Update();
    }
}
