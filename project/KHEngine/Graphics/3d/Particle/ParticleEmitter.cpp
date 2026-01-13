#include "ParticleEmitter.h"
#include <random>
#include <cmath>

std::list<Particle> ParticleEmitter::Emit(std::mt19937& engine, ParticleEffect effect)
{
	std::list<Particle> out;
	for (uint32_t i = 0; i < emitter_.count; ++i)
	{
		out.push_back(MakeNewParticle(engine, emitter_.transform.translate, effect));
	}
	return out;
}

Particle ParticleEmitter::MakeNewParticle(std::mt19937& randamEngine, const Vector3& translate, ParticleEffect effect)
{
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distUniform(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);

	Particle particle;
	particle.transform.rotation = { 0.0f, 0.0f, 0.0f };
	particle.currentTime = 0.0f;

	switch (effect)
	{
	case ParticleEffect::Fire:
	{
		float rx = distUniform(randamEngine) * 0.4f;
		float rz = distUniform(randamEngine) * 0.4f;
		particle.transform.translate = translate + Vector3{ rx, 0.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 0.5f, 1.2f + dist01(randamEngine) * 1.4f, distUniform(randamEngine) * 0.5f };
		particle.color = Vector4{ 0.9f, 0.45f + dist01(randamEngine) * 0.25f, 0.05f, 1.0f };
		particle.lifeTime = 0.6f + dist01(randamEngine) * 1.2f;
		float s = 0.2f + dist01(randamEngine) * 0.8f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Snow:
	{
		float rx = distUniform(randamEngine) * 4.0f;
		float rz = distUniform(randamEngine) * 4.0f;
		particle.transform.translate = translate + Vector3{ rx, 8.0f + dist01(randamEngine) * 2.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 0.1f, -0.2f - dist01(randamEngine) * 0.6f, distUniform(randamEngine) * 0.1f };
		particle.color = Vector4{ 0.95f, 0.95f, 1.0f, 1.0f };
		particle.lifeTime = 6.0f + dist01(randamEngine) * 6.0f;
		float s = 0.15f + dist01(randamEngine) * 0.35f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Explosion:
	{
		particle.transform.translate = translate;
		Vector3 dir = Vector3{ distUniform(randamEngine), distUniform(randamEngine), distUniform(randamEngine) };
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len < 1e-5f) dir = Vector3{ 1,1,0 }; else dir = dir * (1.0f / len);
		float speed = 8.0f + dist01(randamEngine) * 12.0f;
		particle.velocity = dir * speed;
		particle.color = Vector4{ 1.0f, 0.6f + dist01(randamEngine) * 0.4f, 0.1f, 1.0f };
		particle.lifeTime = 0.3f + dist01(randamEngine) * 0.7f;
		float s = 0.2f + dist01(randamEngine) * 0.6f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Smoke:
	{
		float rx = distUniform(randamEngine) * 0.5f;
		float rz = distUniform(randamEngine) * 0.5f;
		particle.transform.translate = translate + Vector3{ rx, 0.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 0.3f, 0.4f + dist01(randamEngine) * 0.6f, distUniform(randamEngine) * 0.3f };
		float g = 0.3f + dist01(randamEngine) * 0.25f;
		particle.color = Vector4{ g, g, g, 0.8f };
		particle.lifeTime = 2.0f + dist01(randamEngine) * 4.0f;
		float s = 0.3f + dist01(randamEngine) * 1.0f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Confetti:
	{
		float rx = distUniform(randamEngine) * 1.0f;
		float rz = distUniform(randamEngine) * 1.0f;
		particle.transform.translate = translate + Vector3{ rx, 0.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 2.0f, 1.0f + dist01(randamEngine) * 2.0f, distUniform(randamEngine) * 2.0f };
		particle.color = Vector4{ dist01(randamEngine), dist01(randamEngine), dist01(randamEngine), 1.0f };
		particle.lifeTime = 3.0f + dist01(randamEngine) * 3.0f;
		float s = 0.05f + dist01(randamEngine) * 0.2f;
		particle.transform.scale = Vector3{ s, s * (0.4f + dist01(randamEngine) * 1.6f), s };
		break;
	}
	case ParticleEffect::Wind:
	default:
	{
		particle.transform.scale = { 1.0f,1.0f,1.0f };
		particle.transform.rotation = { 0.0f, 0.0f, 0.0f };
		particle.transform.translate = { distribution(randamEngine),distribution(randamEngine),distribution(randamEngine) };
		particle.velocity = { distribution(randamEngine), distribution(randamEngine), distribution(randamEngine) };
		particle.color = { distColor(randamEngine), distColor(randamEngine), distColor(randamEngine), 1.0f };
		particle.lifeTime = distTime(randamEngine);
		break;
	}
	}

	return particle;
}