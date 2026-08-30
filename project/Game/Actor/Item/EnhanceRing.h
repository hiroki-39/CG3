#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include <memory>
#include <string>

class Player;

enum class RingType {
    POWER_UP,
    HEAL
};

class EnhanceRing {
public:
    void Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const Vector3& rotation, const std::string& fileName, RingType type, uint32_t skyboxTexIndex);
    
    void Update();
    void Draw();

    void StartShrink();
    bool IsDead() const { return isDead_; }

    // プレイヤーがくぐったかどうかを判定する
    bool CheckPassThrough(Player* player);

    RingType GetType() const { return type_; }
    const Vector3& GetPosition() const { return position_; }

private:
    std::unique_ptr<Object3d> object_;
    Vector3 position_;
    Vector3 rotation_;
    Vector3 baseScale_;
    RingType type_;

    bool isDead_ = false;
    bool isShrinking_ = false;
    float shrinkScale_ = 1.0f;
    float radius_ = 1.0f; // くぐり判定用の半径
};
