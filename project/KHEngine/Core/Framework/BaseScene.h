#pragma once
#include <vector>
#include <memory>
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Core/Services/EngineServices.h"

class SceneManager;

class BaseScene
{
public:
    virtual ~BaseScene() = default;

    
    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void DrawUI() { DrawSprites(); }
    virtual void Finalize()
    {
        ClearSprites();
    }

    virtual void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }

private:
    SceneManager* sceneManager_ = nullptr;

protected:
    
    std::vector<std::unique_ptr<Sprite>> sprites;
    bool isDisplaySprite = true;

    
    void AddSprite(std::unique_ptr<Sprite> s)
    {
        if (s) sprites.emplace_back(std::move(s));
    }

    void UpdateSprites()
    {
        for (auto& s : sprites) if (s) s->Update();
    }

    void DrawSprites()
    {
        auto services = EngineServices::GetInstance();
        auto spriteCommon = services->GetSpriteCommon();
        if (spriteCommon) spriteCommon->SetCommonDrawSetting();

        if (isDisplaySprite)
        {
            for (auto& s : sprites) if (s) s->Draw();
        }
    }

    void ClearSprites()
    {
        
        sprites.clear();
    }

    
    EngineServices* Services() const
    {
        return EngineServices::GetInstance();
    }

    
    SceneManager* GetSceneManager() const { return sceneManager_; }
};
