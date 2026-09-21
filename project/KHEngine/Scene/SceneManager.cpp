#include "SceneManager.h"
#include <cassert>
#include <algorithm>
#include "KHEngine/Graphics/Transition/TransitionRenderer.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/OS/WinApp.h"

namespace
{
	// スムーズステップ (3t^2 - 2t^3): 加減速が滑らかで映画のような美しいトランジションを実現
	inline float SmoothStep(float t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
}

SceneManager::SceneManager() = default;

SceneManager::~SceneManager()
{
	if (scene_)
	{
		scene_->Finalize();
		scene_.reset();
	}
	if (nextScene_)
	{
		nextScene_->Finalize();
		nextScene_.reset();
	}
	transitionRenderer_.reset();
}

void SceneManager::InitTransition()
{
	if (transitionRenderer_) return;

	auto dxCommon = EngineServices::GetInstance()->GetDirectXCommon();
	if (!dxCommon) return;

	transitionRenderer_ = std::make_unique<TransitionRenderer>();
	transitionRenderer_->Initialize(dxCommon);

	// ルールテクスチャのインデックス取得
	const std::string rulePath = "resources/textures/rules/rule_horizontal.png";
	auto texManager = TextureManager::GetInstance();
	ruleTextureIndex_ = texManager->GetTextureIndexByFilePath(rulePath);
	if (ruleTextureIndex_ == UINT32_MAX || ruleTextureIndex_ == 0)
	{
		texManager->LoadTexture(rulePath);
		ruleTextureIndex_ = texManager->GetTextureIndexByFilePath(rulePath);
	}
}

void SceneManager::Update()
{
	float dt = EngineServices::GetInstance()->GetDeltaTime();
	if (dt <= 0.0f)
	{
		dt = 1.0f / 60.0f;
	}

	switch (transitionState_)
	{
	case TransitionState::None:
		if (scene_)
		{
			scene_->Update();
		}
		break;

	case TransitionState::FadeOut:
		// 旧シーンを通常更新（アニメーションや演出が止まらない）
		if (scene_)
		{
			scene_->Update();
		}

		fadeTimer_ += dt;
		{
			float t = std::clamp(fadeTimer_ / fadeDuration_, 0.0f, 1.0f);
			transitionProgress_ = SmoothStep(t);
		}

		if (fadeTimer_ >= fadeDuration_)
		{
			// 左右端から中央まで閉じた（完全暗転）
			transitionProgress_ = 1.0f;
			transitionState_ = TransitionState::FadeOutHold;
			holdFrames_ = 0;
		}
		break;

	case TransitionState::FadeOutHold:
		// 完全な黒画面が画面（フロントバッファ・バックバッファ）に描画・表示されるのを待つ
		transitionProgress_ = 1.0f;

		holdFrames_++;
		if (holdFrames_ >= 2)
		{
			// 画面が完全に閉じた状態で、ロードステートへ
			transitionState_ = TransitionState::Loading;
		}
		break;

	case TransitionState::Loading:
		// 画面は100%真っ黒。この裏で新シーンを生成・初期化
		transitionProgress_ = 1.0f;

		if (scene_)
		{
			scene_->Finalize();
			scene_.reset();
		}

		assert(sceneFactory_);
		scene_ = sceneFactory_->CreateScene(nextSceneName_);
		if (scene_)
		{
			scene_->SetSceneManager(this);
			scene_->Initialize();
		}

		// ロード完了：ロード時の重いフレーム（0.6秒など）によるdeltaTime急増を吸収するため、ウェイト状態へ
		transitionState_ = TransitionState::FadeInWait;
		holdFrames_ = 0;
		break;

	case TransitionState::FadeInWait:
		// 暗転中に新シーンの初期配置（行列計算・カメラ更新等）を1フレーム先行更新
		transitionProgress_ = 0.0f; // アウト（開く）の開始点: 0.0（全閉）から中央が開く

		if (scene_)
		{
			scene_->Update();
		}

		holdFrames_++;
		if (holdFrames_ >= 2)
		{
			// アウト（明転・開く）開始
			transitionState_ = TransitionState::FadeIn;
			fadeTimer_ = 0.0f;
			transitionProgress_ = 0.0f;
		}
		break;

	case TransitionState::FadeIn:
		// フェードイン中は新シーンを更新
		if (scene_)
		{
			scene_->Update();
		}

		fadeTimer_ += dt;
		{
			float t = std::clamp(fadeTimer_ / fadeDuration_, 0.0f, 1.0f);
			transitionProgress_ = SmoothStep(t);
		}

		if (fadeTimer_ >= fadeDuration_)
		{
			// 中央から左右端へ完全に開いた（遷移完了）
			transitionState_ = TransitionState::None;
			transitionProgress_ = 0.0f;
		}
		break;
	}
}

void SceneManager::Draw()
{
	if (scene_)
	{
		scene_->Draw();
	}
}

void SceneManager::DrawUI()
{
	if (scene_)
	{
		scene_->DrawUI();
	}

	// ルール画像トランジション（最前面）描画
	if (transitionState_ != TransitionState::None && transitionRenderer_ && ruleTextureIndex_ != UINT32_MAX)
	{
		bool isOpening = (transitionState_ == TransitionState::FadeIn || transitionState_ == TransitionState::FadeInWait);
		float prog = transitionProgress_;

		// 完全暗転ホールド中またはロード中は確実に全閉（prog = 1.0f, isOpening = false）
		if (transitionState_ == TransitionState::FadeOutHold || transitionState_ == TransitionState::Loading)
		{
			prog = 1.0f;
			isOpening = false;
		}

		transitionRenderer_->Draw(ruleTextureIndex_, prog, isOpening, fadeColor_, 0.06f, edgeColor_);
	}
}

void SceneManager::ChangeScene(const std::string& sceneName, float fadeDuration, const Vector4& fadeColor)
{
	assert(sceneFactory_);

	// 既に遷移中の場合は二重リクエストを無視
	if (transitionState_ != TransitionState::None)
	{
		return;
	}

	// 初回読み込み（起動時等、まだシーンが存在しない場合）
	if (!scene_)
	{
		scene_ = sceneFactory_->CreateScene(sceneName);
		if (scene_)
		{
			scene_->SetSceneManager(this);
			scene_->Initialize();
		}
		transitionState_ = TransitionState::None;
		transitionProgress_ = 0.0f;
		return;
	}

	// 即時切り替え（フェード時間0以下）
	if (fadeDuration <= 0.0f)
	{
		if (scene_)
		{
			scene_->Finalize();
			scene_.reset();
		}
		scene_ = sceneFactory_->CreateScene(sceneName);
		if (scene_)
		{
			scene_->SetSceneManager(this);
			scene_->Initialize();
		}
		transitionState_ = TransitionState::None;
		transitionProgress_ = 0.0f;
		return;
	}

	// イン（左右端から中央へ閉じるトランジション）開始
	nextSceneName_ = sceneName;
	fadeDuration_ = fadeDuration;
	fadeColor_ = fadeColor;
	fadeTimer_ = 0.0f;
	transitionProgress_ = 0.0f;
	holdFrames_ = 0;
	transitionState_ = TransitionState::FadeOut;

	InitTransition();
}
