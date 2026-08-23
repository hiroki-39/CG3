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
    
    auto services = Services();
    if (!services) return;

    auto dxCommon = services->GetDirectXCommon();
    auto spriteCommon = services->GetSpriteCommon();
    auto texManager = TextureManager::GetInstance();
    if (!texManager) return;

    
    if (dxCommon) dxCommon->BeginTextureUploadBatch();

    
    texManager->LoadTexture("monsterBall.png");
    texManager->ExecuteUploadCommands();

    
    uint32_t monsterTex = texManager->GetTextureIndexByFilePath("monsterBall.png");
    
    if (monsterTex == std::numeric_limits<uint32_t>::max())
    {
        
        texManager->ClearIntermediateResources();
        return;
    }

    
    texManager->ClearIntermediateResources();

    
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
    
    UpdateSprites();

    
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
}

void TitleScene::Finalize()
{
    
    BaseScene::Finalize();
}
