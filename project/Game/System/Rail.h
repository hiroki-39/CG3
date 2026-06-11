#pragma once
#include "KHEngine/Scene/LevelLoader.h"
#include "KHEngine/Math/Vector3.h"
#include <vector>

class Rail {
public:
    Rail();
    ~Rail();

    /// <summary>
    /// レールの制御点を初期化する
    /// </summary>
    void Initialize(const std::vector<LevelCurvePoint>& points);

    /// <summary>
    /// 進行度(t) における3D座標を取得する
    /// </summary>
    /// <param name="t">進行度 (0.0f = 始点, 1.0f = 終点)</param>
    Vector3 GetPosition(float t) const;

    /// <summary>
    /// 進行度(t) における進行方向(接線ベクトル)を取得する
    /// </summary>
    Vector3 GetForward(float t) const;

    /// <summary>
    /// 進行度(t) におけるロール角（Tilt）を取得する
    /// </summary>
    float GetTilt(float t) const;

    /// <summary>
    /// レールの総長を取得する（近似値）
    /// </summary>
    float GetTotalLength() const { return totalLength_; }

    /// <summary>
    /// 制御点がセットされているか
    /// </summary>
    bool IsValid() const { return !points_.empty(); }

private:
    std::vector<LevelCurvePoint> points_;
    float totalLength_ = 0.0f;

    // 区間と区間内進行度を計算する補助関数
    void GetSegment(float t, int& outIndex, float& outLocalT) const;
};
