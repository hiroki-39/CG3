#include "Enemy.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, const Vector3& pos, const Vector3& scale, const std::string& typeName, uint32_t skyboxTexIndex, const LevelCollider& colliderInfo) {
    position_ = pos;
    typeName_ = typeName;
    isDead_ = false;
    collider_ = colliderInfo;

    // タイプごとの設定
    std::string modelName = "cube.obj";
    if (typeName_ == "Asteroid") {
        modelName = "monsterBall.obj"; // とりあえずあるモデルを流用
        hp_ = 2; 
    } else if (typeName_ == "Fighter") {
        modelName = "suzanne.obj"; // 猿のモデルをFighterの代わりにする
        hp_ = 2; 
    } else {
        modelName = "cube.obj";
        hp_ = 2;
    }

    // モデルがロードされているか確認してロード
    ModelManager::GetInstance()->LoadModel(modelName);

    object_ = std::make_unique<Object3d>();
    object_->Initialize(object3dCommon);
    object_->SetModel(modelName);
    object_->SetTranslate(position_);
    
    // 敵本体のスケールには引数で受け取ったBlender上のscaleをそのまま設定する
    object_->SetScale(scale);
    
    object_->SetEnvironmentTextureIndex(skyboxTexIndex);

    // デバッグ用コライダーオブジェクトの初期化
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    if (collider_.type == "SPHERE") {
        ModelManager::GetInstance()->LoadModel("collider_sphere.obj");
        colliderObject_->SetModel("collider_sphere.obj"); // 球の代用
        colliderObject_->SetScale({collider_.radius, collider_.radius, collider_.radius});
    } else {
        ModelManager::GetInstance()->LoadModel("collider_cube.obj");
        colliderObject_->SetModel("collider_cube.obj");
        colliderObject_->SetScale({collider_.size.x, collider_.size.y, collider_.size.z});
    }
    
    // コライダー専用のモデルなので、色を赤にしても他のモデルに影響しない
    colliderObject_->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

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

void Enemy::Update(const Vector3& playerPos, const Vector3& playerForward) {
    if (isDead_) return;

    if (!isActive_) {
        // アクティブ化判定: プレイヤーとの距離が一定以内になったら動き出す
        float distance = std::sqrt((position_.x - playerPos.x)*(position_.x - playerPos.x) + (position_.z - playerPos.z)*(position_.z - playerPos.z));
        float spawnDist = (spawnProgress_ > 0.0f) ? spawnProgress_ * 300.0f : 200.0f; // 未指定なら200m
        if (distance < spawnDist) {
            isActive_ = true;
        } else {
            return; // まだ出番ではない
        }
    }

    if (!isAutoAI_) {
        if (movePath_ && movePath_->IsValid()) {
            float speed = movePath_->GetSpeed(pathProgress_);
            if (speed <= 0.0f) speed = 20.0f;
            float length = movePath_->GetTotalLength();
            if (length > 0.0f) {
                pathProgress_ += (speed / length) * (1.0f / 60.0f);
            }
            if (pathProgress_ >= 1.0f) {
                pathProgress_ = 1.0f;
                isAutoAI_ = true; // レール終端でAIに切り替え
                // カメラの視野（画面内）に確実に収まり、重なりを防ぐ程度の小さなオフセット
                aiOffset_ = {
                    ((float)rand() / RAND_MAX * 20.0f - 10.0f),   // X: -10 ~ 10
                    ((float)rand() / RAND_MAX * 10.0f - 5.0f),    // Y: -5 ~ 5
                    80.0f + ((float)rand() / RAND_MAX * 20.0f)    // Z: 80 ~ 100
                };
            }
            position_ = movePath_->GetPosition(pathProgress_);
            Vector3 forward = movePath_->GetForward(pathProgress_);
            float yaw = std::atan2(forward.x, forward.z);
            float pitch = std::asin(-forward.y);
            object_->SetRotation(Vector3(pitch, yaw, 0.0f));
        } else {
            if (typeName_ == "Asteroid") {
                Vector3 rot = object_->GetRotation();
                rot.x += 0.01f;
                rot.y += 0.02f;
                object_->SetRotation(rot);
            } else if (typeName_ == "Turret") {
                // その場にとどまる
                Vector3 rot = object_->GetRotation();
                // プレイヤーの方向を向く
                Vector3 toPlayer = { playerPos.x - position_.x, playerPos.y - position_.y, playerPos.z - position_.z };
                float yaw = std::atan2(toPlayer.x, toPlayer.z);
                float pitch = std::asin(std::clamp(-toPlayer.y / std::sqrt(toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y + toPlayer.z*toPlayer.z), -1.0f, 1.0f));
                // 補間でゆっくり向く
                rot.x += (pitch - rot.x) * 0.1f;
                rot.y += (yaw - rot.y) * 0.1f;
                object_->SetRotation(rot);
            } else if (typeName_ == "Drone") {
                // プレイヤーに向かってまっすぐ突っ込む
                Vector3 toPlayer = { playerPos.x - position_.x, playerPos.y - position_.y, playerPos.z - position_.z };
                float length = std::sqrt(toPlayer.x*toPlayer.x + toPlayer.y*toPlayer.y + toPlayer.z*toPlayer.z);
                if (length > 0.0f) {
                    position_.x += (toPlayer.x / length) * 0.8f;
                    position_.y += (toPlayer.y / length) * 0.8f;
                    position_.z += (toPlayer.z / length) * 0.8f;
                }
            } else {
                isAutoAI_ = true;
                aiOffset_ = {
                    ((float)rand() / RAND_MAX * 40.0f - 20.0f),   // X: -20 ~ 20 (広げる)
                    ((float)rand() / RAND_MAX * 20.0f - 10.0f),    // Y: -10 ~ 10 (広げる)
                    50.0f + ((float)rand() / RAND_MAX * 50.0f)     // Z: 50 ~ 100 (広げる)
                };
            }
        }
    }

    if (isAutoAI_) {
        // 自律戦闘（AI）モード：プレイヤー（カメラ）の前方を浮遊する
        Vector3 right = { playerForward.z, 0.0f, -playerForward.x };
        
        // 揺れ（スウェイ）を一時的にオフ
        float swayX = 0.0f; // std::sin(pathProgress_ * 10.0f) * 5.0f;
        float swayY = 0.0f; // std::cos(pathProgress_ * 8.0f) * 3.0f;
        pathProgress_ += 0.01f; // スウェイのための時間進行用

        Vector3 targetPos = {
            playerPos.x + playerForward.x * aiOffset_.z + right.x * (aiOffset_.x + swayX),
            playerPos.y + aiOffset_.y + swayY,
            playerPos.z + playerForward.z * aiOffset_.z + right.z * (aiOffset_.x + swayX)
        };

        // 目標座標へなめらかに移動（少し早めについてくるようにする）
        float lerpSpeed = 0.05f;
        position_.x += (targetPos.x - position_.x) * lerpSpeed;
        position_.y += (targetPos.y - position_.y) * lerpSpeed;
        position_.z += (targetPos.z - position_.z) * lerpSpeed;

        // プレイヤーの方を向く
        Vector3 dirToPlayer = { playerPos.x - position_.x, playerPos.y - position_.y, playerPos.z - position_.z };
        float dist = std::sqrt(dirToPlayer.x*dirToPlayer.x + dirToPlayer.z*dirToPlayer.z);
        if (dist > 0.001f) {
            float yaw = std::atan2(-dirToPlayer.x, -dirToPlayer.z);
            // フワフワした動きに合わせて少し機体を傾ける（ロール）とさらに良くなる
            float roll = (targetPos.x - position_.x) * 0.05f;
            object_->SetRotation(Vector3(0.0f, yaw, roll));
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
    hp_--;
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
        if (texIndex != UINT32_MAX) {
            object_->GetModel()->SetTextureIndex(texIndex);
        }
    }
}
