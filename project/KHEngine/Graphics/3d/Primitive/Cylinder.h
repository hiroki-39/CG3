#pragma once
#include "KHEngine/Math/MathCommon.h"
#include <vector>

namespace KHPrimitive {

struct CylinderVertex {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

/**
 * @brief 円柱形状の頂点データを生成する
 * @param division 分割数
 * @param topRadius 上面半径
 * @param bottomRadius 底面半径
 * @param height 高さ
 * @return 頂点データのリスト（TRIANGLELIST用）
 */
std::vector<CylinderVertex> CreateCylinderVertices(uint32_t division, float topRadius, float bottomRadius, float height);

} // namespace KHPrimitive
