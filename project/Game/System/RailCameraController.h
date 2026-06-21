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
    /// <param name="rail">追従するレール</param>
    /// <param name="camera">描画用カメラ</param>
    /// <param name="parentObject">プレイヤー等の親となるオブジェクト</param>
    void Initialize(Rail* rail, Camera* camera, Object3d* parentObject);

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
    Rail* rail_ = nullptr;
    Camera* camera_ = nullptr;
    Object3d* parentObject_ = nullptr;

    float progress_ = 0.0f;
};
