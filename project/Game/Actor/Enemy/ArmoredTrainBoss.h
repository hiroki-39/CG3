#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Scene/LevelLoader.h"
#include "KHEngine/Math/CollisionMath.h"
#include "Game/System/Rail.h"
#include <memory>
#include <vector>
#include <list>
#include <string>

class Player;
class EnemyBullet;

/**
 * @brief 装甲列車の個別車両データ
 */
struct TrainCarriage {
    std::string name;             // 車両名（例: "Locomotive", "TurretCar", "MissileCar", "GeneratorCar"）
    std::string displayName;      // 表示名（例: "先頭装甲機関車", "旋回砲塔車", "ミサイルコンテナ車", "動力発電車"）
    std::string modelName;        // モデル
    Vector3 scale = { 4.0f, 3.2f, 10.0f };
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float lengthOffset = 0.0f;    // 先頭車両からの連結距離オフセット (負のZ値)
    
    int hp = 40;
    int maxHp = 40;
    bool isDestroyed = false;
    float attackTimer = 0.0f;
    float attackInterval = 90.0f;

    // 当たり判定
    LevelCollider collider;
    std::unique_ptr<Object3d> object;
    std::unique_ptr<Object3d> colliderObject;
};

/**
 * @brief 装甲列車（Armored Train）中ボス
 * レール上を連結走行し、多数の武装（砲塔、ミサイル、主砲）でプレイヤーを迎え撃つ巨大ボス
 */
class ArmoredTrainBoss {
public:
    ArmoredTrainBoss() = default;
    ~ArmoredTrainBoss() = default;

    void Initialize(Object3dCommon* object3dCommon, const Vector3& startPos, uint32_t skyboxTexIndex);
    void Update(const Vector3& cameraPos, Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets, float gameSpeed = 1.0f);
    void Draw();
    void DrawCollider();

    // 被弾判定
    bool CheckCollision(const Sphere& bulletSphere, int* outCarriageIndex = nullptr);
    bool CheckRaycast(const Ray& ray, float* outDist, int* outCarriageIndex = nullptr);
    void OnDamaged(int carriageIndex, int damage = 1);

    // 進行レール設定
    void SetRail(const Rail* rail) { rail_ = rail; }
    void SetRailProgress(float progress) { railProgress_ = progress; }

    // 状態
    bool IsActive() const { return isActive_; }
    void SetActive(bool active) { isActive_ = active; }
    bool IsDefeated() const { return isDefeated_; }
    float GetTotalHpRate() const;
    const Vector3& GetPosition() const { return position_; }
    const std::vector<TrainCarriage>& GetCarriages() const { return carriages_; }

private:
    void UpdateCarriageTransforms();
    void ExecuteAttacks(Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets, float gameSpeed);

private:
    Object3dCommon* object3dCommon_ = nullptr;
    uint32_t skyboxTexIndex_ = 0;

    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
    float speed_ = 0.4f;

    const Rail* rail_ = nullptr;
    float railProgress_ = 0.0f;

    bool isActive_ = false;
    bool isDefeated_ = false;
    bool isCoreExposed_ = false; // 全武装破壊でメインコア露出

    std::vector<TrainCarriage> carriages_;
    float defeatTimer_ = 0.0f;
    float defeatExplosionTimer_ = 0.0f;
};
