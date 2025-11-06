#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

class Transform
{
public:
    // 位置
    Vector3 position;
	// 回転
    Vector3 rotation;
	// スケール
    Vector3 scale;

	// コンストラクタ
    Transform() : position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) {}
    
	// コンストラクタ（引数付き）
    Transform(const Vector3& pos, const Vector3& rot, const Vector3& scl)
        : position(pos), rotation(rot), scale(scl)
    {
    }

    /// <summary>
	/// ワールド行列を取得 
    /// </summary>
	/// <returns>ワールド行列</returns>
    Matrix4x4 GetWorldMatrix() const
    {
        Matrix4x4 s = Matrix4x4::Scale(scale);
        Matrix4x4 rx = Matrix4x4::RotateX(rotation.x);
        Matrix4x4 ry = Matrix4x4::RotateY(rotation.y);
        Matrix4x4 rz = Matrix4x4::RotateZ(rotation.z);
        Matrix4x4 t = Matrix4x4::Translation(position);

        Matrix4x4 world = Matrix4x4::Multiply(s, rx);
        world = Matrix4x4::Multiply(world, ry);
        world = Matrix4x4::Multiply(world, rz);
        world = Matrix4x4::Multiply(world, t);
        return world;
    }
};
