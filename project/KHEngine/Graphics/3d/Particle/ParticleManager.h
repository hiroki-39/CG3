#pragma once
#include "ParticleEmitter.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include "KHEngine/Math/MathCommon.h"
#include "ParticleRenderer.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"

/**
 * @brief パーティクル全体を管理するシングルトンクラス
 */
class ParticleManager
{
public:
    static ParticleManager* GetInstance();

    // 初期化関連
    void RegisterQuad(const std::string& name, const std::string& textureFilePath);
    void SetupRendererFromAsset(ParticleRenderer& renderer, const std::string& name,
        DirectXCommon* dxCommon, SrvManager* srvManager, uint32_t maxInstances);

    // エミッター（旧システム）の生成管理
    ParticleEmitter& CreateEmitter();
    
    // 全体の更新（一括で更新したい場合に使用可能）
    void Update(float dt);

    // ゲッター
    std::vector<std::unique_ptr<ParticleEmitter>>& GetEmitters() { return emitters_; }

private:
    ParticleManager() = default;
    ~ParticleManager() = default;

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
    std::vector<std::unique_ptr<ParticleEmitter>> emitters_;

    static ParticleManager* instance_;
};
