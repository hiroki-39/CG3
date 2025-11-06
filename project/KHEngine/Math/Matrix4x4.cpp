#include "Matrix4x4.h"
#include <cmath>
#include <cassert>

/* --- 基本行列 --- */

Matrix4x4 Matrix4x4::Identity()
{
    Matrix4x4 result{};
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result.m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    return result;
}

Matrix4x4 Matrix4x4::Scale(const Vector3& scale)
{
    Matrix4x4 result = Identity();
    result.m[0][0] = scale.x;
    result.m[1][1] = scale.y;
    result.m[2][2] = scale.z;
    return result;
}

Matrix4x4 Matrix4x4::Translation(const Vector3& trans)
{
    Matrix4x4 result = Identity();
    result.m[3][0] = trans.x;
    result.m[3][1] = trans.y;
    result.m[3][2] = trans.z;
    return result;
}

Matrix4x4 Matrix4x4::RotateX(float rad)
{
    Matrix4x4 result = Identity();
    float c = std::cosf(rad);
    float s = std::sinf(rad);

    result.m[1][1] = c;
    result.m[1][2] = s;
    result.m[2][1] = -s;
    result.m[2][2] = c;
    return result;
}

Matrix4x4 Matrix4x4::RotateY(float rad)
{
    Matrix4x4 result = Identity();
    float c = std::cosf(rad);
    float s = std::sinf(rad);

    result.m[0][0] = c;
    result.m[0][2] = -s;
    result.m[2][0] = s;
    result.m[2][2] = c;
    return result;
}

Matrix4x4 Matrix4x4::RotateZ(float rad)
{
    Matrix4x4 result = Identity();
    float c = std::cosf(rad);
    float s = std::sinf(rad);

    result.m[0][0] = c;
    result.m[0][1] = s;
    result.m[1][0] = -s;
    result.m[1][1] = c;
    return result;
}

/* --- 行列計算 --- */

Matrix4x4 Matrix4x4::Multiply(const Matrix4x4& a, const Matrix4x4& b)
{
    Matrix4x4 result{};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result.m[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return result;
}

Matrix4x4 Matrix4x4::Transpose(const Matrix4x4& m)
{
    Matrix4x4 result{};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result.m[i][j] = m.m[j][i];
        }
    }
    return result;
}

Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& m)
{
    Matrix4x4 result{};
    float tmp[12];
    float src[16];
    float det;

    for (int i = 0; i < 4; ++i)
    {
        src[i] = m.m[i][0];
        src[i + 4] = m.m[i][1];
        src[i + 8] = m.m[i][2];
        src[i + 12] = m.m[i][3];
    }

    tmp[0] = src[10] * src[15];
    tmp[1] = src[11] * src[14];
    tmp[2] = src[9] * src[15];
    tmp[3] = src[11] * src[13];
    tmp[4] = src[9] * src[14];
    tmp[5] = src[10] * src[13];
    tmp[6] = src[8] * src[15];
    tmp[7] = src[11] * src[12];
    tmp[8] = src[8] * src[14];
    tmp[9] = src[10] * src[12];
    tmp[10] = src[8] * src[13];
    tmp[11] = src[9] * src[12];

    result.m[0][0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]
        - tmp[1] * src[5] - tmp[2] * src[6] - tmp[5] * src[7];

    result.m[0][1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]
        - tmp[0] * src[4] - tmp[7] * src[6] - tmp[8] * src[7];

    result.m[0][2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]
        - tmp[3] * src[4] - tmp[6] * src[5] - tmp[11] * src[7];

    result.m[0][3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]
        - tmp[4] * src[4] - tmp[9] * src[5] - tmp[10] * src[6];

    det = src[0] * result.m[0][0] + src[1] * result.m[0][1] + src[2] * result.m[0][2] + src[3] * result.m[0][3];

    if (det == 0.0f)
        return Identity();

    det = 1.0f / det;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result.m[i][j] *= det;
        }
    }

    return result;
}

/* --- 投影・変換関連 --- */

Matrix4x4 Matrix4x4::Perspective(float fovY, float aspect, float nearZ, float farZ)
{
    Matrix4x4 result{};
    float f = 1.0f / std::tanf(fovY / 2.0f);

    result.m[0][0] = f / aspect;
    result.m[1][1] = f;
    result.m[2][2] = farZ / (farZ - nearZ);
    result.m[2][3] = 1.0f;
    result.m[3][2] = (-nearZ * farZ) / (farZ - nearZ);
    return result;
}

Matrix4x4 Matrix4x4::Orthographic(float left, float top, float right, float bottom, float nearZ, float farZ)
{
    Matrix4x4 result{};
    result.m[0][0] = 2.0f / (right - left);
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[2][2] = 1.0f / (farZ - nearZ);
    result.m[3][0] = -(right + left) / (right - left);
    result.m[3][1] = -(top + bottom) / (top - bottom);
    result.m[3][2] = -nearZ / (farZ - nearZ);
    result.m[3][3] = 1.0f;
    return result;
}

Matrix4x4 Matrix4x4::Viewport(float left, float top, float width, float height, float minDepth, float maxDepth)
{
    Matrix4x4 result{};
    result.m[0][0] = width / 2.0f;
    result.m[1][1] = -height / 2.0f;
    result.m[2][2] = maxDepth - minDepth;
    result.m[3][0] = left + width / 2.0f;
    result.m[3][1] = top + height / 2.0f;
    result.m[3][2] = minDepth;
    result.m[3][3] = 1.0f;
    return result;
}
