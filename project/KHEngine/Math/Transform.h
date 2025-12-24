#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

class Transform
{
public:
    // 位置
    Vector3 translate;
	// 回転
    Vector3 rotation;
	// スケール
    Vector3 scale;

	// コンストラクタ
    Transform() : translate(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) {}
    
	// コンストラクタ（引数付き）
    Transform(const Vector3& pos, const Vector3& rot, const Vector3& scl)
        : translate(pos), rotation(rot), scale(scl)
    {
    }

    /// <summary>
	/// ワールド行列を取得 
    /// </summary>
	/// <returns>ワールド行列</returns>
    Matrix4x4 GetWorldMatrix() const
    {
        Matrix4x4 scaleMatrix = Matrix4x4::Scale(scale);
        Matrix4x4 rotateXMatrix = Matrix4x4::RotateX(rotation.x);
        Matrix4x4 rotateYMatrix = Matrix4x4::RotateY(rotation.y);
        Matrix4x4 rotateZMatrix = Matrix4x4::RotateZ(rotation.z);
        Matrix4x4 translationMatrix = Matrix4x4::Translation(translate);

        Matrix4x4 world = Matrix4x4::Multiply(scaleMatrix, rotateXMatrix);
        world = Matrix4x4::Multiply(world, rotateYMatrix);
        world = Matrix4x4::Multiply(world, rotateZMatrix);
        world = Matrix4x4::Multiply(world, translationMatrix);
        return world;
    }
};
