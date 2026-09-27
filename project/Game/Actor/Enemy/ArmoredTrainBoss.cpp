#define NOMINMAX
#include "ArmoredTrainBoss.h"
#include "Game/Actor/Player/Player.h"
#include "Game/Actor/Bullet/EnemyBullet.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include <cmath>
#include <algorithm>

void ArmoredTrainBoss::Initialize(Object3dCommon* object3dCommon, const Vector3& startPos, uint32_t skyboxTexIndex) {
    object3dCommon_ = object3dCommon;
    skyboxTexIndex_ = skyboxTexIndex;
    position_ = startPos;
    isActive_ = true;
    isDefeated_ = false;
    isCoreExposed_ = false;
    defeatTimer_ = 0.0f;

    ModelManager::GetInstance()->LoadModel("cube.obj");
    ModelManager::GetInstance()->LoadModel("collider_cube_enemy.obj");

    carriages_.clear();

    // 0: 先頭重装甲機関車 (Locomotive)
    {
        TrainCarriage car;
        car.name = "Locomotive";
        car.displayName = "先頭装甲機関車 (Main Core)";
        car.modelName = "cube.obj";
        car.scale = { 4.5f, 3.6f, 13.0f };
        car.color = { 0.35f, 0.38f, 0.42f, 1.0f };
        car.lengthOffset = 0.0f;
        car.hp = 80;
        car.maxHp = 80;
        car.attackInterval = 100.0f;
        car.collider.type = "BOX";
        car.collider.center = { 0.0f, 1.8f, 0.0f };
        car.collider.size = { 4.8f, 3.8f, 13.5f };
        carriages_.push_back(std::move(car));
    }

    // 1: 旋回重砲塔車 (Heavy Turret Car)
    {
        TrainCarriage car;
        car.name = "TurretCar";
        car.displayName = "旋回重砲塔車";
        car.modelName = "cube.obj";
        car.scale = { 4.0f, 2.8f, 10.5f };
        car.color = { 0.42f, 0.45f, 0.48f, 1.0f };
        car.lengthOffset = -13.5f;
        car.hp = 40;
        car.maxHp = 40;
        car.attackInterval = 75.0f;
        car.collider.type = "BOX";
        car.collider.center = { 0.0f, 1.4f, 0.0f };
        car.collider.size = { 4.2f, 3.0f, 11.0f };
        carriages_.push_back(std::move(car));
    }

    // 2: 垂直発射ミサイルコンテナ車 (Missile Car)
    {
        TrainCarriage car;
        car.name = "MissileCar";
        car.displayName = "垂直ミサイルコンテナ車";
        car.modelName = "cube.obj";
        car.scale = { 4.0f, 3.2f, 10.5f };
        car.color = { 0.48f, 0.38f, 0.38f, 1.0f };
        car.lengthOffset = -25.5f;
        car.hp = 40;
        car.maxHp = 40;
        car.attackInterval = 130.0f;
        car.collider.type = "BOX";
        car.collider.center = { 0.0f, 1.6f, 0.0f };
        car.collider.size = { 4.2f, 3.4f, 11.0f };
        carriages_.push_back(std::move(car));
    }

    // 3: 動力ジェネレーター車 (Generator Car)
    {
        TrainCarriage car;
        car.name = "GeneratorCar";
        car.displayName = "動力ジェネレーター車 (サブコア)";
        car.modelName = "cube.obj";
        car.scale = { 3.8f, 2.6f, 9.5f };
        car.color = { 0.32f, 0.45f, 0.55f, 1.0f };
        car.lengthOffset = -37.0f;
        car.hp = 30;
        car.maxHp = 30;
        car.attackInterval = 180.0f;
        car.collider.type = "BOX";
        car.collider.center = { 0.0f, 1.3f, 0.0f };
        car.collider.size = { 4.0f, 2.8f, 10.0f };
        carriages_.push_back(std::move(car));
    }

    // 3Dオブジェクトの初期化
    for (auto& car : carriages_) {
        car.object = std::make_unique<Object3d>();
        car.object->Initialize(object3dCommon_);
        car.object->SetModel(car.modelName);
        car.object->SetScale(car.scale);
        car.object->SetEnvironmentTextureIndex(skyboxTexIndex_);
        car.object->SetColor(car.color);

        car.colliderObject = std::make_unique<Object3d>();
        car.colliderObject->Initialize(object3dCommon_);
        car.colliderObject->SetModel("collider_cube_enemy.obj");
        car.colliderObject->SetScale(car.collider.size);
        car.colliderObject->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
    }

    UpdateCarriageTransforms();
}

void ArmoredTrainBoss::Update(const Vector3& cameraPos, Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets, float gameSpeed) {
    if (!isActive_) return;

    // 撃破演出中
    if (isDefeated_) {
        defeatTimer_ += gameSpeed;
        speed_ = (std::max)(0.0f, speed_ - 0.005f * gameSpeed);
        position_.z += speed_ * gameSpeed;
        UpdateCarriageTransforms();
        return;
    }

    // レールに沿った移動、または直進
    if (rail_) {
        railProgress_ += 0.0003f * speed_ * gameSpeed;
        if (railProgress_ > 1.0f) railProgress_ = 1.0f;
        position_ = rail_->GetPosition(railProgress_);
        // レールの接線ベクトルから向きを計算
        Vector3 nextP = rail_->GetPosition((std::min)(1.0f, railProgress_ + 0.01f));
        Vector3 forward = { nextP.x - position_.x, nextP.y - position_.y, nextP.z - position_.z };
        float fLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
        if (fLen > 0.0001f) {
            rotation_.y = std::atan2(forward.x, forward.z);
            rotation_.x = std::atan2(-forward.y, fLen);
        }
    } else {
        // デフォルトは手前方向へ前進
        position_.z -= speed_ * gameSpeed;
    }

    // 車両位置の更新
    UpdateCarriageTransforms();

    // コア露出判定（護衛車両が2両以上撃破されたら先頭機関車の装甲がパージ）
    int destroyedCount = 0;
    for (size_t i = 1; i < carriages_.size(); ++i) {
        if (carriages_[i].isDestroyed) destroyedCount++;
    }
    if (destroyedCount >= 2 && !isCoreExposed_) {
        isCoreExposed_ = true;
        // 機関車の発光カラーを赤熱化
        carriages_[0].color = { 1.0f, 0.25f, 0.2f, 1.0f };
        if (carriages_[0].object) {
            carriages_[0].object->SetColor(carriages_[0].color);
        }
    }

    // 各車両の武装攻撃
    ExecuteAttacks(player, enemyBullets, gameSpeed);
}

void ArmoredTrainBoss::UpdateCarriageTransforms() {
    float cy = std::cos(rotation_.y);
    float sy = std::sin(rotation_.y);

    for (auto& car : carriages_) {
        // 先頭からのオフセット座標を回転させて適用
        Vector3 carPos = {
            position_.x + (sy * car.lengthOffset),
            position_.y,
            position_.z + (cy * car.lengthOffset)
        };

        if (car.object) {
            car.object->SetTranslate(carPos);
            car.object->SetRotation(rotation_);
            car.object->Update();
        }

        if (car.colliderObject) {
            Vector3 colPos = {
                carPos.x + car.collider.center.x,
                carPos.y + car.collider.center.y,
                carPos.z + car.collider.center.z
            };
            car.colliderObject->SetTranslate(colPos);
            car.colliderObject->SetRotation(rotation_);
            car.colliderObject->Update();
        }
    }
}

void ArmoredTrainBoss::ExecuteAttacks(Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets, float gameSpeed) {
    if (!player || isDefeated_) return;
    Vector3 playerPos = player->GetTranslate();

    for (size_t i = 0; i < carriages_.size(); ++i) {
        auto& car = carriages_[i];
        if (car.isDestroyed) continue;

        car.attackTimer += gameSpeed;
        if (car.attackTimer >= car.attackInterval) {
            car.attackTimer = 0.0f;
            Vector3 carPos = car.object ? car.object->GetTranslate() : position_;

            if (i == 1) {
                // 砲塔車: プレイヤーへ向けた2連装レーザー
                Vector3 toPlayer = { playerPos.x - carPos.x, playerPos.y - carPos.y, playerPos.z - carPos.z };
                float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
                if (len > 0.0f) {
                    toPlayer = { toPlayer.x / len, toPlayer.y / len, toPlayer.z / len };
                }

                for (int side = -1; side <= 1; side += 2) {
                    auto bullet = std::make_unique<EnemyBullet>();
                    Vector3 spawnP = { carPos.x + side * 1.8f, carPos.y + 2.0f, carPos.z };
                    Vector3 vel = { toPlayer.x * 2.2f, toPlayer.y * 2.2f, toPlayer.z * 2.2f };
                    bullet->Initialize(object3dCommon_, spawnP, vel, false, player);
                    enemyBullets.push_back(std::move(bullet));
                }
            } else if (i == 2) {
                // ミサイル車: 上方発射後にプレイヤーを追尾するホーミングミサイル
                for (int side = -1; side <= 1; side += 2) {
                    auto bullet = std::make_unique<EnemyBullet>();
                    Vector3 spawnP = { carPos.x + side * 1.5f, carPos.y + 3.0f, carPos.z };
                    Vector3 vel = { side * 0.3f, 1.2f, -0.5f }; // 最初は上空へ跳ね上がる
                    bullet->Initialize(object3dCommon_, spawnP, vel, true, player);
                    enemyBullets.push_back(std::move(bullet));
                }
            } else if (i == 0 && isCoreExposed_) {
                // 先頭機関車（コア露出時）: 前方への3方向極太ビーム弾
                Vector3 toPlayer = { playerPos.x - carPos.x, playerPos.y - carPos.y, playerPos.z - carPos.z };
                float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
                if (len > 0.0f) {
                    toPlayer = { toPlayer.x / len, toPlayer.y / len, toPlayer.z / len };
                }

                for (float spread = -0.2f; spread <= 0.21f; spread += 0.2f) {
                    auto bullet = std::make_unique<EnemyBullet>();
                    Vector3 spawnP = { carPos.x, carPos.y + 2.0f, carPos.z + 5.0f };
                    Vector3 vel = { (toPlayer.x + spread) * 2.6f, toPlayer.y * 2.6f, toPlayer.z * 2.6f };
                    bullet->Initialize(object3dCommon_, spawnP, vel, false, player);
                    enemyBullets.push_back(std::move(bullet));
                }
            }
        }
    }
}

void ArmoredTrainBoss::Draw() {
    if (!isActive_) return;
    for (auto& car : carriages_) {
        if (car.object) {
            car.object->Draw();
        }
    }
}

void ArmoredTrainBoss::DrawCollider() {
    if (!isActive_ || isDefeated_) return;
    for (auto& car : carriages_) {
        if (!car.isDestroyed && car.colliderObject) {
            car.colliderObject->Draw();
        }
    }
}

bool ArmoredTrainBoss::CheckCollision(const Sphere& bulletSphere, int* outCarriageIndex) {
    if (!isActive_ || isDefeated_) return false;

    for (int i = 0; i < static_cast<int>(carriages_.size()); ++i) {
        auto& car = carriages_[i];
        if (car.isDestroyed) continue;

        Vector3 carPos = car.object ? car.object->GetTranslate() : position_;
        Vector3 colCenter = { carPos.x + car.collider.center.x, carPos.y + car.collider.center.y, carPos.z + car.collider.center.z };

        // OBB 判定
        Matrix4x4 rotMat = Matrix4x4::RotateY(rotation_.y);
        OBB enemyOBB = CollisionMath::CreateOBB(colCenter, car.collider.size, rotMat);
        if (CollisionMath::IsCollision(bulletSphere, enemyOBB)) {
            if (outCarriageIndex) *outCarriageIndex = i;
            return true;
        }
    }
    return false;
}

bool ArmoredTrainBoss::CheckRaycast(const Ray& ray, float* outDist, int* outCarriageIndex) {
    if (!isActive_ || isDefeated_) return false;

    float nearestDist = 99999.0f;
    int hitIdx = -1;

    for (int i = 0; i < static_cast<int>(carriages_.size()); ++i) {
        auto& car = carriages_[i];
        if (car.isDestroyed) continue;

        Vector3 carPos = car.object ? car.object->GetTranslate() : position_;
        Vector3 colCenter = { carPos.x + car.collider.center.x, carPos.y + car.collider.center.y, carPos.z + car.collider.center.z };

        Matrix4x4 rotMat = Matrix4x4::RotateY(rotation_.y);
        OBB enemyOBB = CollisionMath::CreateOBB(colCenter, car.collider.size, rotMat);

        float dist = 0.0f;
        if (CollisionMath::Raycast(ray, enemyOBB, &dist)) {
            if (dist < nearestDist) {
                nearestDist = dist;
                hitIdx = i;
            }
        }
    }

    if (hitIdx >= 0) {
        if (outDist) *outDist = nearestDist;
        if (outCarriageIndex) *outCarriageIndex = hitIdx;
        return true;
    }
    return false;
}

void ArmoredTrainBoss::OnDamaged(int carriageIndex, int damage) {
    if (carriageIndex < 0 || carriageIndex >= static_cast<int>(carriages_.size())) return;
    auto& car = carriages_[carriageIndex];
    if (car.isDestroyed) return;

    // 先頭機関車はコア未露出時はダメージ半減
    if (carriageIndex == 0 && !isCoreExposed_) {
        damage = (std::max)(1, damage / 2);
    }

    car.hp -= damage;
    if (car.hp <= 0) {
        car.hp = 0;
        car.isDestroyed = true;

        // 破壊された車両は黒煙・煤けた色に変化
        car.color = { 0.15f, 0.15f, 0.15f, 1.0f };
        if (car.object) {
            car.object->SetColor(car.color);
        }

        // 先頭機関車が破壊されたらボス全体が撃破
        if (carriageIndex == 0) {
            isDefeated_ = true;
            // 全車両を黒焦げに
            for (auto& c : carriages_) {
                c.isDestroyed = true;
                c.color = { 0.1f, 0.1f, 0.12f, 1.0f };
                if (c.object) {
                    c.object->SetColor(c.color);
                }
            }
        }
    }
}

float ArmoredTrainBoss::GetTotalHpRate() const {
    int cur = 0;
    int max = 0;
    for (const auto& car : carriages_) {
        cur += car.hp;
        max += car.maxHp;
    }
    return (max > 0) ? static_cast<float>(cur) / max : 0.0f;
}
