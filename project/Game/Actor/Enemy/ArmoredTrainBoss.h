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
    Vector3 baseScale = { 4.0f, 3.2f, 10.0f };
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float lengthOffset = 0.0f;        // 先頭車両からの連結距離オフセット (負のZ値)
    float baseLengthOffset = 0.0f;
    
    int hp = 40;
    int maxHp = 40;
    bool isDestroyed = false;
    float attackTimer = 0.0f;
    float attackInterval = 90.0f;

    // 個別のワールド座標と回転（傾斜・カーブ姿勢）
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation = { 0.0f, 0.0f, 0.0f };

    // 当たり判定
    LevelCollider collider;
    Vector3 baseColliderCenter = { 0.0f, 0.0f, 0.0f };
    Vector3 baseColliderSize = { 1.0f, 1.0f, 1.0f };
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
    void Update(const Vector3& cameraPos, const Vector3& playerForward, float playerSpeed, Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets, float gameSpeed = 1.0f);
    void Draw();
    void DrawCollider();

    // 被弾判定
    bool CheckCollision(const Sphere& bulletSphere, int* outCarriageIndex = nullptr);
    bool CheckRaycast(const Ray& ray, float* outDist, int* outCarriageIndex = nullptr);
    void OnDamaged(int carriageIndex, int damage = 1);

    // 進行レール設定
    void SetRail(const Rail* rail) { rail_ = rail; }
    void SetRailProgress(float progress) { railProgress_ = progress; }
    float GetRailProgress() const { return railProgress_; }

    // スケール倍率設定
    void SetScaleMultiplier(float scale);
    float GetScaleMultiplier() const { return scaleMultiplier_; }

    // プレイヤー連動チェイス設定
    void SetFollowPlayer(bool follow) { isFollowPlayer_ = follow; }
    bool IsFollowPlayer() const { return isFollowPlayer_; }
    void SetDesiredLeadDistance(float dist) { desiredLeadDistance_ = dist; targetDynamicLeadDistance_ = dist; }
    float GetDesiredLeadDistance() const { return desiredLeadDistance_; }
    float GetDynamicLeadDistance() const { return dynamicLeadDistance_; }
    void SetRandomLeadDistanceEnabled(bool enabled) { isRandomLeadEnabled_ = enabled; }
    bool IsRandomLeadDistanceEnabled() const { return isRandomLeadEnabled_; }
    bool IsShortDistanceOpportunity() const { return isShortDistanceOpportunity_; }
    void SetRushSpeedBonus(float speed) { rushSpeedBonus_ = speed; }
    float GetRushSpeedBonus() const { return rushSpeedBonus_; }
    float GetCurrentSpeed() const { return currentSpeed_; }
    float GetTargetSpeed() const { return targetSpeed_; }
    float GetRelativeForwardDistance() const { return relativeForwardDist_; }
    bool IsCloseToPlayer() const { return isCloseToPlayer_; }
    bool IsOvertaking() const { return isOvertaking_; }
    void ResetChaseState();

    // 状態
    bool IsActive() const { return isActive_; }
    void SetActive(bool active) { isActive_ = active; }
    bool IsDefeated() const { return isDefeated_; }
    float GetTotalHpRate() const;
    const Vector3& GetPosition() const { return position_; }
    const std::vector<TrainCarriage>& GetCarriages() const { return carriages_; }

    // 各車両部位のワールド座標と破壊判定（マルチロックオン用）
    Vector3 GetCarriagePosition(int carriageIndex) const {
        if (carriageIndex >= 0 && carriageIndex < static_cast<int>(carriages_.size())) {
            return carriages_[carriageIndex].position;
        }
        return position_;
    }
    bool IsCarriageDestroyed(int carriageIndex) const {
        if (carriageIndex >= 0 && carriageIndex < static_cast<int>(carriages_.size())) {
            return carriages_[carriageIndex].isDestroyed;
        }
        return true;
    }

    // 地面吸着（レイキャスト自動接地）設定
    void SetTerrainObjects(const std::vector<std::unique_ptr<Object3d>>* terrainObjects) { terrainObjects_ = terrainObjects; }
    void SetGroundSnapEnabled(bool enable) { isGroundSnapEnabled_ = enable; }
    bool IsGroundSnapEnabled() const { return isGroundSnapEnabled_; }
    void SetGroundSnapOffset(float offset) { groundSnapOffset_ = offset; }
    float GetGroundSnapOffset() const { return groundSnapOffset_; }

private:
    bool GetGroundHeight(float x, float z, float startY, float* outGroundY, Vector3* outNormal = nullptr) const;
    void UpdateCarriageTransforms();
    void ExecuteAttacks(Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets, float gameSpeed);

private:
    Object3dCommon* object3dCommon_ = nullptr;
    uint32_t skyboxTexIndex_ = 0;

    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
    const Rail* rail_ = nullptr;
    float railProgress_ = 0.0f;

    // チェイス走行制御
    bool isFollowPlayer_ = true;        // プレイヤー連動モード
    float currentSpeed_ = 50.0f;        // ボスの現在の実速度 (m/s)
    float targetSpeed_ = 50.0f;         // 目標速度 (m/s)
    float desiredLeadDistance_ = 120.0f; // 基準となるプレイヤー前方の並走距離 (m) (標準120m)
    float dynamicLeadDistance_ = 120.0f; // 現在滑らかに補間中の目標並走距離 (m)
    float targetDynamicLeadDistance_ = 120.0f; // ランダム周期で設定される目標並走距離 (m)
    bool isRandomLeadEnabled_ = true;   // ランダム並走距離短縮の有効化
    bool isShortDistanceOpportunity_ = false; // 現在接近攻撃チャンス中か
    float leadDistanceChangeTimer_ = 0.0f; // 次の距離変更までのタイマー (秒)
    float rushSpeedBonus_ = 45.0f;      // 接近・追い抜き時の追加速度 (m/s)
    float relativeForwardDist_ = 0.0f;  // プレイヤーから見た前後の相対距離 (m)
    bool isCloseToPlayer_ = false;      // プレイヤーの近場に到達したか
    bool isOvertaking_ = false;         // プレイヤーを追い抜き中か

    bool isActive_ = false;
    bool isDefeated_ = false;
    bool isCoreExposed_ = false; // 全武装破壊でメインコア露出

    float scaleMultiplier_ = 1.0f; // ボス全体スケール倍率

    // 地面吸着（レイキャスト）用メンバ
    const std::vector<std::unique_ptr<Object3d>>* terrainObjects_ = nullptr;
    bool isGroundSnapEnabled_ = true;
    float groundSnapOffset_ = 0.0f; // 車体底面からの微調整オフセット

    std::vector<TrainCarriage> carriages_;
    float defeatTimer_ = 0.0f;
    float defeatExplosionTimer_ = 0.0f;
};
