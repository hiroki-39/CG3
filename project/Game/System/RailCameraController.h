#pragma once
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "Game/System/Rail.h"
#include <algorithm>
#include <cmath>

class RailCameraController {
public:
    RailCameraController() = default;
    ~RailCameraController() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="rails">追従する複数のレール</param>
    /// <param name="camera">描画用カメラ</param>
    /// <param name="parentObject">プレイヤー等の親となるオブジェクト</param>
    void Initialize(const std::vector<Rail*>& rails, Camera* camera, Object3d* parentObject);

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <param name="gameSpeed">ゲームスピードの倍率</param>
    /// <param name="playerLocalPos">プレイヤーのローカルオフセット（パンやパララックス用）</param>
    void Update(float gameSpeed = 1.0f, const Vector3& playerLocalPos = {0.0f, 0.0f, 0.0f});

    /// <summary>
    /// レールの先頭にリセットする
    /// </summary>
    void Reset();

    /// <summary>
    /// 進行度を取得
    /// </summary>
    float GetProgress() const { return progress_; }

    /// <summary>
    /// 進行度を設定（ImGui操作用など）
    /// </summary>
    void SetProgress(float p) {
        progress_ = std::clamp(p, 0.0f, 1.0f);
        ApplyTransform({0.0f, 0.0f, 0.0f});
    }

    /// <summary>
    /// レール基準速度の設定（秒速m/s、ゲーム側で一括管理）
    /// </summary>
    void SetBaseSpeed(float speed) { baseSpeed_ = speed; }
    float GetBaseSpeed() const { return baseSpeed_; }

    /// <summary>
    /// 速度倍率の設定（ブースト・ブレーキ等）
    /// </summary>
    void SetSpeedMultiplier(float mult) { speedMultiplier_ = mult; }
    float GetSpeedMultiplier() const { return speedMultiplier_; }

    /// <summary>
    /// カメラのローカルオフセット設定
    /// </summary>
    void SetCameraOffset(const Vector3& offset) { cameraOffset_ = offset; }
    const Vector3& GetCameraOffset() const { return cameraOffset_; }

    /// <summary>
    /// カメラの自機追従率設定 (スターフォックス仕様では微小または0)
    /// </summary>
    void SetCameraFollowRate(float rateX, float rateY) { cameraFollowRateX_ = rateX; cameraFollowRateY_ = rateY; }

    /// <summary>
    /// ImGuiデバッグUI
    /// </summary>
    void DrawImGui();

private:
    void ApplyTransform(const Vector3& playerLocalPos);

private:
    std::vector<Rail*> rails_;
    int currentRailIndex_ = 0;
    Camera* camera_ = nullptr;
    Object3d* parentObject_ = nullptr;

    float progress_ = 0.0f;
    float baseSpeed_ = 50.0f;          // レール進行速度（秒速50m）
    float speedMultiplier_ = 1.0f;     // ブースト等による速度倍率
    Vector3 cameraOffset_ = { 0.0f, 2.5f, -8.0f }; // カメラオフセット
    Vector3 currentCameraLocalPos_ = { 0.0f, 2.5f, -8.0f }; // スムーズ補間用カメラローカル位置
    float cameraFollowRateX_ = 0.70f;  // 自機の移動に伴うカメラ横追従（自機を画面内に収めつつ大きくスライド）
    float cameraFollowRateY_ = 0.60f;  // 自機の移動に伴うカメラ縦追従
};
