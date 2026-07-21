#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Input/Input.h"
#include "Game/Actor/Bullet/PlayerBullet.h"
#include <memory>
#include <list>

class Enemy;
class ParticleEmitter;

class Player {
public:
    /// <summary>
    /// 初期化
    void Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex);

    void LoadSettings(const std::string& filepath);
    void SaveSettings(const std::string& filepath);
    void DrawUI();

    /// <summary>
    /// 譖ｴ譁ｰ
    /// </summary>
    void Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera = nullptr, float gameSpeed = 1.0f);

    /// <summary>
    /// 謠冗判
    /// </summary>
    void Draw();

    void DrawCollider();
    
    Object3d* GetColliderObject() const { return colliderObject_.get(); }

    void Update3DObjectOnly() {
        if (object_) {
            object_->SetScale(playerScale_);
            object_->SetRotation(Vector3(
                baseRotation_.x + modelRotOffset_.x,
                baseRotation_.y + modelRotOffset_.y,
                baseRotation_.z + modelRotOffset_.z
            ));
            object_->SetTranslate(Vector3(
                logicalPosition_.x + modelPosOffset_.x, 
                logicalPosition_.y + modelPosOffset_.y, 
                logicalPosition_.z + modelPosOffset_.z
            ));
            object_->Update();
            
            if (colliderObject_) {
                const Matrix4x4& wMat = object_->GetmatWorld();
                colliderObject_->SetTranslate({ wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] });
                colliderObject_->Update();
            }
        }
        if (accessory_) accessory_->Update();
        if (reticle_) reticle_->Update();
        if (frontReticle_) frontReticle_->Update();
    }

    // Getter
    const Vector3& GetTranslate() const { return logicalPosition_; }
    const Vector3& GetColliderSize() const { return colliderSize_; }
    void SetTranslate(const Vector3& translate) { logicalPosition_ = translate; }
    void SetRotation(const Vector3& rotation) { baseRotation_ = rotation; }
    Object3d* GetObject3d() const { return object_.get(); }
    Object3d* GetReticle() const { return reticle_.get(); }
    Object3d* GetFrontReticle() const { return frontReticle_.get(); }
    
    // 辣ｧ貅・繝ｬ繝・ぅ繧ｯ繝ｫ)髢｢騾｣
    void SetReticleColor(const Vector4& color);
    const Vector4& GetReticleColor() const { return reticleColor_; }
    Vector3 GetReticleWorldPosition() const;

    void SetLockOn(bool isLockOn, const Vector3& targetPos = { 0, 0, 0 }, Enemy* targetEnemy = nullptr) {
        isLockOn_ = isLockOn;
        lockOnTargetPos_ = targetPos;
        lockOnTargetEnemy_ = targetEnemy;
    }

    bool IsBoosting() const { return isBoosting_; }

    // 回避判定用フラグ消費
    bool ConsumeDodgeTrigger() {
        if (isDodgeTriggered_) {
            isDodgeTriggered_ = false;
            return true;
        }
        return false;
    }

    void OnCollision();
    bool IsDead() const { return isDead_; }
    int GetHp() const { return hp_; }
    int GetMaxHp() const { return maxHp_; }

    // 照準アシスト
    void SetAssistTarget(Enemy* enemy) { assistTarget_ = enemy; }

    // 翼端トレイル用のヘルパーメソッド
    bool IsBanking() const;
    Vector3 GetLeftWingPosition() const;
    Vector3 GetRightWingPosition() const;

private:
    /// <summary>
    /// 遘ｻ蜍募・逅・    /// </summary>
    void Move(float gameSpeed);

    /// <summary>
    /// 謾ｻ謦・・逅・    /// </summary>
    void Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera, float gameSpeed);

private:
    std::unique_ptr<Object3d> object_ = nullptr;
    std::unique_ptr<Object3d> colliderObject_ = nullptr;
    std::unique_ptr<Object3d> reticle_ = nullptr; // 奥の照準
    std::unique_ptr<Object3d> frontReticle_ = nullptr; // 手前の照準
    std::unique_ptr<Object3d> accessory_ = nullptr; // アクセサリ
    Object3dCommon* object3dCommon_ = nullptr; // 蠑ｾ逕滓・逕ｨ
    Input* input_ = nullptr;
    
    uint32_t skyboxTexIndex_ = 0; // 蜀榊茜逕ｨ縺吶ｋ縺溘ａ菫晄戟

    Vector3 reticlePosition_ = { 0.0f, 0.0f, 40.0f }; // 照準の座標
    Vector4 reticleColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 照準の色

    // プレイヤー設定パラメータ
    float speed_ = 0.3f;
    float reticleSpeed_ = 0.5f;
    float moveLimitX_ = 25.0f;     // 照準のX移動限界（自機限界と合わせる）
    float moveLimitY_ = 12.0f;     // 照準のY移動限界（自機限界と合わせる）
    float attackInterval_ = 15.0f;
    float rollMaxTime_ = 15.0f;
    float playerLimitX_ = 25.0f;   // プレイヤーの実際のX移動限界
    float playerLimitYMin_ = -5.0f;// 下への移動限界
    float playerLimitYMax_ = 12.0f;// 上への移動限界（カメラ内に収まるよう狭める）
    float followSpeed_ = 0.08f;
    float bulletSpeed_ = 3.0f;

    // 見た目
    std::string modelName_ = "cube.obj";
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool reflection_ = false;
    Vector3 modelPosOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 modelRotOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 playerScale_ = { 0.5f, 0.5f, 0.5f };
    Vector3 colliderSize_ = { 4.0f, 4.0f, 4.0f };

    // 現在の傾き
    float currentPitch_ = 0.0f;
    float currentYaw_ = 0.0f;
    float currentBank_ = 0.0f;

    // 隲也炊菴咲ｽｮ繝ｻ蝗櫁ｻ｢ (繧ｫ繝｡繝ｩ縺ｮ蟄舌が繝悶ず繧ｧ繧ｯ繝医→縺励※縺ｮ繝ｭ繝ｼ繧ｫ繝ｫ蠎ｧ讓吶↓縺ｪ繧・
    Vector3 logicalPosition_ = { 0.0f, -0.0f, 20.0f }; // 位置
    Vector3 baseRotation_ = { 0.0f, 0.0f, 0.0f };

    // 攻撃関連
    float attackTimer_ = 0.0f;

    // XY軸の速度（補間用）
    Vector2 velocity_ = { 0.0f, 0.0f };

    // ローリング・回避関連
    bool isRolling_ = false;
    float rollTimer_ = 0.0f;
    float rollDirection_ = 0.0f; // -1.0f (左), 1.0f (右)
    
    // Q/E ダブルタップ判定用
    float lastQPressTime_ = 0.0f;
    float lastEPressTime_ = 0.0f;
    const float doubleTapThreshold_ = 15.0f; // 15フレーム以内の連続入力でダブルタップと判定

    // 繝ｭ繝・け繧ｪ繝ｳ髢｢騾｣
    bool isLockOn_ = false;
    Vector3 lockOnTargetPos_ = { 0, 0, 0 };
    Enemy* lockOnTargetEnemy_ = nullptr;
    
    // 照準アシスト関連
    Enemy* assistTarget_ = nullptr;

    // ブースト
    bool isBoosting_ = false;

    // 蝗樣∩繝医Μ繧ｬ繝ｼ
    bool isDodgeTriggered_ = false;

    // HP・被弾関連
    int hp_ = 10000;
    int maxHp_ = 10000;
    float invincibilityTimer_ = 0.0f;
    bool isDead_ = false;
};
