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

    const auto& p0 = points_[i];
    const auto& p1 = points_[i + 1];

    // 3次ベジェ曲線の制御点
    Vector3 P0 = p0.position;
    Vector3 P1 = p0.handle_right;
    Vector3 P2 = p1.handle_left;
    Vector3 P3 = p1.position;

    float mt = 1.0f - localT;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    float t2 = localT * localT;
    float t3 = t2 * localT;

    // P(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3
    return (P0 * mt3) + (P1 * (3.0f * mt2 * localT)) + (P2 * (3.0f * mt * t2)) + (P3 * t3);
}

Vector3 Rail::GetForward(float t) const {
    if (points_.empty()) return Vector3(0.0f, 0.0f, 1.0f);
    if (points_.size() == 1) return Vector3(0.0f, 0.0f, 1.0f);

    int i = 0;
    float localT = 0.0f;
    GetSegment(t, i, localT);

    const auto& p0 = points_[i];
    const auto& p1 = points_[i + 1];

    Vector3 P0 = p0.position;
    Vector3 P1 = p0.handle_right;
    Vector3 P2 = p1.handle_left;
    Vector3 P3 = p1.position;

    float mt = 1.0f - localT;
    float mt2 = mt * mt;
    float t2 = localT * localT;

    // P'(t) = 3(1-t)^2*(P1-P0) + 6(1-t)t*(P2-P1) + 3t^2*(P3-P2)
    Vector3 derivative = ((P1 - P0) * (3.0f * mt2)) + ((P2 - P1) * (6.0f * mt * localT)) + ((P3 - P2) * (3.0f * t2));

    float len = derivative.Length();
    if (len > 0.0001f) {
        return derivative.Normalize();
    }
    
    // 微分がゼロベクトルの場合は、単純に次の点までの方向を返す
    return (P3 - P0).Normalize();
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
