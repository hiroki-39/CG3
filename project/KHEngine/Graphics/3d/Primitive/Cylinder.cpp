#include "Cylinder.h"
#include <cmath>
#include <numbers>

namespace KHPrimitive {

std::vector<CylinderVertex> CreateCylinderVertices(uint32_t division, float topRadius, float bottomRadius, float height) {
    std::vector<CylinderVertex> vertices;
    vertices.reserve(division * 6);

    float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(division);

    for (uint32_t index = 0; index < division; ++index) {
        float sin = std::sin(index * radianPerDivide);
        float cos = std::cos(index * radianPerDivide);
        float sinNext = std::sin((index + 1) * radianPerDivide);
        float cosNext = std::cos((index + 1) * radianPerDivide);

        float u = float(index) / float(division);
        float uNext = float(index + 1) / float(division);

        // 頂点定義（画像に基づき構築）
        // ① 左上 (現在・上)
        CylinderVertex v1 = { {-sin * topRadius, height, cos * topRadius, 1.0f}, {u, 0.0f}, {-sin, 0.0f, cos} };
        // ② 右上 (次・上)
        CylinderVertex v2 = { {-sinNext * topRadius, height, cosNext * topRadius, 1.0f}, {uNext, 0.0f}, {-sinNext, 0.0f, cosNext} };
        // ③ 左下 (現在・下)
        CylinderVertex v3 = { {-sin * bottomRadius, 0.0f, cos * bottomRadius, 1.0f}, {u, 1.0f}, {-sin, 0.0f, cos} };
        // ④ 右下 (次・下)
        CylinderVertex v4 = { {-sinNext * bottomRadius, 0.0f, cosNext * bottomRadius, 1.0f}, {uNext, 1.0f}, {-sinNext, 0.0f, cosNext} };

        // 三角形1: ③-①-② （スライドだと: -sin(bottom), -sin(top), -sinNext(top) -> 3, 1, 2）
        vertices.push_back(v3);
        vertices.push_back(v1);
        vertices.push_back(v2);

        // 三角形2: ③-②-④ （スライドだと: -sin(bottom), -sinNext(top), -sinNext(bottom) -> 3, 2, 4）
        vertices.push_back(v3);
        vertices.push_back(v2);
        vertices.push_back(v4);
    }

    return vertices;
}

} // namespace KHPrimitive
