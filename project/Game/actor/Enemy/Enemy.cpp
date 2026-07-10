#include "Enemy.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "Game/Actor/Player/Player.h"
#include "Game/Actor/Bullet/EnemyBullet.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, const LevelObjectData& nodeData, uint32_t skyboxTexIndex) {
    object3dCommon_ = object3dCommon;
    position_ = nodeData.translation;
    spawnPos_ = position_;
    typeName_ = nodeData.enemyType;
    if (typeName_.empty()) typeName_ = "RUSHER";

    targetPos_ = nodeData.enemyTargetPos;
    maxY_ = nodeData.enemyMaxY;
    minY_ = nodeData.enemyMinY;
    formationId_ = nodeData.enemyFormationId;
    
    isDead_ = false;
    collider_ = nodeData.collider;

    // タイプごとの設定
    std::string modelName = "cube.obj";
    if (typeName_ == "RUSHER") {
        modelName = "suzanne.obj";
        hp_ = 2; 
    } else if (typeName_ == "SHOOTER" || typeName_ == "HOMING") {
        modelName = "suzanne.obj";
        hp_ = 3; 
    } else if (typeName_ == "TURRET") {
        modelName = "cube.obj";
        hp_ = 5;
    } else { // fallback
        if (nodeData.fileName == "Asteroid") {
            modelName = "monsterBall.obj";
            hp_ = 2;
        } else {
            modelName = "cube.obj";
            hp_ = 2;
        }
    }

    // モデルがロードされているか確認してロード
    ModelManager::GetInstance()->LoadModel(modelName);

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName);
    object_->SetTranslate(position_);
    
    // 敵本体のスケールには引数で受け取ったBlender上のscaleをそのまま設定する
    object_->SetScale(nodeData.scale);
    
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);

    // デバッグ用コライダーオブジェクトの初期化
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    if (collider_.type == "SPHERE") {
        ModelManager::GetInstance()->LoadModel("collider_sphere_enemy.obj");
        colliderObject_->SetModel("collider_sphere_enemy.obj"); // 球の代用
        colliderObject_->SetScale({collider_.radius, collider_.radius, collider_.radius});
    } else {
        ModelManager::GetInstance()->LoadModel("collider_cube_enemy.obj");
        colliderObject_->SetModel("collider_cube_enemy.obj");
        colliderObject_->SetScale({collider_.size.x, collider_.size.y, collider_.size.z});
    }
    
    // コライダー専用のモデルなので、色を赤にしても他のモデルに影響しない
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
    colliderObject_->SetEnvironmentCoefficient(0.0f);

    // 初期位置にコライダーオブジェクトを追従させる
    object_->Update();
    if (colliderObject_) {
        Vector3 colliderPos = {
            position_.x + collider_.center.x,
            position_.y + collider_.center.y,
            position_.z + collider_.center.z
        };
        colliderObject_->SetTranslate(colliderPos);
        colliderObject_->SetRotation(object_->GetRotation());
        colliderObject_->Update();
    }

    // 丸影用オブジェクトの初期化
    shadowObject_ = std::make_unique<Object3d>();
    shadowObject_->Initialize(object3dCommon);
    ModelManager::GetInstance()->LoadModel("plane.obj");
    shadowObject_->SetModel("plane.obj");
    
    // 【デバッグ用】真っ黒の不透明(alpha=1.0)にして、確実に四角形が見えるかテスト
    shadowObject_->GetModel()->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    shadowObject_->SetEnableLighting(false);
    shadowObject_->SetSelectLightings(0);
    
    // テクスチャを貼る（エンジンのアルファテストで丸く切り抜かれるか確認）
    TextureManager::GetInstance()->LoadTexture("circle2.png");
    uint32_t circleTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("circle2.png");
    if (circleTex != UINT32_MAX) {
        shadowObject_->GetModel()->SetTextureIndex(circleTex);
    }
    
    shadowObject_->Update();
}

void Enemy::SetMovePath(std::unique_ptr<Rail> path) {
    movePath_ = std::move(path);
    pathProgress_ = 0.0f;
}

void Enemy::Update(const Vector3& cameraPos, const Vector3& cameraForward, Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets) {
    if (isDead_) return;

    Vector3 playerWorldPos = cameraPos;
    if (player && player->GetObject3d()) {
        const Matrix4x4& mat = player->GetObject3d()->GetmatWorld();
        playerWorldPos = { mat.m[3][0], mat.m[3][1], mat.m[3][2] };
    }

    if (invincibilityTimer_ > 0) {
        invincibilityTimer_--;
        // 無敵時間中は点滅させる（赤色）
        if (invincibilityTimer_ % 10 < 5) {
            object_->GetModel()->SetColor({ 1.0f, 0.5f, 0.5f, 1.0f });
        } else {
            object_->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        }
    } else {
        object_->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    }

    if (!isActive_) {
        // アクティブ化判定: プレイヤー（またはカメラ）との距離が一定以内になったら動き出す
        float distance = std::sqrt((position_.x - playerWorldPos.x)*(position_.x - playerWorldPos.x) + (position_.z - playerWorldPos.z)*(position_.z - playerWorldPos.z));
        float spawnDist = (spawnProgress_ > 0.0f) ? spawnProgress_ * 300.0f : 200.0f; // 未指定なら200m
        if (distance < spawnDist) {
            isActive_ = true;
        } else {
            return; // まだ出番ではない
        }
    }

    if (!isAutoAI_) {
        if (movePath_ && movePath_->IsValid()) {
            // スプライン曲線に沿って移動する
            float moveSpeed = 0.5f; // 必要に応じて調整
            pathProgress_ += moveSpeed / movePath_->GetTotalLength();
            if (pathProgress_ > 1.0f) {
                isDead_ = true;
            } else {
                position_ = movePath_->GetPosition(pathProgress_);
                Vector3 forward = movePath_->GetForward(pathProgress_);
                float yaw = std::atan2(forward.x, forward.z);
                float pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
                object_->SetRotation({pitch, yaw, 0.0f});
            }
        } else {
            // タイプごとの基本行動
            if (typeName_ == "TURRET") {
                // その場にとどまる
                Vector3 rot = object_->GetRotation();
                Vector3 toPlayer = { playerWorldPos.x - position_.x, playerWorldPos.y - position_.y, playerWorldPos.z - position_.z };
                float yaw = std::atan2(toPlayer.x, toPlayer.z);
                float pitch = std::asin(std::clamp(-toPlayer.y / std::sqrt(toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y + toPlayer.z*toPlayer.z), -1.0f, 1.0f));
                rot.y += (yaw - rot.y) * 0.1f;
                rot.x += (pitch - rot.x) * 0.1f;
                object_->SetRotation(rot);
            } else if (typeName_ == "RUSHER") {
                attackTimer_++;
                if (attackTimer_ < 120) {
                    // 2秒間(120フレーム)はプレイヤーの方を向きながら待機
                    Vector3 toPlayer = { playerWorldPos.x - position_.x, playerWorldPos.y - position_.y, playerWorldPos.z - position_.z };
                    float yaw = std::atan2(toPlayer.x, toPlayer.z);
                    object_->SetRotation({0, yaw, 0});
                    
                    // 待機中は少しフワフワさせる演出（任意）
                    position_.y += std::sin(attackTimer_ * 0.1f) * 0.05f;

                    // 突進開始直前に方向を決定する
                    if (attackTimer_ == 119) {
                        float length = std::sqrt(toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y + toPlayer.z*toPlayer.z);
                        if (length > 0.0f) {
                            dashVelocity_.x = (toPlayer.x / length) * 1.5f; // 突進スピード
                            dashVelocity_.y = (toPlayer.y / length) * 1.5f;
                            dashVelocity_.z = (toPlayer.z / length) * 1.5f;
                        }
                    }
                } else {
                    // 突進！
                    position_.x += dashVelocity_.x;
                    position_.y += dashVelocity_.y;
                    position_.z += dashVelocity_.z;
                }
            } else {
                // SHOOTER, HOMINGなどは、プレイヤー前方に追従して浮遊する（固定砲台と差別化）
                isAutoAI_ = true;
                aiOffset_ = {
                    ((float)rand() / RAND_MAX * 20.0f - 10.0f),   // X: -10 ~ 10 (以前より狭く)
                    ((float)rand() / RAND_MAX * 10.0f - 5.0f),    // Y: -5 ~ 5 (以前より狭く)
                    50.0f + ((float)rand() / RAND_MAX * 50.0f)     // Z: 50 ~ 100
                };
            }
        }
    }

    if (isAutoAI_) {
        // 揺れ（スウェイ）も少しマイルドに
        float swayX = std::sin(pathProgress_ * 3.0f) * 2.0f;
        float swayY = std::cos(pathProgress_ * 2.5f) * 1.5f;
        pathProgress_ += 0.02f; // スウェイのための時間進行用

        // 目標座標（Target Positionが指定されていなければカメラ前方を基準にする）
        Vector3 basePos = cameraPos;
        if (targetPos_.x != 0.0f || targetPos_.y != 0.0f || targetPos_.z != 0.0f) {
            basePos = targetPos_;
        }

        Vector3 right = { cameraForward.z, 0.0f, -cameraForward.x };
        Vector3 targetPos = {
            basePos.x + right.x * (aiOffset_.x + swayX),
            basePos.y + aiOffset_.y + swayY,
            basePos.z + right.z * (aiOffset_.x + swayX) + aiOffset_.z
        };

        // 目標座標へなめらかに移動
        float lerpSpeed = 0.05f;
        position_.x += (targetPos.x - position_.x) * lerpSpeed;
        position_.y += (targetPos.y - position_.y) * lerpSpeed;
        position_.z += (targetPos.z - position_.z) * lerpSpeed;

        // プレイヤーの方を向く
        Vector3 dirToPlayer = { playerWorldPos.x - position_.x, playerWorldPos.y - position_.y, playerWorldPos.z - position_.z };
        float dist = std::sqrt(dirToPlayer.x*dirToPlayer.x + dirToPlayer.z*dirToPlayer.z);
        if (dist > 0.001f) {
            float yaw = std::atan2(-dirToPlayer.x, -dirToPlayer.z);
            // フワフワした動きに合わせて少し機体を傾ける（ロール）とさらに良くなる
            float roll = (targetPos.x - position_.x) * 0.05f;
            object_->SetRotation(Vector3(0.0f, yaw, roll));
        }
    }

    // 高さの制限（クランプ）
    if (position_.y > maxY_) position_.y = maxY_;
    if (position_.y < minY_) position_.y = minY_;

    // 弾の発射処理
    if (typeName_ == "SHOOTER" || typeName_ == "HOMING" || typeName_ == "TURRET") {
        if (isActive_ && !isDead_) {
            attackTimer_++;
            if (attackTimer_ >= 180) { // 3秒ごとに発射（間隔を延長して弱体化）
                attackTimer_ = 0;
                auto bullet = std::make_unique<EnemyBullet>();
                bool isHoming = (typeName_ == "HOMING");
                Vector3 toPlayer = { playerWorldPos.x - position_.x, playerWorldPos.y - position_.y, playerWorldPos.z - position_.z };
                float len = std::sqrt(toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y + toPlayer.z*toPlayer.z);
                if (len > 0.0f) {
                    toPlayer = { toPlayer.x/len, toPlayer.y/len, toPlayer.z/len };
                }
                Vector3 velocity = { toPlayer.x * 2.5f, toPlayer.y * 2.5f, toPlayer.z * 2.5f };
                bullet->Initialize(object3dCommon_, position_, velocity, isHoming, player);
                enemyBullets.push_back(std::move(bullet));
            }
        }
    }

    object_->SetTranslate(position_);
    object_->Update();

    // コライダーオブジェクトも追従させる
    if (colliderObject_) {
        Vector3 colliderPos = {
            position_.x + collider_.center.x,
            position_.y + collider_.center.y,
            position_.z + collider_.center.z
        };
        colliderObject_->SetTranslate(colliderPos);
        colliderObject_->SetRotation(object_->GetRotation());
        colliderObject_->Update();
    }

    // 丸影のデバッグ表示
    if (shadowObject_ && isActive_) {
        // 地形に完全に埋まらないよう、少し高めの Y=0.5f くらいに置いてみる
        float shadowY = 0.5f; 
        shadowObject_->SetTranslate({ position_.x, shadowY, position_.z });
        
        // 壁のように立たないようにX軸を90度（1.570796f）回転して床に寝かせる
        // ※もしこれでカリングされて見えない場合は -1.570796f か 180度に直します
        shadowObject_->SetRotation({ 1.570796f, 0.0f, 0.0f });
        
        // 分かりやすいように少し大きめの四角にする
        float shadowScale = 2.0f;
        shadowObject_->SetScale({ shadowScale, shadowScale, shadowScale });
        shadowObject_->Update();
    }
}

void Enemy::Draw() {
    if (!isDead_ && isActive_ && object_) {
        object_->Draw();
    }
    if (!isDead_ && isActive_ && shadowObject_) {
        shadowObject_->Draw();
    }
}

void Enemy::DrawCollider() {
    if (!isDead_ && colliderObject_) {
        colliderObject_->Draw();
    }
}

void Enemy::OnCollision() {
    if (invincibilityTimer_ > 0) return; // 無敵時間中はダメージを受けない

    hp_--;
    invincibilityTimer_ = 30; // 30フレーム無敵
    
    if (hp_ <= 0) {
        isDead_ = true;
    }
}

bool Enemy::CheckCollision(const Sphere& bulletSphere) const {
    if (isDead_) return false;

    Vector3 colCenter = { position_.x + collider_.center.x, position_.y + collider_.center.y, position_.z + collider_.center.z };

    if (collider_.type == "SPHERE") {
        Sphere enemySphere = { colCenter, collider_.radius };
        return CollisionMath::IsCollision(bulletSphere, enemySphere);
    } else if (collider_.type == "OBB") {
        Matrix4x4 identity = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
        OBB enemyOBB = CollisionMath::CreateOBB(colCenter, collider_.size, identity);
        return CollisionMath::IsCollision(bulletSphere, enemyOBB);
    } else {
        // AABB
        AABB enemyAABB = {
            { colCenter.x - collider_.size.x * 0.5f, colCenter.y - collider_.size.y * 0.5f, colCenter.z - collider_.size.z * 0.5f },
            { colCenter.x + collider_.size.x * 0.5f, colCenter.y + collider_.size.y * 0.5f, colCenter.z + collider_.size.z * 0.5f }
        };
        return CollisionMath::IsCollision(bulletSphere, enemyAABB);
    }
}

bool Enemy::CheckRaycast(const Ray& ray, float* outDist) const {
    if (isDead_) return false;

    Vector3 colCenter = { position_.x + collider_.center.x, position_.y + collider_.center.y, position_.z + collider_.center.z };

    if (collider_.type == "SPHERE") {
        Sphere enemySphere = { colCenter, collider_.radius };
        return CollisionMath::Raycast(ray, enemySphere, outDist);
    } else if (collider_.type == "OBB") {
        Matrix4x4 identity = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
        OBB enemyOBB = CollisionMath::CreateOBB(colCenter, collider_.size, identity);
        return CollisionMath::Raycast(ray, enemyOBB, outDist);
    } else {
        // AABB
        AABB enemyAABB = {
            { colCenter.x - collider_.size.x * 0.5f, colCenter.y - collider_.size.y * 0.5f, colCenter.z - collider_.size.z * 0.5f },
            { colCenter.x + collider_.size.x * 0.5f, colCenter.y + collider_.size.y * 0.5f, colCenter.z + collider_.size.z * 0.5f }
        };
        return CollisionMath::Raycast(ray, enemyAABB, outDist);
    }
}

void Enemy::SetTexturePath(const std::string& path) {
    texturePath_ = path;
    if (!texturePath_.empty() && object_ && object_->GetModel()) {
        TextureManager::GetInstance()->LoadTexture(texturePath_);
        uint32_t texIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(texturePath_);
        if (texIndex != TextureManager::GetInstance()->GetDefaultTextureIndex()) {
            object_->GetModel()->SetTextureIndex(texIndex);
        }
    }
}
