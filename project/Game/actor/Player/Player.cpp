#include "Player.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "externals/nlohmann/json.hpp"
#include <fstream>
#include <algorithm>
#include "Game/Actor/Enemy/Enemy.h"

void Player::Initialize(Object3dCommon* object3dCommon, uint32_t skyboxTexIndex) {
    object3dCommon_ = object3dCommon;
    input_ = EngineServices::GetInstance()->GetInput();

    skyboxTexIndex_ = skyboxTexIndex;

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName_);
    object_->GetModel()->SetColor(color_);
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);
    object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
    object_->SetScale(playerScale_);
     
    //// アクセサリの初期化
    //accessory_ = std::make_unique<Object3d>();
    //accessory_->Initialize(object3dCommon);
    //accessory_->SetModel("suzanne.obj");
    //accessory_->GetModel()->SetColor({0.8f, 0.8f, 0.0f, 1.0f});
    //accessory_->SetEnvironmentTextureIndex(skyboxTexIndex);
    //accessory_->SetEnvironmentCoefficient(0.0f);
    //accessory_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    //accessory_->SetTranslate(Vector3(1.5f, 1.0f, 0.0f)); // プレイヤーの右上に配置
    //accessory_->SetParent(object_.get()); // 親子関係を設定

    // 奥の照準（小）の初期化
    reticle_ = std::make_unique<Object3d>();
    reticle_->Initialize(object3dCommon);
    reticle_->SetModel("crossHair.obj");
    reticle_->GetModel()->SetColor(reticleColor_);
    reticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    reticle_->SetEnvironmentCoefficient(0.0f);
    reticle_->SetEnableLighting(false);
    reticle_->SetSelectLightings(0);
    reticle_->SetScale(Vector3(1.6f, 1.6f, 1.6f));
    
    // 手前の照準（大）の初期化
    frontReticle_ = std::make_unique<Object3d>();
    frontReticle_->Initialize(object3dCommon);
    frontReticle_->SetModel("crossHair.obj");
    // 手前の照準は少し透明度を下げて邪魔にならないようにする
    Vector4 frontColor = reticleColor_;
    frontColor.w = 1.0f;
    frontReticle_->GetModel()->SetColor(frontColor);
    frontReticle_->SetEnvironmentTextureIndex(skyboxTexIndex);
    frontReticle_->SetEnvironmentCoefficient(0.0f);
    frontReticle_->SetEnableLighting(false);
    frontReticle_->SetSelectLightings(0);
    frontReticle_->SetScale(Vector3(1.7f, 1.7f, 1.7f)); 
    
    // 照準の初期位置（カメラの奥）
    reticlePosition_ = { 0.0f, 0.0f, 40.0f }; 

    // 初期状態としてグレースケールをOFFにしておく
    if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
        pp->SetEffectActive("Grayscale", false);
    }
    // コライダー可視化用オブジェクト
    ModelManager::GetInstance()->LoadModel("collider_cube.obj");
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_cube.obj"); // 立方体のモデル
    colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 1.0f, 1.0f }); // 水色
    colliderObject_->SetEnvironmentCoefficient(0.0f);
    colliderObject_->SetEnableLighting(false);
    colliderObject_->SetScale({ 4.0f, 4.0f, 4.0f }); // プレイヤーの当たり判定のサイズ(幅, 高さ, 奥行き)
}

void Player::OnCollision() {
    if (invincibilityTimer_ > 0 || isDead_ || isRolling_) return; // 無敵中、死亡時、またはローリング中（回避）は無効
    
    hp_--;
    if (hp_ <= 0) {
        isDead_ = true;
    } else {
        invincibilityTimer_ = 60; // 1秒間無敵
        
        // 被弾時カメラシェイクやポストエフェクトなどを入れるのも良い
    }
}

void Player::Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera) {
    Move();
    Attack(bullets, parentCamera);

    if (invincibilityTimer_ > 0) {
        invincibilityTimer_--;
    }

    object_->Update();
    if (accessory_) {
        // アクセサリを回転させるデモ
        Vector3 rot = accessory_->GetRotation();
        rot.y += 0.05f;
        accessory_->SetRotation(rot);
        accessory_->Update();
    }
    
    if (colliderObject_) {
        // 親の設定はGamePlayScene側で行う
        colliderObject_->SetTranslate(object_->GetTranslate());
        colliderObject_->SetRotation(object_->GetRotation());
        colliderObject_->Update();
    }
}

void Player::Draw() {
    if (reticle_) {
        reticle_->Draw();
    }
    if (frontReticle_) {
        frontReticle_->Draw();
    }
    
    // 無敵時間中は点滅させる (4フレームに1回非表示)
    bool shouldDrawPlayer = true;
    if (invincibilityTimer_ > 0) {
        if ((invincibilityTimer_ / 4) % 2 == 0) {
            shouldDrawPlayer = false;
        }
    }
    
    if (shouldDrawPlayer) {
        if (object_) {
            object_->Draw();
        }
        if (accessory_) {
            accessory_->Draw();
        }
    }
}

void Player::DrawCollider() {
    if (colliderObject_) {
        colliderObject_->Draw();
    }
}

void Player::SetReticleColor(const Vector4& color) {
    reticleColor_ = color;
    if (reticle_) {
        reticle_->GetModel()->SetColor(reticleColor_);
    }
    if (frontReticle_) {
        Vector4 frontColor = reticleColor_;
        frontColor.w = 1.0f; // 手前も透過させない（モデル共有問題回避）
        frontReticle_->GetModel()->SetColor(frontColor);
    }
}

Vector3 Player::GetReticleWorldPosition() const {
    if (!reticle_) return {0,0,0};
    // 照準オブジェクトのワールド座標をマトリックスから取得
    const Matrix4x4& mat = reticle_->GetmatWorld();
    return { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
}

void Player::Move() {
    if (!input_) return;

    // --- ローリング回避の開始判定 ---
    if (!isRolling_) {
        if (input_->TriggerKey(DIK_Q)) {
            isRolling_ = true;
            rollTimer_ = 0;
            rollDirection_ = -1.0f; // 左
            isDodgeTriggered_ = true;
        } else if (input_->TriggerKey(DIK_E)) {
            isRolling_ = true;
            rollTimer_ = 0;
            rollDirection_ = 1.0f; // 右
            isDodgeTriggered_ = true;
        }
    }

    if (isRolling_) {
        rollTimer_++;
        // ローリング中はレティクルを強制的に移動させる
        float rollMoveSpeed = 0.2f; // フレーム減少に合わせて少しだけ調整
        reticlePosition_.x += rollDirection_ * rollMoveSpeed;

        // 一定時間でローリング終了
        if (rollTimer_ >= rollMaxTime_) {
            isRolling_ = false;
            // if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
            //     pp->SetEffectActive("Grayscale", false);
            // }
        }
    }

    // --- ブースト判定 ---
    isBoosting_ = input_->PushKey(DIK_LSHIFT);
    float currentReticleSpeed = reticleSpeed_;
    float currentFollowSpeed = followSpeed_;
    if (isBoosting_) {
        currentReticleSpeed *= 1.5f;
        currentFollowSpeed *= 1.5f;
    }

    // --- 照準（レティクル）の移動 ---
    if (input_->PushKey(DIK_W) || input_->PushKey(DIK_UP)) {
        reticlePosition_.y += currentReticleSpeed;
    }
    if (input_->PushKey(DIK_S) || input_->PushKey(DIK_DOWN)) {
        reticlePosition_.y -= currentReticleSpeed;
    }
    if (input_->PushKey(DIK_A) || input_->PushKey(DIK_LEFT)) {
        reticlePosition_.x -= currentReticleSpeed;
    }
    if (input_->PushKey(DIK_D) || input_->PushKey(DIK_RIGHT)) {
        reticlePosition_.x += currentReticleSpeed;
    }

    // 照準の移動制限
    // Y座標の下限を厳しくして（例: -2.0f など）、地面に潜らないようにする
    float reticleLimitYMin = -2.0f; // これ以上は下に行かない
    reticlePosition_.x = std::clamp(reticlePosition_.x, -moveLimitX_, moveLimitX_);
    reticlePosition_.y = std::clamp(reticlePosition_.y, reticleLimitYMin, moveLimitY_);

    // 奥照準のZ座標はプレイヤーより一定距離奥に保つ
    reticlePosition_.z = logicalPosition_.z + 40.0f;
    
    reticle_->SetTranslate(reticlePosition_);
    reticle_->Update();

    // 手前照準（大）の計算と更新
    // 自機座標と奥照準座標の中間に配置する（Lerpのような補間）
    if (frontReticle_) {
        Vector3 frontReticlePos = {
            logicalPosition_.x + (reticlePosition_.x - logicalPosition_.x) * 0.4f,
            logicalPosition_.y + (reticlePosition_.y - logicalPosition_.y) * 0.4f,
            logicalPosition_.z + (reticlePosition_.z - logicalPosition_.z) * 0.4f
        };
        frontReticle_->SetTranslate(frontReticlePos);
        frontReticle_->Update();
    }

    // 前回の位置を保存
    Vector3 oldPos = logicalPosition_;

    // プレイヤーの追従（Panzer Dragoon風）
    // プレイヤーは照準のXY座標に向かって少し遅れて追従する
    logicalPosition_.x += (reticlePosition_.x - logicalPosition_.x) * currentFollowSpeed;
    logicalPosition_.y += (reticlePosition_.y - logicalPosition_.y) * currentFollowSpeed;

    // プレイヤーがカメラ外に出ないように制限
    logicalPosition_.x = std::clamp(logicalPosition_.x, -playerLimitX_, playerLimitX_);
    logicalPosition_.y = std::clamp(logicalPosition_.y, playerLimitYMin_, playerLimitYMax_);

    // 実際の移動量（速度）を計算。画面端で動けない時は0になる
    Vector3 velocity = {
        logicalPosition_.x - oldPos.x,
        logicalPosition_.y - oldPos.y,
        0.0f
    };

    // 移動時の体の傾き計算
    // 上下移動時のピッチ角（機首の上下）
    float pitchAngle = velocity.y * -2.0f;
    pitchAngle = std::clamp(pitchAngle, -0.6f, 0.6f);

    // 左右移動時のヨー角（機首の左右）
    float yawAngle = velocity.x * 0.6f;
    yawAngle = std::clamp(yawAngle, -0.4f, 0.4f);

    // 左右移動時のバンク角（機体のロール）
    float bankAngle = velocity.x * -1.5f;
    bankAngle = std::clamp(bankAngle, -0.8f, 0.8f);

    // 滑らかに補間（Lerp）する
    float lerpSpeed = 0.1f; // 傾きが戻る・変わる速さ（0.1 = 毎フレーム10%近づく）
    currentPitch_ += (pitchAngle - currentPitch_) * lerpSpeed;
    currentYaw_ += (yawAngle - currentYaw_) * lerpSpeed;
    currentBank_ += (bankAngle - currentBank_) * lerpSpeed;

    // 最終的なバンク角（ローリング角度は累積させずに一時的に足す）
    float finalBank = currentBank_;
    if (isRolling_) {
        float rollAngle = (static_cast<float>(rollTimer_) / rollMaxTime_) * 3.14159265f * 2.0f;
        finalBank += rollAngle * -rollDirection_;
    }
    
    // 実際のモデルの Transform にはオフセットを足して適用
    object_->SetScale(playerScale_);
    object_->SetRotation(Vector3(
        baseRotation_.x + currentPitch_ + modelRotOffset_.x,
        baseRotation_.y + currentYaw_ + modelRotOffset_.y,
        baseRotation_.z + finalBank + modelRotOffset_.z
    ));
    object_->SetTranslate(Vector3(logicalPosition_.x + modelPosOffset_.x, logicalPosition_.y + modelPosOffset_.y, logicalPosition_.z + modelPosOffset_.z));
}

void Player::Attack(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera) {
    if (!input_) return;

    // 発射間隔のタイマー更新
    if (attackTimer_ > 0) {
        attackTimer_--;
    }

    // タイマーが0以下なら発射可能
    // スペースキーまたはマウス左クリックで発射
    if (attackTimer_ <= 0 && (input_->PushKey(DIK_SPACE))) {
        attackTimer_ = attackInterval_; // タイマーリセット
        
        // プレイヤーのワールド座標を取得する (object_はカメラの子なのでGetTranslateはローカル座標になってしまう)
        const Matrix4x4& mat = object_->GetmatWorld();
        Vector3 playerPos = { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
        
        Vector3 targetPos = GetReticleWorldPosition();
        
        // 照準(またはロックオン座標)に向かうベクトルを計算
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
        
        Vector3 velocity = {
            direction.x * bulletSpeed_,
            direction.y * bulletSpeed_,
            direction.z * bulletSpeed_
        };

        // 弾を生成
        std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerBullet>();
        newBullet->Initialize(object3dCommon_, playerPos, velocity, parentCamera, isLockOn_ ? lockOnTargetEnemy_ : nullptr);
        bullets.push_back(std::move(newBullet));
    }
}

void Player::LoadSettings(const std::string& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        if (j.contains("speed")) speed_ = j["speed"];
        if (j.contains("reticleSpeed")) reticleSpeed_ = j["reticleSpeed"];
        if (j.contains("moveLimitX")) moveLimitX_ = j["moveLimitX"];
        if (j.contains("moveLimitY")) moveLimitY_ = j["moveLimitY"];
        if (j.contains("attackInterval")) attackInterval_ = j["attackInterval"];
        if (j.contains("rollMaxTime")) rollMaxTime_ = j["rollMaxTime"];
        if (j.contains("playerLimitX")) playerLimitX_ = j["playerLimitX"];
        if (j.contains("playerLimitYMin")) playerLimitYMin_ = j["playerLimitYMin"];
        if (j.contains("playerLimitYMax")) playerLimitYMax_ = j["playerLimitYMax"];
        if (j.contains("followSpeed")) followSpeed_ = j["followSpeed"];
        if (j.contains("bulletSpeed")) bulletSpeed_ = j["bulletSpeed"];
        if (j.contains("modelName")) modelName_ = j["modelName"];
        if (j.contains("reflection")) reflection_ = j["reflection"];
        if (j.contains("color")) {
            auto c = j["color"];
            if (c.is_array() && c.size() == 4) {
                color_ = { c[0], c[1], c[2], c[3] };
            }
        }
        if (j.contains("modelPosOffset")) {
            auto arr = j["modelPosOffset"];
            if (arr.is_array() && arr.size() == 3) modelPosOffset_ = { arr[0], arr[1], arr[2] };
        }
        if (j.contains("modelRotOffset")) {
            auto arr = j["modelRotOffset"];
            if (arr.is_array() && arr.size() == 3) modelRotOffset_ = { arr[0], arr[1], arr[2] };
        }
        if (j.contains("playerScale")) {
            auto arr = j["playerScale"];
            if (arr.is_array() && arr.size() == 3) playerScale_ = { arr[0], arr[1], arr[2] };
        }
        file.close();
        
        // 読み込んだ設定を反映
        if (object_) {
            object_->SetModel(modelName_);
            object_->GetModel()->SetColor(color_);
            object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
            if (accessory_) {
                accessory_->SetParent(object_.get());
            }
        }
    }
}

void Player::SaveSettings(const std::string& filepath) {
    nlohmann::json j;
    j["speed"] = speed_;
    j["reticleSpeed"] = reticleSpeed_;
    j["moveLimitX"] = moveLimitX_;
    j["moveLimitY"] = moveLimitY_;
    j["attackInterval"] = attackInterval_;
    j["rollMaxTime"] = rollMaxTime_;
    j["playerLimitX"] = playerLimitX_;
    j["playerLimitYMin"] = playerLimitYMin_;
    j["playerLimitYMax"] = playerLimitYMax_;
    j["followSpeed"] = followSpeed_;
    j["bulletSpeed"] = bulletSpeed_;
    j["modelName"] = modelName_;
    j["reflection"] = reflection_;
    j["color"] = { color_.x, color_.y, color_.z, color_.w };
    j["modelPosOffset"] = { modelPosOffset_.x, modelPosOffset_.y, modelPosOffset_.z };
    j["modelRotOffset"] = { modelRotOffset_.x, modelRotOffset_.y, modelRotOffset_.z };
    j["playerScale"] = { playerScale_.x, playerScale_.y, playerScale_.z };

    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
}

void Player::DrawUI() {
#ifdef USE_IMGUI
    ImGui::Begin("Player Editor");
    
    if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        // モデル変更
        const char* models[] = { "cube.obj", "player.obj", "monsterBall.obj","suzanne.obj"};
        int currentModel = 0;
        for (int i = 0; i < 3; ++i) {
            if (modelName_ == models[i]) {
                currentModel = i;
                break;
            }
        }
        if (ImGui::Combo("Model", &currentModel, models, 3)) {
            modelName_ = models[currentModel];
            if (object_) {
                object_->SetModel(modelName_);
                // モデルを変えた場合、再度色と反射係数をセットする
                object_->GetModel()->SetColor(color_);
                object_->SetEnvironmentTextureIndex(skyboxTexIndex_);
                object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
                if (accessory_) {
                    accessory_->SetParent(object_.get());
                }
            }
        }
        
        // 色変更
        float col[4] = { color_.x, color_.y, color_.z, color_.w };
        if (ImGui::ColorEdit4("Color", col)) {
            color_ = { col[0], col[1], col[2], col[3] };
            if (object_) {
                object_->GetModel()->SetColor(color_);
            }
        }
        
        // 映り込み切り替え
        if (ImGui::Checkbox("Reflection", &reflection_)) {
            if (object_) {
                object_->SetEnvironmentCoefficient(reflection_ ? 1.0f : 0.0f);
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Transform Offset & Scale");
        float posOffset[3] = { modelPosOffset_.x, modelPosOffset_.y, modelPosOffset_.z };
        if (ImGui::DragFloat3("Position Offset", posOffset, 0.1f)) {
            modelPosOffset_ = { posOffset[0], posOffset[1], posOffset[2] };
        }
        float rotOffset[3] = { modelRotOffset_.x, modelRotOffset_.y, modelRotOffset_.z };
        if (ImGui::DragFloat3("Rotation Offset", rotOffset, 0.05f)) {
            modelRotOffset_ = { rotOffset[0], rotOffset[1], rotOffset[2] };
        }
        float scale[3] = { playerScale_.x, playerScale_.y, playerScale_.z };
        if (ImGui::DragFloat3("Scale", scale, 0.05f)) {
            playerScale_ = { scale[0], scale[1], scale[2] };
        }
    }
    
    if (ImGui::CollapsingHeader("Movement Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Reticle Speed", &reticleSpeed_, 0.01f, 0.1f, 5.0f);
        ImGui::DragFloat("Follow Speed", &followSpeed_, 0.001f, 0.01f, 1.0f);
        ImGui::DragFloat("Move Limit X", &moveLimitX_, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("Move Limit Y", &moveLimitY_, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("Player Limit X", &playerLimitX_, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("Player Limit Y Min", &playerLimitYMin_, 0.1f, -10.0f, 50.0f);
        ImGui::DragFloat("Player Limit Y Max", &playerLimitYMax_, 0.1f, 1.0f, 50.0f);
        ImGui::DragInt("Roll Max Time", &rollMaxTime_, 1, 1, 60);
    }
    
    if (ImGui::CollapsingHeader("Combat Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragInt("Attack Interval", &attackInterval_, 1, 1, 60);
        ImGui::DragFloat("Bullet Speed", &bulletSpeed_, 0.1f, 0.1f, 20.0f);
    }
    
    if (ImGui::Button("Save Settings")) {
        SaveSettings("resources/json/player/player_settings.json");
    }
    
    ImGui::End();
#endif
}
