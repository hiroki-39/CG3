#include "ParticleSystem.h"
#include <algorithm>

ParticleSystem::ParticleSystem()
{
	std::random_device rd;
	rnd_.seed(rd());
}

void ParticleSystem::AddInitialParticles(std::mt19937& engine, uint32_t count)
{
	// 指定数分のパーティクルを生成して追加
	for (uint32_t i = 0; i < count; ++i)
	{
		auto list = emitter_.Emit(engine, effect_);
		particles_.splice(particles_.end(), list);
	}
}

void ParticleSystem::Update(float dt)
{
	// 更新のみ（呼ばれるタイミングは外部で制御可）
	for (auto it = particles_.begin(); it != particles_.end(); )
	{
		Particle& p = *it;
		// ライフ切れ
		if (p.currentTime >= p.lifeTime)
		{
			it = particles_.erase(it);
			continue;
		}

		// Wind 効果で加速度を適用
		if (effect_ == ParticleEffect::Wind)
		{
			if (IsCollision(accel_.area, p.transform.translate))
			{
				p.velocity += accel_.accleration * dt;
			}
		}

		// 位置の更新
		p.transform.translate += p.velocity * dt;
		p.currentTime += dt;

		++it;
	}

	// エミッタによる生成
	emitter_.GetEmitter().frequencyTime += dt;
	if (emitter_.GetEmitter().frequency > 0.0f &&
		emitter_.GetEmitter().frequency <= emitter_.GetEmitter().frequencyTime)
	{
		auto newParticles = emitter_.Emit(rnd_, effect_);
		particles_.splice(particles_.end(), newParticles);
		emitter_.GetEmitter().frequencyTime -= emitter_.GetEmitter().frequency;
	}
}

uint32_t ParticleSystem::FillInstancingBuffer(ParticleForGPU* outBuffer, uint32_t maxInstances,
	const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Matrix4x4& billboardMatrix, bool doUpdate)
{
	uint32_t numInstance = 0;

	// 必要なら update も行う（main ループ側で Update を分けたい場合は false にする）
	if (doUpdate) Update(1.0f / 60.0f);

	for (auto it = particles_.begin(); it != particles_.end() && numInstance < maxInstances; ++it)
	{
		const Particle& p = *it;

		Matrix4x4 scalematrix = Matrix4x4::Scale(p.transform.scale);
		Matrix4x4 translatematrix = Matrix4x4::Translation(p.transform.translate);

		Matrix4x4 worldMatrix = scalematrix * billboardMatrix * translatematrix;
		Matrix4x4 worldViewProjectionMatrix = worldMatrix * (viewMatrix * projectionMatrix);

		outBuffer[numInstance].WVP = worldViewProjectionMatrix;
		outBuffer[numInstance].World = worldMatrix;

		// アルファは寿命から計算
		float alpha = 1.0f - (p.currentTime / p.lifeTime);
		outBuffer[numInstance].color = p.color;
		outBuffer[numInstance].color.w = alpha;

		++numInstance;
	}

	return numInstance;
}
