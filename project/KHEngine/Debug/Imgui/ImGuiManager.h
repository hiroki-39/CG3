#pragma once

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#endif // USE_IMGUI
#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"

// imguiの管理
class ImGuiManager
{
public:

	/// <summary>
	/// ImGuiの初期化 
	/// </summary>
	void Initialize(DirectXCommon* dxcommon, WinApp* winApp);

	/// <summary>
	/// Imgui受付開始
	/// </summary>
	void Begin();

	/// <summary>
	/// Imgui受付終了
	/// </summary>
	void End();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

private:
	WinApp* winApp_ = nullptr;

	DirectXCommon* dxCommon_;
};

