#pragma once

#include <vector>
#include <random>
#include <cstdint>

#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Sound/Core/Sound.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"

class KHFramework
{
public: // メンバ関数
	
	virtual ~KHFramework() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Initialize();

	/// <summary>
	/// 終了処理
	/// </summary>
	virtual void Finalize();

	/// <summary>
	/// 毎フレーム更新処理
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	virtual void Draw() = 0;

	void Run();

	// 終了チェック
	virtual bool IsEndRequest() { return endRequst_; };

protected: // フレームワーク共通で使うメンバ（派生クラスから利用可能）
	// windowsアプリケーションのポインタ
	WinApp* winApp = nullptr;

	// DirectX共通部分のポインタ
	DirectXCommon* dxCommon = nullptr;

	// 入力のポインタ
	Input* input = nullptr;

	// スプライトの共通部分のポインタ
	SpriteCommon* spriteCommon = nullptr;

	// 3Dオブジェクトの共通部分のポインタ
	Object3dCommon* object3dCommon = nullptr;

	// スプライト配列
	std::vector<Sprite*> sprites;

	// 3Dモデルインスタンス配列
	std::vector<Object3d*> modelInstances;

	// SRVマネージャー
	SrvManager* srvManager = nullptr;

	// 再生用オブジェクト（簡易）
	Sound sound;

	// サウンドデータ
	SoundManager::SoundData SoundData = {};

	// ImGuiマネージャー
	ImGuiManager* imguiManager = nullptr;

	// フレーム時間（60fps 固定想定）
	const float kDeltaTime = 1.0f / 60.0f;

	// 乱数生成器の初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine{ seedGenerator() };

private:

	// ゲーム終了要求フラグ
	bool endRequst_ = false;
};

