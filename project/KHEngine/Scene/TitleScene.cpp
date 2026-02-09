#include "TitleScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"

void TitleScene::Initialize()
{
	// フレームワーク共通オブジェクト取得
	auto services = EngineServices::GetInstance();
	auto dxCommon = services->GetDirectXCommon();
	auto srvManager = services->GetSrvManager();
	auto spriteCommon = services->GetSpriteCommon();
	auto texManager = TextureManager::GetInstance();

	// テクスチャ読み込みバッチ開始
	if (dxCommon) dxCommon->BeginTextureUploadBatch();

	// モンスターボールを読み込む
	texManager->LoadTexture("resources/monsterBall.png");
	texManager->ExecuteUploadCommands();

	// 生インデックス取得
	uint32_t monsterTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/monsterBall.png");

	// 中間リソース解放
	texManager->ClearIntermediateResources();

	// スプライト作成（中央に表示する例）
	if (spriteCommon)
	{
		Sprite* s = new Sprite();
		s->Initialize(spriteCommon, monsterTex);
		// 画面中央付近に配置（必要なら位置は調整）
		s->SetPosition(Vector2(400.0f, 300.0f));
		s->SetSize(Vector2(256.0f, 256.0f));
		s->SetAnchorPoint(Vector2(0.5f, 0.5f));
		s->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		sprites.push_back(s);
	}
}

void TitleScene::Update()
{
	// スプライト更新
	for (auto s : sprites) if (s) s->Update();
}

void TitleScene::Draw()
{
	auto services = EngineServices::GetInstance();
	auto spriteCommon = services->GetSpriteCommon();

	// スプライト共通設定
	if (spriteCommon) spriteCommon->SetCommonDrawSetting();

	// スプライトのみ描画
	if (isDisplaySprite)
	{
		for (auto s : sprites) if (s) s->Draw();
	}
}

void TitleScene::Finalize()
{
	for (auto s : sprites) delete s;
	sprites.clear();
}