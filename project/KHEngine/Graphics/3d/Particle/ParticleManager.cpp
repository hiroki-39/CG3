#include "ParticleManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include <cassert>

ParticleManager* ParticleManager::instance_ = nullptr;

ParticleManager* ParticleManager::GetInstance()
{
	if (instance_ == nullptr) instance_ = new ParticleManager();
	return instance_;
}

void ParticleManager::RegisterQuad(const std::string& name, const std::string& textureFilePath)
{
	ParticleAsset asset;

	// 左上、右上、左下, 右上、右下、左下の順
	asset.vertices.reserve(6);

	asset.vertices.push_back({ Vector4{ -1.0f,  1.0f, 0.0f, 1.0f }, Vector2{0.0f, 0.0f}, Vector3{0.0f,0.0f,1.0f} }); // 左上
	asset.vertices.push_back({ Vector4{  1.0f,  1.0f, 0.0f, 1.0f }, Vector2{1.0f, 0.0f}, Vector3{0.0f,0.0f,1.0f} }); // 右上
	asset.vertices.push_back({ Vector4{ -1.0f, -1.0f, 0.0f, 1.0f }, Vector2{0.0f, 1.0f}, Vector3{0.0f,0.0f,1.0f} }); // 左下

	asset.vertices.push_back({ Vector4{  1.0f,  1.0f, 0.0f, 1.0f }, Vector2{1.0f, 0.0f}, Vector3{0.0f,0.0f,1.0f} }); // 右上
	asset.vertices.push_back({ Vector4{  1.0f, -1.0f, 0.0f, 1.0f }, Vector2{1.0f, 1.0f}, Vector3{0.0f,0.0f,1.0f} }); // 右下
	asset.vertices.push_back({ Vector4{ -1.0f, -1.0f, 0.0f, 1.0f }, Vector2{0.0f, 1.0f}, Vector3{0.0f,0.0f,1.0f} }); // 左下

	asset.textureFilePath = textureFilePath;

	assets_[name] = std::move(asset);
}

void ParticleManager::SetupRendererFromAsset(ParticleRenderer& renderer, const std::string& name,
	DirectXCommon* dxCommon, SrvManager* srvManager, uint32_t maxInstances)
{
	assert(dxCommon != nullptr);
	assert(srvManager != nullptr);

	auto it = assets_.find(name);
	assert(it != assets_.end()); 

	// 初期化（RootSignature/PSO/Instancing バッファ等）
	renderer.Initialize(dxCommon, srvManager, maxInstances);

	// 頂点バッファ作成
	const auto& verts = it->second.vertices;
	if (!verts.empty())
	{
		renderer.CreateVertexBuffer(verts.data(), static_cast<uint32_t>(verts.size()), static_cast<uint32_t>(sizeof(ParticleVertex)));
	}

	// マテリアルの初期値を作成して CBV を生成
	Material initialMaterial{};
	initialMaterial.color = Vector4{ 1.0f,1.0f,1.0f,1.0f };
	initialMaterial.enableLightingAsInt = 0;
	initialMaterial.selectLightings = 2;
	initialMaterial.uvTransform = Matrix4x4::Identity();

	renderer.CreateMaterialBuffer(sizeof(Material), &initialMaterial);
}

ParticleSystem& ParticleManager::CreateSystem()
{
	systems_.emplace_back();
	return systems_.back();
}