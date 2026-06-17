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
}
