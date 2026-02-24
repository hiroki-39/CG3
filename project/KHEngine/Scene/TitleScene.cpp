#define NOMINMAX
#include "TitleScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Scene/SceneManager.h"
#include <limits>
#include <memory>

void TitleScene::Initialize()
{
    // Framework 共通オブジェクト取得（BaseScene のヘルパーを使用）
    auto services = Services();
    if (!services) return;

    auto dxCommon = services->GetDirectXCommon();
    auto spriteCommon = services->GetSpriteCommon();
    auto texManager = TextureManager::GetInstance();
    if (!texManager) return;

    // テクスチャ読み込みバッチ開始（DirectXCommon があれば）
    if (dxCommon) dxCommon->BeginTextureUploadBatch();

    // モンスターボールを読み込む
    texManager->LoadTexture("resources/monsterBall.png");
    texManager->ExecuteUploadCommands();

    // 生インデックス取得（失敗チェック）
    uint32_t monsterTex = texManager->GetTextureIndexByFilePath("resources/monsterBall.png");
    // 取得に失敗した場合の保険（実際の失敗値に合わせて調整）
    if (monsterTex == std::numeric_limits<uint32_t>::max())
    {
        // 中間リソースは解放して早期リターン
        texManager->ClearIntermediateResources();
        return;
    }

    // 中間リソース解放
    texManager->ClearIntermediateResources();

    // スプライト作成（spriteCommon が有効ならば追加）
    if (spriteCommon)
    {
        auto s = std::make_unique<Sprite>();
        s->Initialize(spriteCommon, monsterTex);
        s->SetPosition(Vector2(400.0f, 300.0f));
        s->SetSize(Vector2(256.0f, 256.0f));
        s->SetAnchorPoint(Vector2(0.5f, 0.5f));
        s->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        AddSprite(std::move(s)); 
    }
}

void TitleScene::Update()
{
    // BaseScene のヘルパーで共通更新
    UpdateSprites();

    // GamePlaySceneへ遷移（トリガー検出）
    auto services = Services();
    if (!services) return;

    auto input = services->GetInput();
    if (input && input->TriggerKey(DIK_SPACE))
    {
        auto sceneManager = GetSceneManager();
        if (sceneManager)
        {
            sceneManager->ChangeScene("GAMEPLAY");
        }
    }
}

void TitleScene::Draw()
{
    // BaseScene のヘルパーで共通描画（内部で spriteCommon の設定を行う）
    DrawSprites();
}

void TitleScene::Finalize()
{
    // 必要なら派生で追加処理を行った上でベース処理を呼ぶ
    BaseScene::Finalize();
}