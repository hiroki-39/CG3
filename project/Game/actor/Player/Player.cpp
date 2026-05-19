#include "Player.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include <algorithm>

void Player::Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex) {
    object3dCommon_ = object3dCommon;
    input_ = EngineServices::GetInstance()->GetInput();

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel("cube.obj");
    object_->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);
    object_->SetEnvironmentCoefficient(1.0f);
    object_->SetTranslate(Vector3(0.0f, 1.0f, -4.0f));
    object_->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
    object_->SetScale(Vector3(1.0f, 1.0f, 1.0f));

    // 照準の初期化
    reticle_ = std::make_unique<Object3d>();
    reticle_->Initialize(object3dCommon);
    reticle_->SetModel("plane.obj"); // 照準モデルとして使用
    reticle_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
    reticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    reticle_->SetEnvironmentCoefficient(0.0f);
    reticle_->SetScale(Vector3(0.5f, 0.5f, 0.5f));
    
    // 照準の初期位置（プレイヤーの奥）
    reticlePosition_ = { 0.0f, 1.0f, 20.0f }; 

    // 初期状態としてグレースケールをOFFにしておく
    if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
        pp->SetEffectActive("Grayscale", false);
    }
}

void Player::Update(std::list<std::unique_ptr<PlayerBullet>>& bullets) {
    Move();
    Attack(bullets);

    object_->Update();
}

void Player::Draw() {
        if (reticle_) {
        reticle_->Draw();
    }
        if (object_) {
        object_->Draw();
    }

}

void Player::Move() {
    if (!input_) return;

    // --- ローリング回避の開始判定 ---
    if (!isRolling_) {
        if (input_->TriggerKey(DIK_Q)) {
            isRolling_ = true;
            rollTimer_ = 0;
            rollDirection_ = -1.0f; // 左
            if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
                pp->SetEffectActive("Grayscale", true);
            }
        } else if (input_->TriggerKey(DIK_E)) {
            isRolling_ = true;
            rollTimer_ = 0;
            rollDirection_ = 1.0f; // 右
            if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
                pp->SetEffectActive("Grayscale", true);
            }
        }
    }

    if (isRolling_) {
        rollTimer_++;
        // ローリング中はレティクルを強制的に移動させる
        float rollMoveSpeed = 0.2f; // フレーム減少に合わせて少しだけ調整
        reticlePosition_.x += rollDirection_ * rollMoveSpeed;

        // 一定時間でローリング終了
        if (rollTimer_ >= kRollMaxTime) {
            isRolling_ = false;
            if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
                pp->SetEffectActive("Grayscale", false);
            }
        }
    }

    // --- 照準（レティクル）の移動 ---
    if (input_->PushKey(DIK_W) || input_->PushKey(DIK_UP)) {
        reticlePosition_.y += reticleSpeed_;
    }
    if (input_->PushKey(DIK_S) || input_->PushKey(DIK_DOWN)) {
        reticlePosition_.y -= reticleSpeed_;
    }
    if (input_->PushKey(DIK_A) || input_->PushKey(DIK_LEFT)) {
        reticlePosition_.x -= reticleSpeed_;
    }
    if (input_->PushKey(DIK_D) || input_->PushKey(DIK_RIGHT)) {
        reticlePosition_.x += reticleSpeed_;
    }

    // 移動制限
    reticlePosition_.x = std::clamp(reticlePosition_.x, -kMoveLimitX, kMoveLimitX);
    reticlePosition_.y = std::clamp(reticlePosition_.y, -kMoveLimitY, kMoveLimitY);

    Vector3 playerPos = object_->GetTranslate();

    // 照準のZ座標はプレイヤーより一定距離奥に保つ
    reticlePosition_.z = playerPos.z + 40.0f;
    
    reticle_->SetTranslate(reticlePosition_);
    reticle_->Update();

    // --- プレイヤーの追従（Panzer Dragoon風） ---
    // プレイヤーは照準のXY座標に向かって少し遅れて追従する
    float followSpeed = 0.08f;
    playerPos.x += (reticlePosition_.x - playerPos.x) * followSpeed;
    playerPos.y += (reticlePosition_.y - playerPos.y) * followSpeed;

    // プレイヤーがカメラ外に出ないように制限（大幅に狭めました）
    const float kPlayerLimitX = 4.0f;      // 左右の制限
    const float kPlayerLimitYMin = 2.0f;   // 下方向の制限
    const float kPlayerLimitYMax = 8.0f;   // 上方向の制限
    playerPos.x = std::clamp(playerPos.x, -kPlayerLimitX, kPlayerLimitX);
    playerPos.y = std::clamp(playerPos.y, kPlayerLimitYMin, kPlayerLimitYMax);

    // 移動時の体の傾き計算（Panzer Dragoon風）
    // 上下移動時のピッチ角（機首の上下）
    float pitchAngle = (reticlePosition_.y - playerPos.y) * -0.1f; // 符号を反転・係数を大きくして上を向くように
    pitchAngle = std::clamp(pitchAngle, -0.6f, 0.6f);

    // 左右移動時のヨー角（機首の左右）
    float yawAngle = (reticlePosition_.x - playerPos.x) * 0.05f;
    yawAngle = std::clamp(yawAngle, -0.4f, 0.4f);

    // 左右移動時のバンク角（機体のロール）
    float bankAngle = (reticlePosition_.x - playerPos.x) * -0.1f;
    bankAngle = std::clamp(bankAngle, -0.8f, 0.8f);

    // ローリング中なら360度回転を追加
    if (isRolling_) {
        float rollAngle = (static_cast<float>(rollTimer_) / kRollMaxTime) * 3.14159265f * 2.0f;
        bankAngle += rollAngle * -rollDirection_; // 左回避ならプラス回転、右回避ならマイナス回転
    }
    
    object_->SetRotation(Vector3(pitchAngle, yawAngle, bankAngle));
    object_->SetTranslate(playerPos);
}

void Player::Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets) {
    if (!input_) return;

    // 発射間隔のタイマー更新
    if (attackTimer_ > 0) {
        attackTimer_--;
    }

    // タイマーが0以下なら発射可能
    // スペースキーまたはマウス左クリックで発射
    if (attackTimer_ <= 0 && (input_->PushKey(DIK_SPACE) || input_->PushMouseButton(0))) {
        attackTimer_ = kAttackInterval; // タイマーリセット
        Vector3 playerPos = object_->GetTranslate();
        Vector3 targetPos = reticlePosition_;
        
        // 照準に向かうベクトルを計算
        Vector3 direction = {
            targetPos.x - playerPos.x,
            targetPos.y - playerPos.y,
            targetPos.z - playerPos.z
        };
        
        // 正規化
        float length = std::sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (length > 0.0f) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        }
        
        float bulletSpeed = 3.0f; // 弾の速度
        Vector3 velocity = {
            direction.x * bulletSpeed,
            direction.y * bulletSpeed,
            direction.z * bulletSpeed
        };

        // 弾を生成
        std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerBullet>();
        newBullet->Initialize(object3dCommon_, playerPos, velocity);
        bullets.push_back(std::move(newBullet));
    }
}
