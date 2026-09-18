#include "CollisionMath.h"
#include <algorithm>
#include <cmath>

namespace CollisionMath {

    bool IsCollision(const Sphere& s1, const Sphere& s2) {
        float dx = s1.center.x - s2.center.x;
        float dy = s1.center.y - s2.center.y;
        float dz = s1.center.z - s2.center.z;
        float distanceSq = dx * dx + dy * dy + dz * dz;
        float radiusSum = s1.radius + s2.radius;
        return distanceSq <= (radiusSum * radiusSum);
    }

    bool IsCollision(const Sphere& sphere, const AABB& aabb) {
        // AABB上の球の中心に最も近い点を求める
        Vector3 closestPoint;
        closestPoint.x = std::clamp(sphere.center.x, aabb.min.x, aabb.max.x);
        closestPoint.y = std::clamp(sphere.center.y, aabb.min.y, aabb.max.y);
        closestPoint.z = std::clamp(sphere.center.z, aabb.min.z, aabb.max.z);

        // その最近接点と球の中心との距離をチェック
        float dx = closestPoint.x - sphere.center.x;
        float dy = closestPoint.y - sphere.center.y;
        float dz = closestPoint.z - sphere.center.z;
        float distanceSq = dx * dx + dy * dy + dz * dz;

        return distanceSq <= (sphere.radius * sphere.radius);
    }

    bool IsCollision(const Sphere& sphere, const OBB& obb) {
        // 球の中心からOBBの中心へのベクトル
        Vector3 d = {
            sphere.center.x - obb.center.x,
            sphere.center.y - obb.center.y,
            sphere.center.z - obb.center.z
        };

        // OBB上の球に最も近い点
        Vector3 closestPoint = obb.center;
        
        float extents[3] = { obb.halfExtents.x, obb.halfExtents.y, obb.halfExtents.z };
        for (int i = 0; i < 3; ++i) {
            // 球の中心とOBBの中心の差分ベクトルを、OBBの各ローカル軸に射影
            float dist = d.x * obb.orientations[i].x + d.y * obb.orientations[i].y + d.z * obb.orientations[i].z;
            
            // 射影した距離を OBB のサイズ内にクランプ
            dist = std::clamp(dist, -extents[i], extents[i]);
            
            // 最近接点にローカル軸ベクトル×距離を足し込む
            closestPoint.x += dist * obb.orientations[i].x;
            closestPoint.y += dist * obb.orientations[i].y;
            closestPoint.z += dist * obb.orientations[i].z;
        }

        // 最近接点と球の中心の距離をチェック
        float dx = closestPoint.x - sphere.center.x;
        float dy = closestPoint.y - sphere.center.y;
        float dz = closestPoint.z - sphere.center.z;
        float distanceSq = dx * dx + dy * dy + dz * dz;

        return distanceSq <= (sphere.radius * sphere.radius);
    }

    OBB CreateOBB(const Vector3& center, const Vector3& size, const Matrix4x4& rotateMatrix) {
        OBB obb;
        obb.center = center;
        obb.halfExtents = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
        // rotateMatrixがアフィン変換行列であり、左上3x3が各軸の回転（正規化済み）であることを前提とする
        // DirectX系(Row-Major)の場合は m[0] が X軸、m[1] が Y軸、m[2] が Z軸 となる
        obb.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] }; // X軸
        obb.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] }; // Y軸
        obb.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] }; // Z軸
        return obb;
    }

    bool Raycast(const Ray& ray, const Sphere& sphere, float* outDistance) {
        Vector3 m = { ray.origin.x - sphere.center.x, ray.origin.y - sphere.center.y, ray.origin.z - sphere.center.z };
        float b = m.x * ray.direction.x + m.y * ray.direction.y + m.z * ray.direction.z;
        float c = (m.x * m.x + m.y * m.y + m.z * m.z) - sphere.radius * sphere.radius;

        // 起点が球の外側にあり、レイが球から遠ざかっている場合
        if (c > 0.0f && b > 0.0f) return false;

        float discr = b * b - c;
        // 負の判別式はレイが球を外れていることを示す
        if (discr < 0.0f) return false;

        // 交差判定（最小の正の根を計算）
        if (outDistance) {
            float t = -b - std::sqrtf(discr);
            if (t < 0.0f) t = 0.0f;
            *outDistance = t;
        }
        return true;
    }

    bool Raycast(const Ray& ray, const AABB& aabb, float* outDistance) {
        float tmin = 0.0f;
        float tmax = 100000.0f; // 十分に大きな値

        // X, Y, Z軸の各スラブについて交差をテスト
        float origin[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
        float dir[3] = { ray.direction.x, ray.direction.y, ray.direction.z };
        float minVal[3] = { aabb.min.x, aabb.min.y, aabb.min.z };
        float maxVal[3] = { aabb.max.x, aabb.max.y, aabb.max.z };

        for (int i = 0; i < 3; i++) {
            if (std::abs(dir[i]) < 0.00001f) {
                // レイがスラブと平行な場合
                if (origin[i] < minVal[i] || origin[i] > maxVal[i]) return false;
            } else {
                float ood = 1.0f / dir[i];
                float t1 = (minVal[i] - origin[i]) * ood;
                float t2 = (maxVal[i] - origin[i]) * ood;
                if (t1 > t2) std::swap(t1, t2);
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) return false;
            }
        }
        if (outDistance) *outDistance = tmin;
        return true;
    }

    bool Raycast(const Ray& ray, const OBB& obb, float* outDistance) {
        float tmin = 0.0f;
        float tmax = 100000.0f;

        Vector3 p = { obb.center.x - ray.origin.x, obb.center.y - ray.origin.y, obb.center.z - ray.origin.z };
        float extents[3] = { obb.halfExtents.x, obb.halfExtents.y, obb.halfExtents.z };

        for (int i = 0; i < 3; i++) {
            Vector3 axis = obb.orientations[i];
            float e = axis.x * p.x + axis.y * p.y + axis.z * p.z;
            float f = axis.x * ray.direction.x + axis.y * ray.direction.y + axis.z * ray.direction.z;

            if (std::abs(f) > 0.00001f) {
                float t1 = (e + extents[i]) / f;
                float t2 = (e - extents[i]) / f;
                if (t1 > t2) std::swap(t1, t2);
                if (t1 > tmin) tmin = t1;
                if (t2 < tmax) tmax = t2;
                if (tmin > tmax) return false;
            } else {
                if (-e - extents[i] > 0.0f || -e + extents[i] < 0.0f) return false;
            }
        }
        if (outDistance) *outDistance = tmin;
        return true;
    }

    Vector3 ClosestPointOnTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c) {
        Vector3 ab = { b.x - a.x, b.y - a.y, b.z - a.z };
        Vector3 ac = { c.x - a.x, c.y - a.y, c.z - a.z };
        Vector3 ap = { p.x - a.x, p.y - a.y, p.z - a.z };

        float d1 = ab.x * ap.x + ab.y * ap.y + ab.z * ap.z;
        float d2 = ac.x * ap.x + ac.y * ap.y + ac.z * ap.z;
        if (d1 <= 0.0f && d2 <= 0.0f) return a;

        Vector3 bp = { p.x - b.x, p.y - b.y, p.z - b.z };
        float d3 = ab.x * bp.x + ab.y * bp.y + ab.z * bp.z;
        float d4 = ac.x * bp.x + ac.y * bp.y + ac.z * bp.z;
        if (d3 >= 0.0f && d4 <= d3) return b;

        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            float v = d1 / (d1 - d3);
            return { a.x + v * ab.x, a.y + v * ab.y, a.z + v * ab.z };
        }

        Vector3 cp = { p.x - c.x, p.y - c.y, p.z - c.z };
        float d5 = ab.x * cp.x + ab.y * cp.y + ab.z * cp.z;
        float d6 = ac.x * cp.x + ac.y * cp.y + ac.z * cp.z;
        if (d6 >= 0.0f && d5 <= d6) return c;

        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            float w = d2 / (d2 - d6);
            return { a.x + w * ac.x, a.y + w * ac.y, a.z + w * ac.z };
        }

        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            Vector3 bc = { c.x - b.x, c.y - b.y, c.z - b.z };
            return { b.x + w * bc.x, b.y + w * bc.y, b.z + w * bc.z };
        }

        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return { a.x + ab.x * v + ac.x * w, a.y + ab.y * v + ac.y * w, a.z + ab.z * v + ac.z * w };
    }

    bool IsCollision(const Sphere& sphere, const Triangle& tri, CollisionResult* outResult) {
        Vector3 closest = ClosestPointOnTriangle(sphere.center, tri.p0, tri.p1, tri.p2);
        Vector3 diff = { sphere.center.x - closest.x, sphere.center.y - closest.y, sphere.center.z - closest.z };
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distSq <= sphere.radius * sphere.radius) {
            if (outResult) {
                outResult->isHit = true;
                outResult->hitPoint = closest;
                float dist = std::sqrt(distSq);
                if (dist > 0.0001f) {
                    outResult->normal = { diff.x / dist, diff.y / dist, diff.z / dist };
                } else {
                    outResult->normal = tri.normal;
                    if (outResult->normal.x == 0.0f && outResult->normal.y == 0.0f && outResult->normal.z == 0.0f) {
                        outResult->normal = { 0.0f, 1.0f, 0.0f };
                    }
                }
                outResult->penetrationDepth = sphere.radius - dist;
            }
            return true;
        }
        return false;
    }

    AABB GetBoundingAABB(const OBB& obb) {
        float rx = obb.halfExtents.x * std::abs(obb.orientations[0].x) +
                   obb.halfExtents.y * std::abs(obb.orientations[1].x) +
                   obb.halfExtents.z * std::abs(obb.orientations[2].x);
        float ry = obb.halfExtents.x * std::abs(obb.orientations[0].y) +
                   obb.halfExtents.y * std::abs(obb.orientations[1].y) +
                   obb.halfExtents.z * std::abs(obb.orientations[2].y);
        float rz = obb.halfExtents.x * std::abs(obb.orientations[0].z) +
                   obb.halfExtents.y * std::abs(obb.orientations[1].z) +
                   obb.halfExtents.z * std::abs(obb.orientations[2].z);
        return {
            { obb.center.x - rx, obb.center.y - ry, obb.center.z - rz },
            { obb.center.x + rx, obb.center.y + ry, obb.center.z + rz }
        };
    }

    bool IsCollision(const OBB& obb, const AABB& aabb) {
        AABB bAABB = GetBoundingAABB(obb);
        return (bAABB.min.x <= aabb.max.x && bAABB.max.x >= aabb.min.x) &&
               (bAABB.min.y <= aabb.max.y && bAABB.max.y >= aabb.min.y) &&
               (bAABB.min.z <= aabb.max.z && bAABB.max.z >= aabb.min.z);
    }

    // SAT テスト用マクロ: AABB基本軸と三角形エッジの外積軸
    #define AXIS_TEST_X(e_y, e_z, fe_y, fe_z, v0_y, v0_z, v2_y, v2_z, h_y, h_z) \
        p0 = (e_z) * (v0_y) - (e_y) * (v0_z); \
        p2 = (e_z) * (v2_y) - (e_y) * (v2_z); \
        minP = (std::min)(p0, p2); \
        maxP = (std::max)(p0, p2); \
        rad = (fe_z) * (h_y) + (fe_y) * (h_z); \
        if (minP > rad || maxP < -rad) return false;

    #define AXIS_TEST_Y(e_x, e_z, fe_x, fe_z, v0_x, v0_z, v2_x, v2_z, h_x, h_z) \
        p0 = -(e_z) * (v0_x) + (e_x) * (v0_z); \
        p2 = -(e_z) * (v2_x) + (e_x) * (v2_z); \
        minP = (std::min)(p0, p2); \
        maxP = (std::max)(p0, p2); \
        rad = (fe_z) * (h_x) + (fe_x) * (h_z); \
        if (minP > rad || maxP < -rad) return false;

    #define AXIS_TEST_Z(e_x, e_y, fe_x, fe_y, v1_x, v1_y, v2_x, v2_y, h_x, h_y) \
        p1 = (e_y) * (v1_x) - (e_x) * (v1_y); \
        p2 = (e_y) * (v2_x) - (e_x) * (v2_y); \
        minP = (std::min)(p1, p2); \
        maxP = (std::max)(p1, p2); \
        rad = (fe_y) * (h_x) + (fe_x) * (h_y); \
        if (minP > rad || maxP < -rad) return false;

    bool IsCollision(const OBB& obb, const Triangle& tri, CollisionResult* outResult) {
        // 1. 三角形の3頂点をOBBローカル座標系に変換
        Vector3 d0 = { tri.p0.x - obb.center.x, tri.p0.y - obb.center.y, tri.p0.z - obb.center.z };
        Vector3 d1 = { tri.p1.x - obb.center.x, tri.p1.y - obb.center.y, tri.p1.z - obb.center.z };
        Vector3 d2 = { tri.p2.x - obb.center.x, tri.p2.y - obb.center.y, tri.p2.z - obb.center.z };

        Vector3 v0 = {
            d0.x * obb.orientations[0].x + d0.y * obb.orientations[0].y + d0.z * obb.orientations[0].z,
            d0.x * obb.orientations[1].x + d0.y * obb.orientations[1].y + d0.z * obb.orientations[1].z,
            d0.x * obb.orientations[2].x + d0.y * obb.orientations[2].y + d0.z * obb.orientations[2].z
        };
        Vector3 v1 = {
            d1.x * obb.orientations[0].x + d1.y * obb.orientations[0].y + d1.z * obb.orientations[0].z,
            d1.x * obb.orientations[1].x + d1.y * obb.orientations[1].y + d1.z * obb.orientations[1].z,
            d1.x * obb.orientations[2].x + d1.y * obb.orientations[2].y + d1.z * obb.orientations[2].z
        };
        Vector3 v2 = {
            d2.x * obb.orientations[0].x + d2.y * obb.orientations[0].y + d2.z * obb.orientations[0].z,
            d2.x * obb.orientations[1].x + d2.y * obb.orientations[1].y + d2.z * obb.orientations[1].z,
            d2.x * obb.orientations[2].x + d2.y * obb.orientations[2].y + d2.z * obb.orientations[2].z
        };

        const Vector3& h = obb.halfExtents;

        // 2. AABB 3主軸テスト
        float minX = (std::min)({ v0.x, v1.x, v2.x });
        float maxX = (std::max)({ v0.x, v1.x, v2.x });
        if (minX > h.x || maxX < -h.x) return false;

        float minY = (std::min)({ v0.y, v1.y, v2.y });
        float maxY = (std::max)({ v0.y, v1.y, v2.y });
        if (minY > h.y || maxY < -h.y) return false;

        float minZ = (std::min)({ v0.z, v1.z, v2.z });
        float maxZ = (std::max)({ v0.z, v1.z, v2.z });
        if (minZ > h.z || maxZ < -h.z) return false;

        // 3. 三角形エッジベクトル
        Vector3 e0 = { v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
        Vector3 e1 = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
        Vector3 e2 = { v0.x - v2.x, v0.y - v2.y, v0.z - v2.z };

        // 4. 三角形面法線テスト
        Vector3 normalLocal = {
            e0.y * e1.z - e0.z * e1.y,
            e0.z * e1.x - e0.x * e1.z,
            e0.x * e1.y - e0.y * e1.x
        };
        float nLen = std::sqrt(normalLocal.x * normalLocal.x + normalLocal.y * normalLocal.y + normalLocal.z * normalLocal.z);
        if (nLen < 1e-6f) return false; // 退化三角形
        normalLocal.x /= nLen;
        normalLocal.y /= nLen;
        normalLocal.z /= nLen;

        float d = v0.x * normalLocal.x + v0.y * normalLocal.y + v0.z * normalLocal.z;
        float r = h.x * std::abs(normalLocal.x) + h.y * std::abs(normalLocal.y) + h.z * std::abs(normalLocal.z);
        if (std::abs(d) > r) return false;

        // 5. 9本のクロス積軸テスト
        float fe0x = std::abs(e0.x), fe0y = std::abs(e0.y), fe0z = std::abs(e0.z);
        float fe1x = std::abs(e1.x), fe1y = std::abs(e1.y), fe1z = std::abs(e1.z);
        float fe2x = std::abs(e2.x), fe2y = std::abs(e2.y), fe2z = std::abs(e2.z);

        float p0, p1, p2, minP, maxP, rad;

        // Axis A0 x e0
        AXIS_TEST_X(e0.y, e0.z, fe0y, fe0z, v0.y, v0.z, v2.y, v2.z, h.y, h.z);
        // Axis A0 x e1
        AXIS_TEST_X(e1.y, e1.z, fe1y, fe1z, v0.y, v0.z, v1.y, v1.z, h.y, h.z);
        // Axis A0 x e2
        AXIS_TEST_X(e2.y, e2.z, fe2y, fe2z, v0.y, v0.z, v1.y, v1.z, h.y, h.z);

        // Axis A1 x e0
        AXIS_TEST_Y(e0.x, e0.z, fe0x, fe0z, v0.x, v0.z, v2.x, v2.z, h.x, h.z);
        // Axis A1 x e1
        AXIS_TEST_Y(e1.x, e1.z, fe1x, fe1z, v0.x, v0.z, v1.x, v1.z, h.x, h.z);
        // Axis A1 x e2
        AXIS_TEST_Y(e2.x, e2.z, fe2x, fe2z, v0.x, v0.z, v1.x, v1.z, h.x, h.z);

        // Axis A2 x e0
        AXIS_TEST_Z(e0.x, e0.y, fe0x, fe0y, v1.x, v1.y, v2.x, v2.y, h.x, h.y);
        // Axis A2 x e1
        AXIS_TEST_Z(e1.x, e1.y, fe1x, fe1y, v0.x, v0.y, v2.x, v2.y, h.x, h.y);
        // Axis A2 x e2
        AXIS_TEST_Z(e2.x, e2.y, fe2x, fe2y, v0.x, v0.y, v1.x, v1.y, h.x, h.y);

        // すべての分離軸をクリア ＝ 衝突！
        if (outResult) {
            outResult->isHit = true;
            outResult->hitPoint = ClosestPointOnTriangle(obb.center, tri.p0, tri.p1, tri.p2);

            // めり込み深さ: 三角形平面へのOBBの射影重なり
            float penetration = r - std::abs(d);
            outResult->penetrationDepth = (std::max)(penetration, 0.01f);

            // ワールド法線: 三角形の面法線を基準とし、OBB中心から外側へ向かう方向
            Vector3 worldNormal = tri.normal;
            float wnLen = std::sqrt(worldNormal.x * worldNormal.x + worldNormal.y * worldNormal.y + worldNormal.z * worldNormal.z);
            if (wnLen > 1e-4f) {
                worldNormal.x /= wnLen;
                worldNormal.y /= wnLen;
                worldNormal.z /= wnLen;
            } else {
                worldNormal = {
                    normalLocal.x * obb.orientations[0].x + normalLocal.y * obb.orientations[1].x + normalLocal.z * obb.orientations[2].x,
                    normalLocal.x * obb.orientations[0].y + normalLocal.y * obb.orientations[1].y + normalLocal.z * obb.orientations[2].y,
                    normalLocal.x * obb.orientations[0].z + normalLocal.y * obb.orientations[1].z + normalLocal.z * obb.orientations[2].z
                };
            }

            // OBB中心から接触点へのベクトルとの内積で向きを確認（中心から離れる向きに揃える）
            Vector3 toCenter = { obb.center.x - outResult->hitPoint.x, obb.center.y - outResult->hitPoint.y, obb.center.z - outResult->hitPoint.z };
            if (toCenter.x * worldNormal.x + toCenter.y * worldNormal.y + toCenter.z * worldNormal.z < 0.0f) {
                worldNormal.x = -worldNormal.x;
                worldNormal.y = -worldNormal.y;
                worldNormal.z = -worldNormal.z;
            }
            outResult->normal = worldNormal;
        }

        return true;
    }
}

Frustum Frustum::CreateFromViewProjection(const Matrix4x4& vp)
{
    Frustum f;

    // Row-major: 行ベクトル v' = v * VP
    // Left:   col3 + col0 -> row(i, 3) + row(i, 0)
    // Right:  col3 - col0 -> row(i, 3) - row(i, 0)
    // Bottom: col3 + col1 -> row(i, 3) + row(i, 1)
    // Top:    col3 - col1 -> row(i, 3) - row(i, 1)
    // Near:   col2        -> row(i, 2)
    // Far:    col3 - col2 -> row(i, 3) - row(i, 2)

    // 0: Left
    f.planes[0].normal.x = vp.m[0][3] + vp.m[0][0];
    f.planes[0].normal.y = vp.m[1][3] + vp.m[1][0];
    f.planes[0].normal.z = vp.m[2][3] + vp.m[2][0];
    f.planes[0].distance = vp.m[3][3] + vp.m[3][0];

    // 1: Right
    f.planes[1].normal.x = vp.m[0][3] - vp.m[0][0];
    f.planes[1].normal.y = vp.m[1][3] - vp.m[1][0];
    f.planes[1].normal.z = vp.m[2][3] - vp.m[2][0];
    f.planes[1].distance = vp.m[3][3] - vp.m[3][0];

    // 2: Bottom
    f.planes[2].normal.x = vp.m[0][3] + vp.m[0][1];
    f.planes[2].normal.y = vp.m[1][3] + vp.m[1][1];
    f.planes[2].normal.z = vp.m[2][3] + vp.m[2][1];
    f.planes[2].distance = vp.m[3][3] + vp.m[3][1];

    // 3: Top
    f.planes[3].normal.x = vp.m[0][3] - vp.m[0][1];
    f.planes[3].normal.y = vp.m[1][3] - vp.m[1][1];
    f.planes[3].normal.z = vp.m[2][3] - vp.m[2][1];
    f.planes[3].distance = vp.m[3][3] - vp.m[3][1];

    // 4: Near
    f.planes[4].normal.x = vp.m[0][2];
    f.planes[4].normal.y = vp.m[1][2];
    f.planes[4].normal.z = vp.m[2][2];
    f.planes[4].distance = vp.m[3][2];

    // 5: Far
    f.planes[5].normal.x = vp.m[0][3] - vp.m[0][2];
    f.planes[5].normal.y = vp.m[1][3] - vp.m[1][2];
    f.planes[5].normal.z = vp.m[2][3] - vp.m[2][2];
    f.planes[5].distance = vp.m[3][3] - vp.m[3][2];

    // 各平面を正規化
    for (int i = 0; i < 6; ++i)
    {
        float length = std::sqrt(
            f.planes[i].normal.x * f.planes[i].normal.x +
            f.planes[i].normal.y * f.planes[i].normal.y +
            f.planes[i].normal.z * f.planes[i].normal.z
        );
        if (length > 0.000001f)
        {
            float invLen = 1.0f / length;
            f.planes[i].normal.x *= invLen;
            f.planes[i].normal.y *= invLen;
            f.planes[i].normal.z *= invLen;
            f.planes[i].distance *= invLen;
        }
    }

    return f;
}

bool Frustum::ContainsSphere(const Vector3& center, float radius) const
{
    for (int i = 0; i < 6; ++i)
    {
        float dist = planes[i].normal.x * center.x +
                     planes[i].normal.y * center.y +
                     planes[i].normal.z * center.z +
                     planes[i].distance;
        // 平面の裏側（視錐台の外側）に完全に外れている場合
        if (dist < -radius)
        {
            return false;
        }
    }
    return true;
}
