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

    // ライフサイクル（必要に応じてオーバーライド）
    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw() {}
    virtual void Finalize()
    {
        ClearSprites();
    }

    virtual void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }

private:
    SceneManager* sceneManager_ = nullptr;

protected:
    // シーン共通データ
    std::vector<std::unique_ptr<Sprite>> sprites;
    bool isDisplaySprite = true;
    const float kDeltaTime_ = 1.0f / 60.0f;

    // AddSprite: unique_ptr 受け取り版（所有権を明確化)
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
        // unique_ptr により自動解放
        sprites.clear();
    }

    // EngineServices 取得ヘルパー
    EngineServices* Services() const
    {
        return EngineServices::GetInstance();
    }

    // 派生クラスが SceneManager にアクセスできるようにするアクセサを追加
    SceneManager* GetSceneManager() const { return sceneManager_; }
};