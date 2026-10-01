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

    // 0: 先頭重装甲機関車 (Locomotive) - 巨大要塞機関車
    {
        TrainCarriage car;
        car.name = "Locomotive";
        car.displayName = "先頭装甲機関車 (Main Core)";
        car.modelName = "cube.obj";
        car.baseScale = { 10.0f, 8.5f, 26.0f };
        car.scale = car.baseScale;
        car.color = { 0.35f, 0.38f, 0.42f, 1.0f };
        car.baseLengthOffset = 0.0f;
        car.lengthOffset = 0.0f;
        car.hp = 80;
        car.maxHp = 80;
        car.attackInterval = 100.0f;
        car.collider.type = "BOX";
        car.baseColliderCenter = { 0.0f, 0.0f, 0.0f };
        car.collider.center = car.baseColliderCenter;
        car.baseColliderSize = { 10.5f, 9.0f, 27.0f };
        car.collider.size = car.baseColliderSize;
        carriages_.push_back(std::move(car));
    }

    // 1: 旋回重砲塔車 (Heavy Turret Car)
    {
        TrainCarriage car;
        car.name = "TurretCar";
        car.displayName = "旋回重砲塔車";
        car.modelName = "cube.obj";
        car.baseScale = { 9.0f, 7.0f, 22.0f };
        car.scale = car.baseScale;
        car.color = { 0.42f, 0.45f, 0.48f, 1.0f };
        car.baseLengthOffset = -27.0f;
        car.lengthOffset = car.baseLengthOffset;
        car.hp = 40;
        car.maxHp = 40;
        car.attackInterval = 75.0f;
        car.collider.type = "BOX";
        car.baseColliderCenter = { 0.0f, 0.0f, 0.0f };
        car.collider.center = car.baseColliderCenter;
        car.baseColliderSize = { 9.5f, 7.5f, 23.0f };
        car.collider.size = car.baseColliderSize;
        carriages_.push_back(std::move(car));
    }

    // 2: 垂直発射ミサイルコンテナ車 (Missile Car)
    {
        TrainCarriage car;
        car.name = "MissileCar";
        car.displayName = "垂直ミサイルコンテナ車";
        car.modelName = "cube.obj";
        car.baseScale = { 9.0f, 7.8f, 22.0f };
        car.scale = car.baseScale;
        car.color = { 0.48f, 0.38f, 0.38f, 1.0f };
        car.baseLengthOffset = -52.0f;
        car.lengthOffset = car.baseLengthOffset;
        car.hp = 40;
        car.maxHp = 40;
        car.attackInterval = 130.0f;
        car.collider.type = "BOX";
        car.baseColliderCenter = { 0.0f, 0.0f, 0.0f };
        car.collider.center = car.baseColliderCenter;
        car.baseColliderSize = { 9.5f, 8.2f, 23.0f };
        car.collider.size = car.baseColliderSize;
        carriages_.push_back(std::move(car));
    }

    // 3: 動力ジェネレーター車 (Generator Car)
    {
        TrainCarriage car;
        car.name = "GeneratorCar";
        car.displayName = "動力ジェネレーター車 (サブコア)";
        car.modelName = "cube.obj";
        car.baseScale = { 8.5f, 6.5f, 20.0f };
        car.scale = car.baseScale;
        car.color = { 0.32f, 0.45f, 0.55f, 1.0f };
        car.baseLengthOffset = -76.0f;
        car.lengthOffset = car.baseLengthOffset;
        car.hp = 30;
        car.maxHp = 30;
        car.attackInterval = 180.0f;
        car.collider.type = "BOX";
        car.baseColliderCenter = { 0.0f, 0.0f, 0.0f };
        car.collider.center = car.baseColliderCenter;
        car.baseColliderSize = { 9.0f, 7.0f, 21.0f };
        car.collider.size = car.baseColliderSize;
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

    SetScaleMultiplier(scaleMultiplier_);
    UpdateCarriageTransforms();
}

void ArmoredTrainBoss::SetScaleMultiplier(float scale) {
    scaleMultiplier_ = (std::max)(0.2f, scale);
    for (auto& car : carriages_) {
        car.scale = {
            car.baseScale.x * scaleMultiplier_,
            car.baseScale.y * scaleMultiplier_,
            car.baseScale.z * scaleMultiplier_
        };
        car.lengthOffset = car.baseLengthOffset * scaleMultiplier_;
        car.collider.center = {
            car.baseColliderCenter.x * scaleMultiplier_,
            car.baseColliderCenter.y * scaleMultiplier_,
            car.baseColliderCenter.z * scaleMultiplier_
        };
        car.collider.size = {
            car.baseColliderSize.x * scaleMultiplier_,
            car.baseColliderSize.y * scaleMultiplier_,
            car.baseColliderSize.z * scaleMultiplier_
        };
        if (car.object) {
            car.object->SetScale(car.scale);
        }
        if (car.colliderObject) {
            car.colliderObject->SetScale(car.collider.size);
        }
    }
    UpdateCarriageTransforms();
}

void ArmoredTrainBoss::ResetChaseState() {
    isCloseToPlayer_ = false;
    isOvertaking_ = false;
    currentSpeed_ = 50.0f;
    dynamicLeadDistance_ = desiredLeadDistance_;
    targetDynamicLeadDistance_ = desiredLeadDistance_;
    isShortDistanceOpportunity_ = false;
    leadDistanceChangeTimer_ = 3.5f;
}

void ArmoredTrainBoss::Update(const Vector3& cameraPos, const Vector3& playerForward, float playerSpeed, Player* player, std::list<std::unique_ptr<EnemyBullet>>& enemyBullets, float gameSpeed) {
    if (!isActive_) return;

    // 撃破演出中
    if (isDefeated_) {
        defeatTimer_ += gameSpeed;
        currentSpeed_ = (std::max)(0.0f, currentSpeed_ - 0.8f * gameSpeed);
        if (rail_) {
            float totalLen = (std::max)(1.0f, rail_->GetTotalLength());
            float deltaProg = (currentSpeed_ * (gameSpeed / 60.0f)) / totalLen;
            railProgress_ = (std::min)(1.0f, railProgress_ + deltaProg);
            position_ = rail_->GetPosition(railProgress_);
        } else {
            position_.z -= currentSpeed_ * (gameSpeed / 60.0f);
        }
        UpdateCarriageTransforms();
        return;
    }

    // レール走行・プレイヤー連動チェイス制御
    if (rail_) {
        if (isFollowPlayer_) {
            // プレイヤー位置と向き
            Vector3 playerPos = player ? player->GetWorldPosition() : cameraPos;
            Vector3 toBoss = { position_.x - playerPos.x, position_.y - playerPos.y, position_.z - playerPos.z };
            float straightDist = std::sqrt(toBoss.x * toBoss.x + toBoss.y * toBoss.y + toBoss.z * toBoss.z);

            // 自機前進方向におけるボスの前後相対距離 (内積)
            // 正: ボスが自機の前方にいる / 負: 自機がボスの前にいる(追い抜いた)
            relativeForwardDist_ = toBoss.x * playerForward.x + toBoss.y * playerForward.y + toBoss.z * playerForward.z;

            // ランダム並走距離の短縮サイクル
            if (isCloseToPlayer_ && !isOvertaking_ && isRandomLeadEnabled_) {
                float dtSec = gameSpeed / 60.0f;
                leadDistanceChangeTimer_ -= dtSec;
                if (leadDistanceChangeTimer_ <= 0.0f) {
                    if (!isShortDistanceOpportunity_) {
                        // 基準距離 -> 接近チャンス（並走距離をランダムに短縮！）
                        // 接近の最低ラインを70mに変更（70.0m 〜 88.0m）
                        isShortDistanceOpportunity_ = true;
                        float randFactor = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                        targetDynamicLeadDistance_ = 70.0f + randFactor * 18.0f; // 最低70m、最大約88m
                        // 短縮接近チャンス持続時間: 4.0秒 〜 6.0秒
                        leadDistanceChangeTimer_ = 4.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f;
                    } else {
                        // 接近チャンス終了 -> 基準距離（標準120m）へ復帰
                        isShortDistanceOpportunity_ = false;
                        float randOffset = ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f) * 12.0f;
                        targetDynamicLeadDistance_ = (std::max)(100.0f, desiredLeadDistance_ + randOffset);
                        // 通常巡航持続時間: 4.5秒 〜 7.5秒
                        leadDistanceChangeTimer_ = 4.5f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.0f;
                    }
                }
            } else if (!isRandomLeadEnabled_) {
                targetDynamicLeadDistance_ = desiredLeadDistance_;
            }

            // 目標距離を滑らかに補間（前後の位置取りが不自然に急変しないようにする）
            float distLerpRate = 0.035f * gameSpeed;
            dynamicLeadDistance_ += (targetDynamicLeadDistance_ - dynamicLeadDistance_) * distLerpRate;

            // --- 3段階のチェイス挙動判定 ---
            if (!isCloseToPlayer_) {
                // 【フェーズ1: 出現〜接近】
                // 出現位置が遠いため、プレイヤーに追いつくまで高速で疾走
                targetSpeed_ = playerSpeed + rushSpeedBonus_ + 25.0f;

                // プレイヤーの近場（自機前方 desiredLeadDistance_ 付近、かつ直線距離が近づいた）に到達したら並走へ移行
                if (relativeForwardDist_ >= (desiredLeadDistance_ - 20.0f) && straightDist < 200.0f) {
                    isCloseToPlayer_ = true;
                    leadDistanceChangeTimer_ = 3.5f; // 最初は基準距離で少し並走してから短縮チャンスへ
                }
            } else {
                // 【フェーズ3: 追い抜き判定】
                // プレイヤーがブースト等でボスの前に躍り出た場合 (先頭から見て20m以内または追い抜き)
                if (relativeForwardDist_ < 20.0f) {
                    isOvertaking_ = true;
                } else if (relativeForwardDist_ >= (dynamicLeadDistance_ + 5.0f)) {
                    isOvertaking_ = false;
                }

                if (isOvertaking_) {
                    // 自機より前に出るため進行スピードを急上昇させて追い抜く
                    targetSpeed_ = playerSpeed + rushSpeedBonus_;
                } else {
                    // 【フェーズ2: 並走（シンクロ巡航）】
                    // プレイヤーの近場で自機速度と同程度で並走
                    // dynamicLeadDistance_（基準70mまたはランダム短縮距離）との前後ズレに応じて速度を微調整
                    float distDiff = relativeForwardDist_ - dynamicLeadDistance_;
                    targetSpeed_ = playerSpeed - distDiff * 0.8f;
                    // 急停止しないよう下限を設定 (自機速度の50%以上、かつ最低20m/s)
                    float minSpeed = (std::max)(20.0f, playerSpeed * 0.5f);
                    if (targetSpeed_ < minSpeed) {
                        targetSpeed_ = minSpeed;
                    }
                }
            }

            // 加減速のスムージング（急激な速度変化を抑えて自然な列車走行感を出す）
            float accelRate = (targetSpeed_ > currentSpeed_) ? (2.5f * gameSpeed) : (1.8f * gameSpeed);
            if (currentSpeed_ < targetSpeed_) {
                currentSpeed_ = (std::min)(targetSpeed_, currentSpeed_ + accelRate);
            } else {
                currentSpeed_ = (std::max)(targetSpeed_, currentSpeed_ - accelRate);
            }

            // 実速度(m/s)に基づいて進行度を加算（物理的に瞬間移動せず連続走行）
            float totalLen = (std::max)(1.0f, rail_->GetTotalLength());
            float deltaProg = (currentSpeed_ * (gameSpeed / 60.0f)) / totalLen;
            railProgress_ = (std::clamp)(railProgress_ + deltaProg, 0.0f, 1.0f);

        } else {
            // 固定速度走行モード
            float totalLen = (std::max)(1.0f, rail_->GetTotalLength());
            float deltaProg = (currentSpeed_ * (gameSpeed / 60.0f)) / totalLen;
            railProgress_ = (std::clamp)(railProgress_ + deltaProg, 0.0f, 1.0f);
        }
    } else {
        // レールなし時の直進
        position_.z -= currentSpeed_ * (gameSpeed / 60.0f);
    }

    // 各車両の位置・向き（坂道の斜面傾斜・カーブ）を個別に更新
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

bool ArmoredTrainBoss::GetGroundHeight(float x, float z, float startY, float* outGroundY, Vector3* outNormal) const {
    if (!terrainObjects_ || terrainObjects_->empty()) return false;

    float bestY = -1e9f;
    Vector3 bestNormal = { 0.0f, 1.0f, 0.0f };
    bool found = false;

    for (const auto& obj : *terrainObjects_) {
        if (!obj || !obj->IsCollisionEnabled()) continue;
        float gy = 0.0f;
        Vector3 norm;
        if (obj->RaycastDown(x, z, startY, &gy, &norm)) {
            if (gy > bestY) {
                bestY = gy;
                bestNormal = norm;
                found = true;
            }
        }
    }

    if (found) {
        if (outGroundY) *outGroundY = bestY;
        if (outNormal) *outNormal = bestNormal;
        return true;
    }
    return false;
}

void ArmoredTrainBoss::UpdateCarriageTransforms() {
    float totalLen = rail_ ? (std::max)(1.0f, rail_->GetTotalLength()) : 1.0f;

    for (size_t i = 0; i < carriages_.size(); ++i) {
        auto& car = carriages_[i];

        if (rail_ && rail_->IsValid()) {
            // 各車両のレール上進行度（先頭進行度 + lengthOffset(負の値) / totalLen）
            float carProgress = railProgress_ + (car.lengthOffset / totalLen);

            if (carProgress >= 0.0f && carProgress <= 1.0f) {
                // レール上の正規位置
                car.position = rail_->GetPosition(carProgress);

                // レールの接線ベクトル（前進方向）からピッチ・ヨー角を個別に算出（坂道の斜面に完全平行化）
                Vector3 forward = rail_->GetForward(carProgress);
                float fLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
                if (fLen > 0.0001f) {
                    car.rotation.y = std::atan2(forward.x, forward.z);
                    car.rotation.x = std::atan2(-forward.y, fLen); // 坂道の傾斜角（ピッチ）
                }
                car.rotation.z = rail_->GetTilt(carProgress); // カント（ロール）
            } else if (carProgress < 0.0f) {
                // レール始点の手前にいる場合：始点の接線ベクトルを逆方向に伸ばして自然に配置
                Vector3 startPos = rail_->GetPosition(0.0f);
                Vector3 startForward = rail_->GetForward(0.0f);
                float distBehind = -carProgress * totalLen;
                car.position = {
                    startPos.x - startForward.x * distBehind,
                    startPos.y - startForward.y * distBehind,
                    startPos.z - startForward.z * distBehind
                };
                float fLen = std::sqrt(startForward.x * startForward.x + startForward.z * startForward.z);
                if (fLen > 0.0001f) {
                    car.rotation.y = std::atan2(startForward.x, startForward.z);
                    car.rotation.x = std::atan2(-startForward.y, fLen);
                }
                car.rotation.z = rail_->GetTilt(0.0f);
            } else {
                // レール終点を超えた場合：終点の接線ベクトル方向に自然に延長
                Vector3 endPos = rail_->GetPosition(1.0f);
                Vector3 endForward = rail_->GetForward(1.0f);
                float distAhead = (carProgress - 1.0f) * totalLen;
                car.position = {
                    endPos.x + endForward.x * distAhead,
                    endPos.y + endForward.y * distAhead,
                    endPos.z + endForward.z * distAhead
                };
                float fLen = std::sqrt(endForward.x * endForward.x + endForward.z * endForward.z);
                if (fLen > 0.0001f) {
                    car.rotation.y = std::atan2(endForward.x, endForward.z);
                    car.rotation.x = std::atan2(-endForward.y, fLen);
                }
                car.rotation.z = rail_->GetTilt(1.0f);
            }
        } else {
            // レールなし時のフォールバック（直線追従）
            float cy = std::cos(rotation_.y);
            float sy = std::sin(rotation_.y);
            car.position = {
                position_.x + (sy * car.lengthOffset),
                position_.y,
                position_.z + (cy * car.lengthOffset)
            };
            car.rotation = rotation_;
        }

        // 地面メッシュへの自動吸着（地面レイキャスト）
        if (isGroundSnapEnabled_ && terrainObjects_) {
            float groundY = 0.0f;
            Vector3 groundNorm;
            float searchStartY = car.position.y + 40.0f;
            if (GetGroundHeight(car.position.x, car.position.z, searchStartY, &groundY, &groundNorm)) {
                // 車両の底面 (car.scale.y * 0.5f) が地面高さに密着するよう配置
                car.position.y = groundY + (car.scale.y * 0.5f) + groundSnapOffset_;

                // 前後のサンプリング点による地形勾配（ピッチ角）の精密フィッティング
                float halfLen = (car.scale.z * 0.5f) * 0.8f;
                float frontX = car.position.x + std::sin(car.rotation.y) * halfLen;
                float frontZ = car.position.z + std::cos(car.rotation.y) * halfLen;
                float backX  = car.position.x - std::sin(car.rotation.y) * halfLen;
                float backZ  = car.position.z - std::cos(car.rotation.y) * halfLen;

                float frontGY = 0.0f, backGY = 0.0f;
                bool hasFront = GetGroundHeight(frontX, frontZ, searchStartY, &frontGY);
                bool hasBack  = GetGroundHeight(backX,  backZ,  searchStartY, &backGY);

                if (hasFront && hasBack) {
                    float deltaY = frontGY - backGY;
                    float distXZ = halfLen * 2.0f;
                    car.rotation.x = std::atan2(-deltaY, distXZ);
                }
            }
        }

        // 先頭車両（i == 0）の位置と向きをボスの代表値として同期
        if (i == 0) {
            position_ = car.position;
            rotation_ = car.rotation;
        }

        // 3Dオブジェクトとコライダーの更新
        if (car.object) {
            car.object->SetTranslate(car.position);
            car.object->SetRotation(car.rotation);
            car.object->Update();
        }

        if (car.colliderObject) {
            car.colliderObject->SetTranslate(car.position);
            car.colliderObject->SetRotation(car.rotation);
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
                    Vector3 spawnP = { carPos.x + side * (3.8f * scaleMultiplier_), carPos.y + (4.0f * scaleMultiplier_), carPos.z };
                    Vector3 vel = { toPlayer.x * 2.2f, toPlayer.y * 2.2f, toPlayer.z * 2.2f };
                    bullet->Initialize(object3dCommon_, spawnP, vel, false, player);
                    enemyBullets.push_back(std::move(bullet));
                }
            } else if (i == 2) {
                // ミサイル車: 上方発射後にプレイヤーを追尾するホーミングミサイル
                for (int side = -1; side <= 1; side += 2) {
                    auto bullet = std::make_unique<EnemyBullet>();
                    Vector3 spawnP = { carPos.x + side * (3.5f * scaleMultiplier_), carPos.y + (5.5f * scaleMultiplier_), carPos.z };
                    Vector3 vel = { side * 0.4f, 1.4f, -0.5f }; // 最初は上空へ高く跳ね上がる
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
                    Vector3 spawnP = { carPos.x, carPos.y + (5.0f * scaleMultiplier_), carPos.z + (12.0f * scaleMultiplier_) };
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

        Vector3 carPos = car.object ? car.object->GetTranslate() : car.position;
        Matrix4x4 rotMat = Matrix4x4::MakeAffine({ 1.0f, 1.0f, 1.0f }, car.rotation, { 0.0f, 0.0f, 0.0f });
        Vector3 colCenter = carPos + (rotMat * car.collider.center);

        // OBB 判定
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

        Vector3 carPos = car.object ? car.object->GetTranslate() : car.position;
        Matrix4x4 rotMat = Matrix4x4::MakeAffine({ 1.0f, 1.0f, 1.0f }, car.rotation, { 0.0f, 0.0f, 0.0f });
        Vector3 colCenter = carPos + (rotMat * car.collider.center);

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
