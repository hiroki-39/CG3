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
    void Update(float gameSpeed = 1.0f);

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
        ApplyTransform();
    }

private:
    void ApplyTransform();

private:
    std::vector<Rail*> rails_;
    int currentRailIndex_ = 0;
    Camera* camera_ = nullptr;
    Object3d* parentObject_ = nullptr;

    float progress_ = 0.0f;
};
