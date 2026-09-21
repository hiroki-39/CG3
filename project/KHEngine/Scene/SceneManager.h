#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include "KHEngine/Core/Framework/BaseScene.h"
#include "KHEngine/Scene/AbstractSceneFactory.h"
#include "KHEngine/Math/Vector4.h"

#include "KHEngine/Graphics/Transition/TransitionRenderer.h"

class Sprite;

class SceneManager
{
public:
	enum class TransitionState
	{
		None,            // 通常時
		FadeOut,         // 暗転中（旧シーン稼働中 -> 左右から中央へ閉じる）
		FadeOutHold,     // 暗転完了・完全黒描画確定待ち（真っ黒を画面にPresent）
		Loading,         // 暗転中ロード実行（旧シーン破棄＆新シーン初期化）
		FadeInWait,      // ロード直後のタメ（先行更新と黒画面保持）
		FadeIn           // 明転中（新シーン稼働中 -> 中央から左右端へ開く）
	};

	SceneManager();
	~SceneManager();

	void Update();

	void Draw();
	void DrawUI();

	/// <summary>
	/// シーン変更（トランジション付き）
	/// </summary>
	/// <param name="sceneName">遷移先のシーン名</param>
	/// <param name="fadeDuration">フェードアウト/インにかける時間（秒）</param>
	/// <param name="fadeColor">フェード色（デフォルト黒）</param>
	void ChangeScene(const std::string& sceneName, float fadeDuration = 0.5f, const Vector4& fadeColor = { 0.0f, 0.0f, 0.0f, 1.0f });

	/// <summary>
	/// 遷移中かどうか
	/// </summary>
	bool IsTransitioning() const { return transitionState_ != TransitionState::None; }

	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

private:
	void InitTransition();

private:
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	std::unique_ptr<BaseScene> scene_ = nullptr;
	AbstractSceneFactory* sceneFactory_ = nullptr;

	// トランジション（ルール画像遷移）管理
	TransitionState transitionState_ = TransitionState::None;
	std::string nextSceneName_;
	float fadeDuration_ = 0.6f;
	float fadeTimer_ = 0.0f;
	float transitionProgress_ = 0.0f;
	int holdFrames_ = 0;
	Vector4 fadeColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4 edgeColor_ = { 0.2f, 0.75f, 1.0f, 0.7f }; // 境界発光アクセント

	std::unique_ptr<TransitionRenderer> transitionRenderer_ = nullptr;
	uint32_t ruleTextureIndex_ = UINT32_MAX;
};

