#pragma once
#include "KHEngine/Math/MathCommon.h"
#include <list>
#include <cstdint>

struct Particle
{
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU
{
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

struct Emitter
{
	Transform transform; // エミッターの位置情報
	uint32_t count = 1; // 生成するパーティクルの数
	float frequency = 1.0f; // 生成頻度（秒）
	float frequencyTime = 0.0f; // 周期用タイムカウンタ
};

struct AABB
{
	Vector3 min;
	Vector3 max;
};

struct AccelerationField
{
	Vector3 accleration; // 加速度
	AABB area; // 範囲
};

enum class ParticleEffect
{
	Wind = 0,
	Fire,
	Snow,
	Explosion,
	Smoke,
	Confetti,
	Count
};

// ユーティリティ
inline bool IsCollision(const AABB& aabb, const Vector3& point)
{
	if (point.x < aabb.min.x || point.x > aabb.max.x) return false;
	if (point.y < aabb.min.y || point.y > aabb.max.y) return false;
	if (point.z < aabb.min.z || point.z > aabb.max.z) return false;
	return true;
}

