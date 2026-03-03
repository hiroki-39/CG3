#define NOMINMAX
#include "GamePlayScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include <memory>

void GamePlayScene::Initialize()
{
    // フレームワーク共通オブジェクトを取得
    auto services = EngineServices::GetInstance();
    auto dxCommon = services->GetDirectXCommon();
    auto texManager = TextureManager::GetInstance();
    auto spriteCommon = services->GetSpriteCommon();

    // テクスチャ読み込み（バッチ処理）
    if (dxCommon && texManager)
    {
        dxCommon->BeginTextureUploadBatch();
        texManager->LoadTexture("resources/uvChecker.png");
        texManager->ExecuteUploadCommands();
    }

    uint32_t uvCheckerTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/uvChecker.png");

    // 既存スプライトをクリアして確実に1つだけ作る
    sprites.clear();

    auto s = std::make_unique<Sprite>();
    s->Initialize(spriteCommon, uvCheckerTex);
    // 初期位置 (100,100)
    s->SetPosition(Vector2(100.0f, 100.0f));
    s->SetSize(Vector2(128.0f, 128.0f));
    s->SetAnchorPoint(Vector2(0.5f, 0.5f));
    s->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    AddSprite(std::move(s));
}

void GamePlayScene::Finalize()
{
    // BaseScene::Finalize() でスプライトをクリア
    BaseScene::Finalize();
}

void GamePlayScene::Update()
{
    // スプライトの更新
    UpdateSprites();

#ifdef USE_IMGUI
    // --- Sprite ウィンドウ ---
    ImGui::Begin("Sprite");
    if (!sprites.empty())
    {
        Sprite* s = sprites[0].get();

        bool display = isDisplaySprite;
        if (ImGui::Checkbox("Display Sprite", &display))
        {
            isDisplaySprite = display;
        }

        Vector2 pos = s->GetPosition();
        float posArr[2] = { pos.x, pos.y };
        if (ImGui::DragFloat2("Position", posArr, 1.0f, -9999.9f, 9999.9f, "%04.1f"))
        {
            s->SetPosition(Vector2(posArr[0], posArr[1]));
        }

        Vector2 size = s->GetSize();
        float sizeArr[2] = { size.x, size.y };
        if (ImGui::DragFloat2("Size", sizeArr, 1.0f, 1.0f, 4096.0f))
        {
            s->SetSize(Vector2(sizeArr[0], sizeArr[1]));
        }

        float rotation = s->GetRotation();
        if (ImGui::DragFloat("Rotation", &rotation, 0.5f))
        {
            s->SetRotation(rotation);
        }

        Vector4 col = s->GetColor();
        float colArr[4] = { col.x, col.y, col.z, col.w };
        if (ImGui::ColorEdit4("Color", colArr))
        {
            s->SetColor(Vector4(colArr[0], colArr[1], colArr[2], colArr[3]));
        }
    }
    ImGui::End();
#endif // USE_IMGUI
}

void GamePlayScene::Draw()
{
    // BaseScene のヘルパーでスプライト描画
    DrawSprites();
}