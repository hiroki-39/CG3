#define NOMINMAX
#include "GameOverScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Scene/SceneManager.h"
#include <limits>
#include <memory>

void GameOverScene::Initialize()
{
    auto services = Services();
    if (!services) return;

    auto dxCommon = services->GetDirectXCommon();
    auto spriteCommon = services->GetSpriteCommon();
    auto texManager = TextureManager::GetInstance();
    if (!texManager) return;

    if (dxCommon) dxCommon->BeginTextureUploadBatch();

    texManager->LoadTexture("UI/gameover.png");
    texManager->ExecuteUploadCommands();

    uint32_t texIndex = texManager->GetTextureIndexByFilePath("UI/gameover.png");
    if (texIndex == std::numeric_limits<uint32_t>::max())
    {
        texManager->ClearIntermediateResources();
        return;
    }

    texManager->ClearIntermediateResources();

    if (spriteCommon)
    {
        auto s = std::make_unique<Sprite>();
        s->Initialize(spriteCommon, texIndex);
        s->SetPosition(Vector2(640.0f, 360.0f));
        s->SetSize(Vector2(1280.0f, 720.0f));
        s->SetAnchorPoint(Vector2(0.5f, 0.5f));
        s->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        AddSprite(std::move(s)); 
    }
}

void GameOverScene::Update()
{
    UpdateSprites();

    auto services = Services();
    if (!services) return;

    auto input = services->GetInput();
    
    if (input && (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)))
    {
        auto sceneManager = GetSceneManager();
        if (sceneManager)
        {
            sceneManager->ChangeScene("TITLE");
        }
    }
}

void GameOverScene::Draw()
{
    
}

void GameOverScene::Finalize()
{
    BaseScene::Finalize();
}
