
import codecs

with codecs.open("Player.cpp", "r", "utf-8") as f:
    content = f.read()

target = """    frontReticle_->SetEnableLighting(false);
    
    if (shouldDrawPlayer) {"""

replace = """    frontReticle_->SetEnableLighting(false);
    frontReticle_->SetSelectLightings(0);
    frontReticle_->SetScale(Vector3(1.7f, 1.7f, 1.7f)); 
    
    // 照準の初期位置（カメラの奥）
    reticlePosition_ = { 0.0f, 0.0f, 40.0f }; 

    // 初期状態としてグレースケールをOFFにしておく
    if (auto pp = EngineServices::GetInstance()->GetPostProcess()) {
        pp->SetEffectActive("Grayscale", false);
    }
    // コライダー可視化用オブジェクト
    ModelManager::GetInstance()->LoadModel("collider_cube_player.obj");
    colliderObject_ = std::make_unique<Object3d>();
    colliderObject_->Initialize(object3dCommon);
    colliderObject_->SetModel("collider_cube_player.obj"); // プレイヤー専用の立方体モデル
    colliderObject_->GetModel()->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色
    colliderObject_->SetEnvironmentCoefficient(0.0f);
    colliderObject_->SetEnableLighting(false);
    colliderObject_->SetScale(colliderSize_); // プレイヤーの当たり判定のサイズ
}

void Player::OnCollision() {
    if (invincibilityTimer_ > 0.0f || isDead_ || isRolling_) return; // 無敵中、死亡時、またはローリング中（回避）は無効
    
    hp_--;
    if (hp_ <= 0) {
        isDead_ = true;
    } else {
        invincibilityTimer_ = 60.0f; // 1秒間無敵
    }
}

void Player::Update(std::list<std::unique_ptr<PlayerBullet>>& bullets, Object3d* parentCamera, float gameSpeed) {
    Move(gameSpeed);
    Attack(bullets, parentCamera, gameSpeed);

    if (invincibilityTimer_ > 0.0f) {
        invincibilityTimer_ -= gameSpeed;
    }

    object_->Update();
    
    if (accessory_) {
        // アクセサリを回転させるデモ
        Vector3 rot = accessory_->GetRotation();
        rot.y += 0.05f * gameSpeed;
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
    if (invincibilityTimer_ > 0.0f) {
        if (std::fmod(invincibilityTimer_ / 4.0f, 2.0f) < 1.0f) {
            shouldDrawPlayer = false;
        }
    }
    
    if (shouldDrawPlayer) {"""

if target in content:
    content = content.replace(target, replace)
    with codecs.open("Player.cpp", "w", "utf-8") as f:
        f.write(content)
    print("Success")
else:
    print("Target not found")

