#include "ParticleManager.h"
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
    asset.vertices.reserve(6);

    // 四角形ポリゴンの構築
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

    renderer.Initialize(dxCommon, srvManager, maxInstances);

    const auto& verts = it->second.vertices;
    if (!verts.empty())
    {
        renderer.CreateVertexBuffer(verts.data(), static_cast<uint32_t>(verts.size()), static_cast<uint32_t>(sizeof(ParticleVertex)));
    }

    Material initialMaterial{};
    initialMaterial.color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
    initialMaterial.enableLightingAsInt = 0;
    initialMaterial.selectLightings = 2;
    initialMaterial.uvTransform = Matrix4x4::Identity();

    renderer.CreateMaterialBuffer(sizeof(Material), &initialMaterial);
}

ParticleEmitter& ParticleManager::CreateEmitter()
{
    emitters_.push_back(std::make_unique<ParticleEmitter>());
    return *emitters_.back();
}

void ParticleManager::Update(float dt)
{
    for (auto& emitter : emitters_)
    {
        emitter->Update(dt);
    }
}