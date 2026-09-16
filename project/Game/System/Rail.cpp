#include "Rail.h"
#include <algorithm>
#include <cmath>

Rail::Rail() {}
Rail::~Rail() {}

void Rail::Initialize(const std::vector<LevelCurvePoint>& points) {
    points_ = points;
    totalLength_ = 0.0f;

    // 簡単な長さの近似（各区間の直線距離の合計）
    if (points_.size() > 1) {
        for (size_t i = 0; i < points_.size() - 1; ++i) {
            totalLength_ += (points_[i + 1].position - points_[i].position).Length();
        }

        // 制御点のイベントからトンネル/狭窄区間を自動登録
        narrowZones_.clear();
        int segmentCount = static_cast<int>(points_.size()) - 1;
        int narrowStart = -1;
        for (int i = 0; i < (int)points_.size(); ++i) {
            const std::string& ev = points_[i].event;
            bool isNarrow = (ev.find("tunnel") != std::string::npos || ev.find("narrow") != std::string::npos);
            if (isNarrow && narrowStart == -1) {
                narrowStart = i;
            } else if (!isNarrow && narrowStart != -1) {
                float sT = (float)narrowStart / segmentCount;
                float eT = (float)i / segmentCount;
                AddNarrowZone(sT, eT, 10.0f, -3.0f, 8.0f);
                narrowStart = -1;
            }
        }
        if (narrowStart != -1) {
            float sT = (float)narrowStart / segmentCount;
            AddNarrowZone(sT, 1.0f, 10.0f, -3.0f, 8.0f);
        }
    }
}

void Rail::GetSegment(float t, int& outIndex, float& outLocalT) const {
    if (points_.empty()) {
        outIndex = 0;
        outLocalT = 0.0f;
        return;
    }
    
    if (points_.size() == 1) {
        outIndex = 0;
        outLocalT = 0.0f;
        return;
    }

    // t は 0.0 ~ 1.0 の前提
    t = std::clamp(t, 0.0f, 1.0f);
    
    // 区間数を計算
    int segmentCount = static_cast<int>(points_.size()) - 1;
    
    // 全体を segmentCount 等分している簡易的な計算（本来は曲線長ベースが望ましいがここでは単純化）
    float scaledT = t * segmentCount;
    outIndex = static_cast<int>(scaledT);
    
    if (outIndex >= segmentCount) {
        outIndex = segmentCount - 1;
        outLocalT = 1.0f;
    } else {
        outLocalT = scaledT - static_cast<float>(outIndex);
    }
}

Vector3 Rail::GetPosition(float t) const {
    if (points_.empty()) return Vector3();
    if (points_.size() == 1) return points_[0].position;

    int i = 0;
    float localT = 0.0f;
    GetSegment(t, i, localT);

    // Catmull-Rom スプラインの制御点 P0, P1, P2, P3 を設定
    // P1とP2が現在の区間。P0はP1の1つ前、P3はP2の1つ後。
    // 範囲外の場合は端の点を複製する
    Vector3 P0 = (i - 1 >= 0) ? points_[i - 1].position : points_[i].position;
    Vector3 P1 = points_[i].position;
    Vector3 P2 = points_[i + 1].position;
    Vector3 P3 = (i + 2 < static_cast<int>(points_.size())) ? points_[i + 2].position : points_[i + 1].position;

    float t2 = localT * localT;
    float t3 = t2 * localT;

    // Catmull-Rom Spline 計算
    // P(t) = 0.5 * ( (2*P1) + (-P0 + P2)*t + (2*P0 - 5*P1 + 4*P2 - P3)*t^2 + (-P0 + 3*P1 - 3*P2 + P3)*t^3 )
    Vector3 result = (P1 * 2.0f) + 
                     (P2 - P0) * localT + 
                     (P0 * 2.0f - P1 * 5.0f + P2 * 4.0f - P3) * t2 + 
                     (P1 * 3.0f - P0 - P2 * 3.0f + P3) * t3;
                     
    return result * 0.5f;
}

Vector3 Rail::GetForward(float t) const {
    if (points_.empty()) return Vector3(0.0f, 0.0f, 1.0f);
    if (points_.size() == 1) return Vector3(0.0f, 0.0f, 1.0f);

    int i = 0;
    float localT = 0.0f;
    GetSegment(t, i, localT);

    // Catmull-Rom スプラインの制御点
    Vector3 P0 = (i - 1 >= 0) ? points_[i - 1].position : points_[i].position;
    Vector3 P1 = points_[i].position;
    Vector3 P2 = points_[i + 1].position;
    Vector3 P3 = (i + 2 < static_cast<int>(points_.size())) ? points_[i + 2].position : points_[i + 1].position;

    float t2 = localT * localT;

    // Catmull-Rom Spline 微分 (速度ベクトル)
    // P'(t) = 0.5 * ( (-P0 + P2) + 2*(2*P0 - 5*P1 + 4*P2 - P3)*t + 3*(-P0 + 3*P1 - 3*P2 + P3)*t^2 )
    Vector3 derivative = (P2 - P0) + 
                         (P0 * 2.0f - P1 * 5.0f + P2 * 4.0f - P3) * (2.0f * localT) + 
                         (P1 * 3.0f - P0 - P2 * 3.0f + P3) * (3.0f * t2);
                         
    derivative = derivative * 0.5f;

    float len = derivative.Length();
    if (len > 0.0001f) {
        return derivative.Normalize();
    }
    
    // 微分がゼロベクトルの場合は、単純に次の点までの方向を返す
    return (P2 - P1).Normalize();
}

float Rail::GetTilt(float t) const {
    if (points_.empty()) return 0.0f;
    if (points_.size() == 1) return points_[0].tilt;

    int i = 0;
    float localT = 0.0f;
    GetSegment(t, i, localT);

    // 単純な線形補間
    return points_[i].tilt * (1.0f - localT) + points_[i + 1].tilt * localT;
}

float Rail::GetSpeed(float t) const {
    if (points_.empty()) return 20.0f;
    if (points_.size() == 1) return points_[0].speed;

    int i = 0;
    float localT = 0.0f;
    GetSegment(t, i, localT);

    // 単純な線形補間
    return points_[i].speed * (1.0f - localT) + points_[i + 1].speed * localT;
}

std::string Rail::GetEvent(float t) const {
    if (points_.empty()) return "none";
    if (points_.size() == 1) return points_[0].event;

    int i = 0;
    float localT = 0.0f;
    GetSegment(t, i, localT);

    // イベントは補間できないため、現在の区間（始点）のイベントを返す
    return points_[i].event;
}

void Rail::AddNarrowZone(float startT, float endT, float limitX, float limitYMin, float limitYMax) {
    NarrowZone zone;
    zone.startT = startT;
    zone.endT = endT;
    zone.limitX = limitX;
    zone.limitYMin = limitYMin;
    zone.limitYMax = limitYMax;
    narrowZones_.push_back(zone);
}

void Rail::GetMoveLimits(float t, float& outLimitX, float& outLimitYMin, float& outLimitYMax) const {
    outLimitX = 25.0f;
    outLimitYMin = -5.0f;
    outLimitYMax = 12.0f;

    // 前後0.03（進行度の3%）を手前からの導入・脱出フェード区間とする
    const float fadeDist = 0.03f;

    for (const auto& zone : narrowZones_) {
        float effectiveStart = zone.startT - fadeDist;
        float effectiveEnd = zone.endT + fadeDist;

        if (t >= effectiveStart && t <= effectiveEnd) {
            float weight = 1.0f;
            if (t < zone.startT) {
                // 入口手前フェードイン
                weight = (t - effectiveStart) / fadeDist;
            } else if (t > zone.endT) {
                // 出口後フェードアウト
                weight = (effectiveEnd - t) / fadeDist;
            }

            // スムーズステップ (3w^2 - 2w^3)
            weight = weight * weight * (3.0f - 2.0f * weight);

            outLimitX = (1.0f - weight) * 25.0f + weight * zone.limitX;
            outLimitYMin = (1.0f - weight) * (-5.0f) + weight * zone.limitYMin;
            outLimitYMax = (1.0f - weight) * 12.0f + weight * zone.limitYMax;
            return;
        }
    }
}
