#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>
#include <string>
#include "KHEngine/Scene/LevelLoader.h"
#include "KHEngine/Math/CollisionMath.h"

class Obstacle {
public:
    void Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const Vector3& rotation, const std::string& fileName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo);
    
    // 今後の拡張（落ちてくる岩、崩れる柱などのアニメーション）用にUpdateを用意しておく
    void Update();
    
    void Update3DObjectOnly() {
        if (object_) object_->Update();
        if (colliderObject_) colliderObject_->Update();
    }
    
    void Draw();
    void DrawCollider(); // デバッグ描画用
    void OnCollision(); // 弾などが当たった時の処理

    void SetSpawnProgress(float progress) { spawnProgress_ = progress; }
    void SetTexturePath(const std::string& path);

    // Getter
    const Vector3& GetPosition() const { return position_; }
    const LevelCollider& GetCollider() const { return collider_; }
    bool IsDead() const { return isDead_; }

    // 衝突判定用メソッド
    bool CheckCollision(const Sphere& bulletSphere) const;
    bool CheckRaycast(const Ray& ray, float* outDist) const;

private:
    std::unique_ptr<Object3d> object_;
    std::unique_ptr<Object3d> colliderObject_; // デバッグ描画用オブジェクト
    Vector3 position_;
    Vector3 rotation_;
    LevelCollider collider_; // コライダー情報
    bool isDead_ = false;
    bool isVisible_ = true;
    bool isDestructible_ = true;
    
    float spawnProgress_ = 0.0f;
    std::string texturePath_;
    
    // アニメーション用変数などが必要になればここに追加する
};
