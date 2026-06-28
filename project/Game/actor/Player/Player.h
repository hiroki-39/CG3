#pragma once
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Input/Input.h"
#include "Game/actor/Bullet/PlayerBullet.h"
#include <memory>
#include <list>

class Player {
public:
    /// <summary>
    /// 初期化
    void Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex);

    void LoadSettings(const std::string& filepath);
    void SaveSettings(const std::string& filepath);
    void DrawUI();

    /// <summary>
    /// 更新
    /// </summary>
    void Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera = nullptr);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

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
        }
        if (accessory_) accessory_->Update();
        if (reticle_) reticle_->Update();
        if (frontReticle_) frontReticle_->Update();
    }

    // Getter
    const Vector3& GetTranslate() const { return logicalPosition_; }
    void SetTranslate(const Vector3& translate) { logicalPosition_ = translate; }
    void SetRotation(const Vector3& rotation) { baseRotation_ = rotation; }
    Object3d* GetObject3d() const { return object_.get(); }
    Object3d* GetReticle() const { return reticle_.get(); }
    Object3d* GetFrontReticle() const { return frontReticle_.get(); }
    
    // 照準(レティクル)関連
    void SetReticleColor(const Vector4& color);
    const Vector4& GetReticleColor() const { return reticleColor_; }
    Vector3 GetReticleWorldPosition() const;

    void SetLockOn(bool isLockOn, const Vector3& targetPos = { 0, 0, 0 }) {
        isLockOn_ = isLockOn;
        lockOnTargetPos_ = targetPos;
    }

    bool IsBoosting() const { return isBoosting_; }

    // 回避が発生した瞬間にtrueを返し、同時にフラグを下ろす（消費型）
    bool ConsumeDodgeTrigger() {
        if (isDodgeTriggered_) {
            isDodgeTriggered_ = false;
            return true;
        }
        return false;
    }

private:
    /// <summary>
    /// 移動処理
    /// </summary>
    void Move();

    /// <summary>
    /// 攻撃処理
    /// </summary>
    void Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera);

private:
    std::unique_ptr<Object3d> object_ = nullptr;
    std::unique_ptr<Object3d> reticle_ = nullptr; // 奥の照準（小）
    std::unique_ptr<Object3d> frontReticle_ = nullptr; // 手前の照準（大）
    std::unique_ptr<Object3d> accessory_ = nullptr; // 親子関係のデモ用アクセサリ
    Object3dCommon* object3dCommon_ = nullptr; // 弾生成用
    Input* input_ = nullptr;
    
    uint32_t skyboxTexIndex_ = 0; // 再利用するため保持

    Vector3 reticlePosition_ = { 0.0f, 0.0f, 40.0f }; // 照準のローカル座標
    Vector4 reticleColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 照準の色

    // プレイヤー設定パラメータ
    float speed_ = 0.3f;
    float reticleSpeed_ = 0.5f;
    float moveLimitX_ = 15.0f;
    float moveLimitY_ = 10.0f;
    int attackInterval_ = 15;
    int rollMaxTime_ = 15;
    float playerLimitX_ = 4.0f;
    float playerLimitYMin_ = 2.0f;
    float playerLimitYMax_ = 8.0f;
    float followSpeed_ = 0.08f;
    float bulletSpeed_ = 3.0f;

    // 見た目関連の設定
    std::string modelName_ = "cube.obj";
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool reflection_ = false;
    Vector3 modelPosOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 modelRotOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 playerScale_ = { 0.5f, 0.5f, 0.5f };

    // 現在の傾き（補間用）
    float currentPitch_ = 0.0f;
    float currentYaw_ = 0.0f;
    float currentBank_ = 0.0f;

    // 論理位置・回転 (カメラの子オブジェクトとしてのローカル座標になる)
    Vector3 logicalPosition_ = { 0.0f, -0.0f, 20.0f }; // カメラから20前方のやや下
    Vector3 baseRotation_ = { 0.0f, 0.0f, 0.0f };

    // 攻撃関連
    int attackTimer_ = 0;

    // ローリング回避関連
    bool isRolling_ = false;
    int rollTimer_ = 0;
    float rollDirection_ = 0.0f; // -1.0f (左), 1.0f (右)

    // ロックオン関連
    bool isLockOn_ = false;
    Vector3 lockOnTargetPos_ = {0.0f, 0.0f, 0.0f};

    // ブースト状態
    bool isBoosting_ = false;

    // 回避トリガー
    bool isDodgeTriggered_ = false;
};
