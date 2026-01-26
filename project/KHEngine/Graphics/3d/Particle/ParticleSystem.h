#pragma once
#include "Particle.h"
#include "ParticleEmitter.h"
#include <list>
#include <random>

class ParticleSystem
{
public:
	static constexpr uint32_t kMaxInstances = 100;

	ParticleSystem();

	// システム固有設定
	ParticleEmitter& GetEmitter() { return emitter_; }
	void SetEffect(ParticleEffect e) { effect_ = e; }
	void SetAccelerationField(const AccelerationField& f) { accel_ = f; }
	void SetUseBillboard(bool b) { useBillboard_ = b; }

	// 初期パーティクルを追加
	void AddInitialParticles(std::mt19937& engine, uint32_t count);

	// 更新（CPU側）
	void Update(float dt);

	// GPU用のインスタンシング配列へ書き込む
	uint32_t FillInstancingBuffer(ParticleForGPU* outBuffer, uint32_t maxInstances,
		const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Matrix4x4& billboardMatrix, bool doUpdate = false);

private:

	std::list<Particle> particles_;
	ParticleEmitter emitter_;
	AccelerationField accel_;
	ParticleEffect effect_ = ParticleEffect::Wind;
	
	// ビルボード使用フラグ
	bool useBillboard_ = true;

	// 内部乱数
	std::mt19937 rnd_;
};

