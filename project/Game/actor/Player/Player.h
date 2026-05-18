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
    void Initialize(Object3dCommon* object3dCommon);

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
    Object3dCommon* object3dCommon_ = nullptr; // 弾生成用
    Input* input_ = nullptr;

    // 移動速度
    float speed_ = 0.3f;
    
    // 移動制限範囲
    const float kMoveLimitX = 15.0f;
    const float kMoveLimitY = 10.0f;
};
