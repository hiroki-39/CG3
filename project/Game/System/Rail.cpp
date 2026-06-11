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
