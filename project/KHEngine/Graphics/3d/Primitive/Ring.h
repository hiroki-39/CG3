#pragma once
#include "KHEngine/Math/MathCommon.h"
#include <vector>

namespace KHPrimitive {

struct RingVertex {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

/**
 * @brief リング形状の頂点データを生成する
 * @param division 分割数
 * @param innerRadius 内径
 * @param outerRadius 外径
 * @return 頂点データのリスト（TRIANGLELIST用）
 */
std::vector<RingVertex> CreateRingVertices(uint32_t division, float innerRadius, float outerRadius);

} // namespace KHPrimitive
