#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>
#include <string>
#include "KHEngine/Scene/LevelLoader.h"
#include "KHEngine/Math/CollisionMath.h"

class Obstacle {
public:
    void Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const Vector3& rotation, const std::string& fileName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo, bool isDestructible = true);
    
    
    void Update();
    

    
    void Update3DObjectOnly() {
        if (object_) object_->Update();
        if (colliderObject_) colliderObject_->Update();
    }
    
    void Draw();
    void DrawCollider(); 
    void OnCollision(); 
    void Kill(); 

    void SetSpawnProgress(float progress) { spawnProgress_ = progress; }
    void SetTexturePath(const std::string& path);
    void SetIsRing(bool isRing) { isRing_ = isRing; }
    bool GetIsRing() const { return isRing_; }

    
    const Vector3& GetPosition() const { return position_; }
    const LevelCollider& GetCollider() const { return collider_; }
    bool IsDead() const { return isDead_; }

    
    bool CheckCollision(const Sphere& bulletSphere) const;
    bool CheckRaycast(const Ray& ray, float* outDist) const;

private:
    std::unique_ptr<Object3d> object_;
    std::unique_ptr<Object3d> colliderObject_; 
    Vector3 position_;
    Vector3 rotation_;
    LevelCollider collider_; 
    bool isDead_ = false;
    bool isVisible_ = true;
    bool isDestructible_ = true;
    bool isRing_ = false;
    
    float spawnProgress_ = 0.0f;
    std::string texturePath_;
    
    
};

