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
    /// </summary>
    void Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex);

    /// <summary>
    /// 更新
    /// </summary>
    void Update(std::list<std::unique_ptr<PlayerBullet>>& bullets);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    // Getter
    const Vector3& GetTranslate() const { return object_->GetTranslate(); }
    void SetTranslate(const Vector3& translate) { object_->SetTranslate(translate); }
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
    Object3dCommon* object3dCommon_ = nullptr; // 弾生成用
    Input* input_ = nullptr;

    Vector3 reticlePosition_ = { 0.0f, 0.0f, 40.0f }; // 照準のワールド座標

    // 移動速度
    float speed_ = 0.3f;
    float reticleSpeed_ = 0.5f; // 照準の移動速度
    
    // 移動制限範囲
    const float kMoveLimitX = 15.0f;
    const float kMoveLimitY = 10.0f;

    // 攻撃関連
    int attackTimer_ = 0;
    const int kAttackInterval = 15; // 発射間隔（フレーム数）

    // ローリング回避関連
    bool isRolling_ = false;
    int rollTimer_ = 0;
    const int kRollMaxTime = 15; // 30から15に減らして回転速度を倍速に
    float rollDirection_ = 0.0f; // -1.0f (左), 1.0f (右)
};
