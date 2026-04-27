#include "Ring.h"
#include <cmath>
#include <numbers>

namespace KHPrimitive {

std::vector<RingVertex> CreateRingVertices(uint32_t division, float innerRadius, float outerRadius) {
    std::vector<RingVertex> vertices;
    vertices.reserve(division * 6); // 1分割につき2つの三角形（6頂点）

    float radianPerDivide = 2.0f * std::numbers::pi_v<float> / static_cast<float>(division);

    for (uint32_t i = 0; i < division; ++i) {
        float angle = static_cast<float>(i) * radianPerDivide;
        float nextAngle = static_cast<float>(i + 1) * radianPerDivide;

        float sin = std::sin(angle);
        float cos = std::cos(angle);
        float sinNext = std::sin(nextAngle);
        float cosNext = std::cos(nextAngle);

        float u = static_cast<float>(i) / static_cast<float>(division);
        float uNext = static_cast<float>(i + 1) / static_cast<float>(division);

        // 頂点定義 (画像に基づき時計回りに構成)
        // ① 外側現在
        RingVertex v1 = { {-sin * outerRadius, cos * outerRadius, 0.0f, 1.0f}, {u, 0.0f}, {0.0f, 0.0f, -1.0f} };
        // ② 外側次
        RingVertex v2 = { {-sinNext * outerRadius, cosNext * outerRadius, 0.0f, 1.0f}, {uNext, 0.0f}, {0.0f, 0.0f, -1.0f} };
        // ③ 内側現在
        RingVertex v3 = { {-sin * innerRadius, cos * innerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0.0f, 0.0f, -1.0f} };
        // ④ 内側次
        RingVertex v4 = { {-sinNext * innerRadius, cosNext * innerRadius, 0.0f, 1.0f}, {uNext, 1.0f}, {0.0f, 0.0f, -1.0f} };

        // 三角形1: ③-①-②
        vertices.push_back(v3);
        vertices.push_back(v1);
        vertices.push_back(v2);

        // 三角形2: ③-②-④
        vertices.push_back(v3);
        vertices.push_back(v2);
        vertices.push_back(v4);
    }

    return vertices;
}

} // namespace KHPrimitive
