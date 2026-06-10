#include "Player.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "externals/nlohmann/json.hpp"
#include <fstream>
#include <algorithm>

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
     
    // アクセサリの初期化
    accessory_ = std::make_unique<Object3d>();
    accessory_->Initialize(object3dCommon);
    accessory_->SetModel("suzanne.obj");
    accessory_->GetModel()->SetColor({0.8f, 0.8f, 0.0f, 1.0f});
    accessory_->SetEnvironmentTextureIndex(skyboxTexIndex);
    accessory_->SetEnvironmentCoefficient(0.0f);
    accessory_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    accessory_->SetTranslate(Vector3(1.5f, 1.0f, 0.0f)); // プレイヤーの右上に配置
    accessory_->SetParent(object_.get()); // 親子関係を設定

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
    if (accessory_) {
        // アクセサリを回転させるデモ
        Vector3 rot = accessory_->GetRotation();
        rot.y += 0.05f;
        accessory_->SetRotation(rot);
        accessory_->Update();
    }
}

void Player::Draw() {
        if (reticle_) {
        reticle_->Draw();
    }
        if (object_) {
        object_->Draw();
    }
    if (accessory_) {
        accessory_->Draw();
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
        if (rollTimer_ >= rollMaxTime_) {
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
    reticlePosition_.x = std::clamp(reticlePosition_.x, -moveLimitX_, moveLimitX_);
    reticlePosition_.y = std::clamp(reticlePosition_.y, -moveLimitY_, moveLimitY_);

    // 照準のZ座標はプレイヤーより一定距離奥に保つ
    reticlePosition_.z = logicalPosition_.z + 40.0f;
    
    reticle_->SetTranslate(reticlePosition_);
    reticle_->Update();

    // --- プレイヤーの追従（Panzer Dragoon風） ---
    // プレイヤーは照準のXY座標に向かって少し遅れて追従する
    logicalPosition_.x += (reticlePosition_.x - logicalPosition_.x) * followSpeed_;
    logicalPosition_.y += (reticlePosition_.y - logicalPosition_.y) * followSpeed_;

    // プレイヤーがカメラ外に出ないように制限
    logicalPosition_.x = std::clamp(logicalPosition_.x, -playerLimitX_, playerLimitX_);
    logicalPosition_.y = std::clamp(logicalPosition_.y, playerLimitYMin_, playerLimitYMax_);

    // 移動時の体の傾き計算（Panzer Dragoon風）
    // 上下移動時のピッチ角（機首の上下）
    float pitchAngle = (reticlePosition_.y - logicalPosition_.y) * -0.1f;
    pitchAngle = std::clamp(pitchAngle, -0.6f, 0.6f);

    // 左右移動時のヨー角（機首の左右）
    float yawAngle = (reticlePosition_.x - logicalPosition_.x) * 0.05f;
    yawAngle = std::clamp(yawAngle, -0.4f, 0.4f);

    // 左右移動時のバンク角（機体のロール）
    float bankAngle = (reticlePosition_.x - logicalPosition_.x) * -0.1f;
    bankAngle = std::clamp(bankAngle, -0.8f, 0.8f);

    // ローリング中なら360度回転を追加
    if (isRolling_) {
        float rollAngle = (static_cast<float>(rollTimer_) / rollMaxTime_) * 3.14159265f * 2.0f;
        bankAngle += rollAngle * -rollDirection_;
    }
    
    // 実際のモデルの Transform にはオフセットを足して適用
    object_->SetScale(playerScale_);
    object_->SetRotation(Vector3(pitchAngle + modelRotOffset_.x, yawAngle + modelRotOffset_.y, bankAngle + modelRotOffset_.z));
    object_->SetTranslate(Vector3(logicalPosition_.x + modelPosOffset_.x, logicalPosition_.y + modelPosOffset_.y, logicalPosition_.z + modelPosOffset_.z));
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
        attackTimer_ = attackInterval_; // タイマーリセット
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
        
        Vector3 velocity = {
            direction.x * bulletSpeed_,
            direction.y * bulletSpeed_,
            direction.z * bulletSpeed_
        };

        // 弾を生成
        std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerBullet>();
        newBullet->Initialize(object3dCommon_, playerPos, velocity);
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
