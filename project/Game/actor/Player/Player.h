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
    void Update(std::list<std::unique_ptr<PlayerBullet>>& bullets);

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
    }

    // Getter
    const Vector3& GetTranslate() const { return logicalPosition_; }
    void SetTranslate(const Vector3& translate) { logicalPosition_ = translate; }
    void SetRotation(const Vector3& rotation) { baseRotation_ = rotation; }
    Object3d* GetObject3d() const { return object_.get(); }
    Object3d* GetReticle() const { return reticle_.get(); }

private:
    /// <summary>
    /// 移動処理
    /// </summary>
    void Move();

    /// <summary>
    /// 攻撃処理
    /// </summary>
    void Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets);

private:
    std::unique_ptr<Object3d> object_ = nullptr;
    std::unique_ptr<Object3d> reticle_ = nullptr; // 照準用モデル
    std::unique_ptr<Object3d> accessory_ = nullptr; // 親子関係のデモ用アクセサリ
    Object3dCommon* object3dCommon_ = nullptr; // 弾生成用
    Input* input_ = nullptr;
    
    uint32_t skyboxTexIndex_ = 0; // 再利用するため保持

    Vector3 reticlePosition_ = { 0.0f, 0.0f, 40.0f }; // 照準のワールド座標

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

    // 論理位置・回転
    Vector3 logicalPosition_ = { 0.0f, 1.0f, -4.0f };
    Vector3 baseRotation_ = { 0.0f, 0.0f, 0.0f };

    // 攻撃関連
    int attackTimer_ = 0;

    // ローリング回避関連
    bool isRolling_ = false;
    int rollTimer_ = 0;
    float rollDirection_ = 0.0f; // -1.0f (左), 1.0f (右)
};
