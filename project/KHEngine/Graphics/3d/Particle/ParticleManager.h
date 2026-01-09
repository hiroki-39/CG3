#pragma once
#include "ParticleSystem.h"
#include <vector>
#include <map>
#include <string>
#include "KHEngine/Math/MathCommon.h"
#include "ParticleRenderer.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"

class ParticleManager
{
public:
	ParticleManager() = default;

	// シングルトン取得
	static ParticleManager* GetInstance();

	// 四角を登録
	void RegisterQuad(const std::string& name, const std::string& textureFilePath);

	// 登録済みアセットを使って ParticleRenderer を初期化／頂点／マテリアルバッファを作成する
	void SetupRendererFromAsset(ParticleRenderer& renderer, const std::string& name,
		DirectXCommon* dxCommon, SrvManager* srvManager, uint32_t maxInstances);

	// 既存の ParticleSystem 管理 API（変更なし）
	ParticleSystem& CreateSystem();
	std::vector<ParticleSystem>& GetSystems() { return systems_; }

private:

	struct ParticleVertex
	{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	struct ParticleAsset
	{
		std::vector<ParticleVertex> vertices;
		std::string textureFilePath;
	};

	struct Material
	{
		Vector4 color;
		int32_t selectLightings;
		int32_t enableLightingAsInt;
		float padding[2];
		Matrix4x4 uvTransform;
	};

	std::map<std::string, ParticleAsset> assets_;

	// 既存のシステム管理
	std::vector<ParticleSystem> systems_;

	// シングルトンインスタンス
	static ParticleManager* instance_;
};

