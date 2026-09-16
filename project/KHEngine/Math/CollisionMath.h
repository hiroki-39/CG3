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

// 光線 (Ray)
struct Ray {
    Vector3 origin;
    Vector3 direction; // 正規化された方向ベクトル
};

// 三角形
struct Triangle {
    Vector3 p0;
    Vector3 p1;
    Vector3 p2;
    Vector3 normal;
};

// 衝突結果詳細
struct CollisionResult {
    bool isHit = false;
    Vector3 hitPoint = { 0.0f, 0.0f, 0.0f };
    Vector3 normal = { 0.0f, 1.0f, 0.0f }; // 接触面から押し出す法線ベクトル
    float penetrationDepth = 0.0f;        // めり込み深さ
};

namespace CollisionMath {
    // 交差判定関数
    bool IsCollision(const Sphere& s1, const Sphere& s2);
    bool IsCollision(const Sphere& sphere, const AABB& aabb);
    bool IsCollision(const Sphere& sphere, const OBB& obb);
    bool IsCollision(const Sphere& sphere, const Triangle& tri, CollisionResult* outResult = nullptr);
    bool IsCollision(const OBB& obb, const AABB& aabb);
    bool IsCollision(const OBB& obb, const Triangle& tri, CollisionResult* outResult = nullptr);
    AABB GetBoundingAABB(const OBB& obb);
    
    // 最近接点計算ヘルパー
    Vector3 ClosestPointOnTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c);
    
    // Raycast関数
    bool Raycast(const Ray& ray, const Sphere& sphere, float* outDistance = nullptr);
    bool Raycast(const Ray& ray, const AABB& aabb, float* outDistance = nullptr);
    bool Raycast(const Ray& ray, const OBB& obb, float* outDistance = nullptr);
    
    // OBBの構築ヘルパー
    // rotateMatrixは3x3の回転部分を含むMatrix4x4（平行移動を含まない純粋な回転行列を想定）
    OBB CreateOBB(const Vector3& center, const Vector3& size, const Matrix4x4& rotateMatrix);
}
