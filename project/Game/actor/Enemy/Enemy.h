#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>
#include <string>
#include "KHEngine/Scene/LevelLoader.h"
#include "KHEngine/Math/CollisionMath.h"
#include "Game/System/Rail.h"

class Enemy {
public:
    void Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const std::string& typeName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo);
    void Update(const Vector3& playerPos, const Vector3& playerForward);
    void Update3DObjectOnly() {
        if (object_) object_->Update();
        if (colliderObject_) colliderObject_->Update();
    }
    void Draw();
    void DrawCollider(); // デバッグ描画用
    void OnCollision(); // 弾が当たった時の処理

    void SetMovePath(std::unique_ptr<Rail> path);

    void SetSpawnProgress(float progress) { spawnProgress_ = progress; }
    void SetTexturePath(const std::string& path) { texturePath_ = path; }

    // Getter / Setter
    const Vector3& GetPosition() const { return position_; }
    const LevelCollider& GetCollider() const { return collider_; }
    bool IsDead() const { return isDead_; }
    const Rail* GetMovePath() const { return movePath_.get(); }

    // 衝突判定用メソッド
    bool CheckCollision(const Sphere& bulletSphere) const;
    bool CheckRaycast(const Ray& ray, float* outDist) const;

private:
    std::unique_ptr<Object3d> object_;
    std::unique_ptr<Object3d> colliderObject_; // デバッグ描画用オブジェクト
    Vector3 position_;
    Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    LevelCollider collider_; // コライダー情報
    int hp_ = 3;
    bool isDead_ = false;
    std::string typeName_;

    // パス移動用
    std::unique_ptr<Rail> movePath_;
    float pathProgress_ = 0.0f;

    bool isActive_ = false;
    float spawnProgress_ = 0.0f;
    std::string texturePath_;
    bool isAutoAI_ = false;
    Vector3 aiOffset_ = {0.0f, 0.0f, 0.0f};
};
