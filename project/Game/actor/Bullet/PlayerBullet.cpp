#include "PlayerBullet.h"
#include "Game/Actor/Enemy/Enemy.h"
#include <cmath>

void PlayerBullet::Initialize(Object3dCommon* object3dCommon, const Vector3& position, const Vector3& velocity, Object3d* parent, Enemy* targetEnemy) {
    velocity_ = velocity;
    targetEnemy_ = targetEnemy;
    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("cube.obj");
    object_->GetModel()->SetColor({ 0.5f, 1.0f, 0.0f, 1.0f }); // 蠑ｾ繧帝ｻ・ｷ題牡縺ｫ縺吶ｋ
    object_->SetTranslate(position);
    object_->SetScale({ 4.0f, 4.0f, 4.0f }); // 隕九ｄ縺吶＞繧医≧縺ｫ繝｢繝・Ν繧貞､ｧ縺阪￥縺吶ｋ
    // 蠑ｾ縺ｯ繝ｯ繝ｼ繝ｫ繝牙ｺｧ讓咏ｳｻ縺ｧ鬟帙・縺吶◆繧√∬ｦｪ(繧ｫ繝｡繝ｩ)縺ｯ險ｭ螳壹＠縺ｪ縺・
    // if (parent) {
    //     object_->SetParent(parent);
    // }

    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_sphere.obj"); // 蠑ｾ縺ｮ蠖薙◆繧雁愛螳壹→蜷後§逅・
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 襍､濶ｲ
    colliderObject_->SetTranslate(position);
    colliderObject_->SetScale({ 4.0f, 4.0f, 4.0f }); // 蠖薙◆繧雁愛螳壹ｒ螟ｧ縺阪￥縺吶ｋ
}

void PlayerBullet::Update() {
    Vector3 pos = object_->GetTranslate();

    // 繝帙・繝溘Φ繧ｰ蜃ｦ逅・
    if (targetEnemy_ && !targetEnemy_->IsDead()) {
        Vector3 targetPos = targetEnemy_->GetPosition();
        Vector3 toTarget = {
            targetPos.x - pos.x,
            targetPos.y - pos.y,
            targetPos.z - pos.z
        };

        // 繧ｿ繝ｼ繧ｲ繝・ヨ縺ｸ縺ｮ譁ｹ蜷代ｒ豁｣隕丞喧
        float length = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        if (length > 0.0f) {
            toTarget.x /= length;
            toTarget.y /= length;
            toTarget.z /= length;
        }

        // 迴ｾ蝨ｨ縺ｮ騾溷ｺｦ繝吶け繝医Ν縺ｮ髟ｷ縺包ｼ医せ繝斐・繝会ｼ峨ｒ蜿門ｾ・
        float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);

        // 迴ｾ蝨ｨ縺ｮ騾溷ｺｦ繝吶け繝医Ν繧貞ｰ代＠縺壹▽繧ｿ繝ｼ繧ｲ繝・ヨ譁ｹ蜷代∈蜷代￠繧具ｼ医・繝ｼ繝溘Φ繧ｰ縺ｮ蠑ｷ縺包ｼ・.1f 遞句ｺｦ・・
        float homingStrength = 0.15f;
        velocity_.x += (toTarget.x * speed - velocity_.x) * homingStrength;
        velocity_.y += (toTarget.y * speed - velocity_.y) * homingStrength;
        velocity_.z += (toTarget.z * speed - velocity_.z) * homingStrength;

        // 蜀阪・髟ｷ縺輔ｒspeed縺ｫ蜷医ｏ縺帙ｋ・磯溷ｺｦ縺悟､峨ｏ繧峨↑縺・ｈ縺・↓縺吶ｋ・・
        float newLength = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
        if (newLength > 0.0f) {
            velocity_.x = (velocity_.x / newLength) * speed;
            velocity_.y = (velocity_.y / newLength) * speed;
            velocity_.z = (velocity_.z / newLength) * speed;
        }
    }

    // 騾溷ｺｦ繝吶け繝医Ν縺ｫ蠕薙▲縺ｦ遘ｻ蜍・
    pos.x += velocity_.x;
    pos.y += velocity_.y;
    pos.z += velocity_.z;
    object_->SetTranslate(pos);

    // 蟇ｿ蜻ｽ
    if (--deathTimer_ <= 0) {
        isDead_ = true;
    }

    object_->Update();
    if (colliderObject_) {
        colliderObject_->SetTranslate(pos);
        colliderObject_->Update();
    }
}

void PlayerBullet::Draw() {
    if (object_) {
        object_->Draw();
    }
}

void PlayerBullet::DrawCollider() {
    if (colliderObject_) {
        colliderObject_->Draw();
    }
}
