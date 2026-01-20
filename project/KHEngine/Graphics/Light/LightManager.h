#pragma once
#include "KHEngine/Graphics/Light/Light.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace KHEngine::Graphics::LightSystem
{

	class LightManager
	{
	public:
		LightManager() = default;

		// Directional を作るユーティリティ
		DirectionalLightPtr CreateDirectionalLight()
		{
			auto p = std::make_shared<DirectionalLight>();
			lights.push_back(p);
			return p;
		}

		PointLightPtr CreatePointLight()
		{
			auto p = std::make_shared<PointLight>();
			lights.push_back(p);
			return p;
		}

		SpotLightPtr CreateSpotLight()
		{
			auto p = std::make_shared<SpotLight>();
			lights.push_back(p);
			return p;
		}

		void Remove(const LightPtr& p)
		{
			lights.erase(std::remove(lights.begin(), lights.end(), p), lights.end());
		}

		void Clear() { lights.clear(); }

		const std::vector<LightPtr>& All() const { return lights; }

		// 便利：最初の Directional を返す（なければ nullptr）
		DirectionalLightPtr GetFirstDirectional() const
		{
			for (auto& l : lights)
			{
				if (l && l->type == Light::Type::Directional)
				{
					return std::static_pointer_cast<DirectionalLight>(l);
				}
			}
			return nullptr;
		}

	private:
		std::vector<LightPtr> lights;
	};

}