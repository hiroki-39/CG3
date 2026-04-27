#pragma once
#include "Particle.h"
#include <list>
#include <random>
#include <string>
#include <memory>

/**
 * @brief パーティクルの生成・更新・管理を行うクラス
 * 従来の ParticleEmitter と ParticleSystem を統合し、シンプルにしました。
 */
class ParticleEmitter
{
public:
    static constexpr uint32_t kMaxInstances = 1024;

    ParticleEmitter();

    // 更新処理
    void Update(float dt);

    // インスタンシング用バッファの構築
    uint32_t FillInstancingBuffer(ParticleForGPU* outBuffer, uint32_t maxInstances,
        const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Matrix4x4& billboardMatrix);

    // 設定
    void SetParameter(const ParticleEmitterParameter& param) { parameter_ = param; }
    const ParticleEmitterParameter& GetParameter() const { return parameter_; }

    void SetTextureName(const std::string& name) { textureName_ = name; }
    const std::string& GetTextureName() const { return textureName_; }

    void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
    BlendMode GetBlendMode() const { return blendMode_; }

    void SetPosition(const Vector3& pos) { position_ = pos; }
    void SetUseBillboard(bool use) { useBillboard_ = use; }
    
    // フィールド設定
    void SetAccelerationField(const AccelerationField& field) { accelField_ = field; }

    // 直接パーティクルを追加（バースト生成など）
    void Emit(uint32_t count);

    // --- プリセット（エフェクトの Primitive） ---
    
    // 炎
    static ParticleEmitterParameter CreateFirePreset();
    // 雪
    static ParticleEmitterParameter CreateSnowPreset();
    // 爆発
    static ParticleEmitterParameter CreateExplosionPreset();
    // スラッシュ（画像のような細長いヒットエフェクト）
    static ParticleEmitterParameter CreateSlashPreset();
    // リング
    static ParticleEmitterParameter CreateRingPreset();
    // 互換性用（Slash と同じ）
    static ParticleEmitterParameter CreateCirclePreset() { return CreateSlashPreset(); }

private:
    // 単体生成
    Particle MakeNewParticle();

    // メンバ変数
    std::list<Particle> particles_;
    ParticleEmitterParameter parameter_;
    AccelerationField accelField_;
    
    Vector3 position_ = { 0, 0, 0 };
    std::string textureName_ = "white";
    BlendMode blendMode_ = BlendMode::Alpha;
    bool useBillboard_ = true;

    float frequencyTimer_ = 0.0f;
    std::mt19937 randomEngine_;
};
