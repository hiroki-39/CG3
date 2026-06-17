#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

// 球体
struct Sphere {
    Vector3 center;
    float radius;
};

// 軸並行境界ボックス (Axis Aligned Bounding Box)
struct AABB {
    Vector3 min;
    Vector3 max;
};

// 有向境界ボックス (Oriented Bounding Box)
struct OBB {
    Vector3 center;
    Vector3 orientations[3]; // X, Y, Zのローカル軸 (正規化ベクトル)
    Vector3 halfExtents;     // 中心からの各ローカル軸方向へのサイズの半分
};

namespace CollisionMath {
    // 交差判定関数
    bool IsCollision(const Sphere& s1, const Sphere& s2);
    bool IsCollision(const Sphere& sphere, const AABB& aabb);
    bool IsCollision(const Sphere& sphere, const OBB& obb);
    
    // OBBの構築ヘルパー
    // rotateMatrixは3x3の回転部分を含むMatrix4x4（平行移動を含まない純粋な回転行列を想定）
    OBB CreateOBB(const Vector3& center, const Vector3& size, const Matrix4x4& rotateMatrix);
}
