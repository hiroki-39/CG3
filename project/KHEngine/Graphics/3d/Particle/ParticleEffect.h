#pragma once
#include "KHEngine/Graphics/3d/Particle/ParticleEmitter.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include <vector>
#include <memory>
#include <string>

/**
 * @brief 複数のパーティクル（Plane, Ring, Cylinder）を組み合わせて
 * 1つの複雑なエフェクトを作るためのクラス
 */
class ParticleEffect {
public:
    struct Node {
        char name[64] = "Node";
        ParticleRenderer renderer;
        ParticleEmitter emitter;
        ParticleForGPU* instancingData = nullptr;
        uint32_t numInstance = 0;
        
        int shapeType = 0; // 0: Quad(Plane), 1: Ring, 2: Cylinder
        
        // アニメーション設定
        float uvScrollSpeed = 0.0f;
        float uvScrollOffset = 0.0f;
        bool enableColorAnim = false;
        float colorAnimSpeed = 2.0f;
        float colorTimer = 0.0f;
        Vector4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        Vector4 startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        Vector4 endColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    ParticleEffect() = default;
    ~ParticleEffect() = default;

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void Update(float dt, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Matrix4x4& billboardMatrix);
    void Draw();
    
    // ImGuiによるエディタ描画
    void DrawImGui();

    // ノード（エフェクトの層）を追加
    void AddNode(const std::string& name, int shapeType);
    
    // すべてのノードで放出（バースト）
    void Play();

private:
    void SetupRendererForNode(Node* node);

    std::vector<std::unique_ptr<Node>> nodes_;
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
    uint32_t maxInstances_ = 1000;
};
