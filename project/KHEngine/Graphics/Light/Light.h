#pragma once
#include "KHEngine/Math/MathCommon.h"
#include <memory>
#include <string>

namespace KHEngine::Graphics::LightSystem
{

	// 基底ライト（データのみ）
	class Light
	{
	public:
		enum class Type { Directional, Point, Spot };
		Light(Type t) : type(t), enabled(true), color{ 1.0f,1.0f,1.0f,1.0f }, intensity(1.0f) {}
		virtual ~Light() = default;

		Type type;
		bool enabled;
		Vector4 color;
		float intensity;
		std::string name;
	};

	class DirectionalLight : public Light
	{
	public:
		DirectionalLight() : Light(Type::Directional), direction{ 0.0f,-1.0f,0.0f } {}
		Vector3 direction;
	};

	class PointLight : public Light
	{
	public:
		PointLight() : Light(Type::Point), position{ 0,0,0 }, range(10.0f), constant(1.0f), linear(0.09f), quadratic(0.032f) {}
		Vector3 position;
		float range;
		float constant, linear, quadratic;
	};

	class SpotLight : public Light
	{
	public:
		SpotLight() : Light(Type::Spot), position{ 0,0,0 }, direction{ 0,-1,0 }, innerCutoffDeg(12.5f), outerCutoffDeg(17.5f),
			constant(1.0f), linear(0.09f), quadratic(0.032f)
		{
		}
		Vector3 position;
		Vector3 direction;
		float innerCutoffDeg;
		float outerCutoffDeg;
		float constant, linear, quadratic;
	};

	using LightPtr = std::shared_ptr<Light>;
	using DirectionalLightPtr = std::shared_ptr<DirectionalLight>;
	using PointLightPtr = std::shared_ptr<PointLight>;
	using SpotLightPtr = std::shared_ptr<SpotLight>;

}