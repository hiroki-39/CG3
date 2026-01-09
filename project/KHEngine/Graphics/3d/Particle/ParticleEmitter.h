#pragma once
#include "KHEngine/Math/MathCommon.h"
#include "Particle.h"
#include <random>
#include <list>

class ParticleEmitter
{
public:
	ParticleEmitter() = default;

	// 初期エミッタ設定を直接操作して使う
	Emitter& GetEmitter() { return emitter_; }

	// 指定した乱数エンジンで count 個の粒子を生成して返す
	std::list<Particle> Emit(std::mt19937& engine, ParticleEffect effect);

private:
	Particle MakeNewParticle(std::mt19937& engine, const Vector3& translate, ParticleEffect effect);

	Emitter emitter_;
};

