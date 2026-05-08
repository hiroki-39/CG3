#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/3d/Particle/ParticleEmitter.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Math/MathCommon.h"
#include <map>
#include <memory>
#include <string>
#include <vector>


class ParticleManager {
public:
  static ParticleManager *GetInstance();

  // 初期化関連
  void RegisterQuad(const std::string &name,
                    const std::string &textureFilePath);
  void RegisterRing(const std::string &name, const std::string &textureFilePath,
                    uint32_t division = 32, float innerRadius = 0.5f,
                    float outerRadius = 1.0f);
  void RegisterCylinder(const std::string &name, const std::string &textureFilePath,
                        uint32_t division = 32, float topRadius = 1.0f,
                        float bottomRadius = 1.0f, float height = 3.0f);
  void SetupRendererFromAsset(ParticleRenderer &renderer,
                              const std::string &name, DirectXCommon *dxCommon,
                              SrvManager *srvManager, uint32_t maxInstances);

  // マテリアルの更新（UVスクロールや色変更などに使用）
  void UpdateMaterial(ParticleRenderer &renderer, const Vector4& color, const Matrix4x4& uvTransform);

  // エミッター（旧システム）の生成管理
  ParticleEmitter &CreateEmitter();

  // 全体の更新（一括で更新したい場合に使用可能）
  void Update(float dt);

  // ゲッター
  std::vector<std::unique_ptr<ParticleEmitter>> &GetEmitters() {
    return emitters_;
  }

private:
  ParticleManager() = default;
  ~ParticleManager() = default;

  struct ParticleVertex {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
  };

  struct ParticleAsset {
    std::vector<ParticleVertex> vertices;
    std::string textureFilePath;
  };

  struct Material {
    Vector4 color;
    int32_t selectLightings;
    int32_t enableLightingAsInt;
    float padding;
    float alphaReference;
    Matrix4x4 uvTransform;
  };

  std::map<std::string, ParticleAsset> assets_;
  std::vector<std::unique_ptr<ParticleEmitter>> emitters_;

  static ParticleManager *instance_;
};
