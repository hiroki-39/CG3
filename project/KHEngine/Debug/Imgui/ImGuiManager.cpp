#include "ImGuiManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <filesystem>

void ImGuiManager::Initialize([[maybe_unused]]DirectXCommon* dxcommon, [[maybe_unused]] WinApp* winApp)
{
#ifdef USE_IMGUI

	this->dxCommon_ = dxcommon;
	winApp_ = winApp;

	// ImGui のバージョンチェックとコンテキスト作成
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// IO の設定（キーボード、ゲームパッド、ドッキングなど）
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// スタイル
	ApplyModernDarkTheme();

	// 各種フォントの安全なロード
	auto loadFont = [&](const std::string& path, float size, const ImWchar* glyphRanges) -> ImFont* {
		if (std::filesystem::exists(path)) {
			return io.Fonts->AddFontFromFileTTF(path.c_str(), size, nullptr, glyphRanges);
		}
		std::string altPath = path;
		if (altPath.rfind("resources/", 0) == 0) {
			altPath.replace(0, 10, "Resources/");
		} else if (altPath.rfind("Resources/", 0) == 0) {
			altPath.replace(0, 10, "resources/");
		}
		if (std::filesystem::exists(altPath)) {
			return io.Fonts->AddFontFromFileTTF(altPath.c_str(), size, nullptr, glyphRanges);
		}
		return nullptr;
	};

	// 1. デフォルトUIフォント (YuGothR 14px)
	fonts_[static_cast<size_t>(FontType::Default)] = loadFont(
		"resources/font/YuGothR.ttc",
		14.0f,
		io.Fonts->GetGlyphRangesJapanese()
	);
	if (!fonts_[static_cast<size_t>(FontType::Default)]) {
		fonts_[static_cast<size_t>(FontType::Default)] = io.Fonts->AddFontDefault();
	}

	// 2. 日本語 MPLUS フォント (通常 24px / 大 48px)
	fonts_[static_cast<size_t>(FontType::Japanese_MPLUS)] = loadFont(
		"resources/font/MPLUS1p-Medium.ttf",
		24.0f,
		io.Fonts->GetGlyphRangesJapanese()
	);
	fonts_[static_cast<size_t>(FontType::Japanese_MPLUS_Large)] = loadFont(
		"resources/font/MPLUS1p-Medium.ttf",
		48.0f,
		io.Fonts->GetGlyphRangesJapanese()
	);

	// 3. 英数等幅 FiraMono フォント (スコア・タイマー等用 通常 24px / 大 48px)
	fonts_[static_cast<size_t>(FontType::English_FiraMono)] = loadFont(
		"resources/font/FiraMono-Medium.ttf",
		24.0f,
		io.Fonts->GetGlyphRangesDefault()
	);
	fonts_[static_cast<size_t>(FontType::English_FiraMono_Large)] = loadFont(
		"resources/font/FiraMono-Medium.ttf",
		48.0f,
		io.Fonts->GetGlyphRangesDefault()
	);

	// Win32 側の初期化
	ImGui_ImplWin32_Init(winApp_->GetHwnd());

	// --- DX12 バックエンドの初期化情報を作成 ---
	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = dxCommon_->GetDevice();
	init_info.CommandQueue = dxCommon_->GetCommandQueue();
	init_info.NumFramesInFlight = static_cast<int>(dxCommon_->GetSwapChainResourceNum());
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	init_info.SrvDescriptorHeap = nullptr;

	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*,
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
		{
			SrvManager* srv = SrvManager::GetInstance();
			if (srv == nullptr) return;
			if (!srv->CanAllocate()) return;
			uint32_t idx = srv->Allocate();
			*out_cpu_handle = srv->GetSRVCPUDescriptorHandle(idx);
			*out_gpu_handle = srv->GetSRVGPUDescriptorHandle(idx);
		};

	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)
		{
		};

	// ImGui の DX12 初期化
	ImGui_ImplDX12_Init(&init_info);

#endif // USE_IMGUI
}

void ImGuiManager::Begin()
{
#ifdef USE_IMGUI

	// ImGui のフレーム開始処理
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	/*ImGui::ShowDemoWindow();*/

#endif // USE_IMGUI
}

void ImGuiManager::End()
{
#ifdef USE_IMGUI

	// 描画前準備
	ImGui::Render();

#endif // USE_IMGUI


}

void ImGuiManager::Draw()
{
#ifdef USE_IMGUI

	// デスクリプタヒープの設定
	//ID3D12DescriptorHeap* ppHeaps[] = { srvHeap_.get()};
	//dxCommon_->GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon_->GetCommandList());

#endif // USE_IMGUI
}

void ImGuiManager::Finalize()
{
#ifdef USE_IMGUI

	// ウィンドウレイアウトをディスクに確実に保存
	ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);

	// ImGuiのDX12終了処理
	ImGui_ImplDX12_Shutdown();
	// ImGuiのWin32終了処理
	ImGui_ImplWin32_Shutdown();
	// ImGuiコンテキストの破棄
	ImGui::DestroyContext();

#endif // USE_IMGUI
}

void ImGuiManager::ApplyModernDarkTheme()
{
#ifdef USE_IMGUI
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// --- 幾何学スタイル（角丸・パディング・ボーダー） ---
	style.WindowPadding     = ImVec2(12.0f, 12.0f);
	style.FramePadding      = ImVec2(8.0f, 5.0f);
	style.CellPadding       = ImVec2(6.0f, 4.0f);
	style.ItemSpacing       = ImVec2(8.0f, 6.0f);
	style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
	style.IndentSpacing     = 20.0f;
	style.ScrollbarSize     = 14.0f;
	style.GrabMinSize       = 12.0f;

	style.WindowRounding    = 6.0f;
	style.ChildRounding     = 5.0f;
	style.FrameRounding     = 4.0f;
	style.PopupRounding     = 5.0f;
	style.ScrollbarRounding = 5.0f;
	style.GrabRounding      = 4.0f;
	style.TabRounding       = 5.0f;

	style.WindowBorderSize  = 1.0f;
	style.ChildBorderSize   = 1.0f;
	style.PopupBorderSize   = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.TabBorderSize     = 0.0f;

	// --- プロ仕様のモダンダークカラーパレット ---
	// 背景系 (Deep Charcoal / Slate)
	colors[ImGuiCol_WindowBg]             = ImVec4(0.09f, 0.09f, 0.11f, 0.96f);
	colors[ImGuiCol_ChildBg]              = ImVec4(0.12f, 0.12f, 0.15f, 0.60f);
	colors[ImGuiCol_PopupBg]              = ImVec4(0.11f, 0.11f, 0.14f, 0.98f);
	colors[ImGuiCol_Border]               = ImVec4(0.24f, 0.25f, 0.30f, 0.55f);
	colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	// テキスト
	colors[ImGuiCol_Text]                 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.52f, 0.58f, 1.00f);

	// ヘッダー / タイトルバー
	colors[ImGuiCol_TitleBg]              = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgActive]        = ImVec4(0.16f, 0.17f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.08f, 0.08f, 0.10f, 0.75f);
	colors[ImGuiCol_MenuBarBg]            = ImVec4(0.13f, 0.14f, 0.18f, 1.00f);

	// 入力フレーム
	colors[ImGuiCol_FrameBg]              = ImVec4(0.17f, 0.18f, 0.22f, 0.80f);
	colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.23f, 0.25f, 0.32f, 0.90f);
	colors[ImGuiCol_FrameBgActive]        = ImVec4(0.28f, 0.31f, 0.40f, 1.00f);

	// ボタン (上品なアクセント Slate Blue)
	colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.28f, 0.38f, 0.85f);
	colors[ImGuiCol_ButtonHovered]        = ImVec4(0.28f, 0.38f, 0.52f, 1.00f);
	colors[ImGuiCol_ButtonActive]         = ImVec4(0.18f, 0.46f, 0.72f, 1.00f);

	// ヘッダー (CollapsingHeader, Selectable)
	colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.26f, 0.35f, 0.75f);
	colors[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.35f, 0.48f, 0.90f);
	colors[ImGuiCol_HeaderActive]         = ImVec4(0.22f, 0.44f, 0.68f, 1.00f);

	// スライダー・スクロールバー・グラブ
	colors[ImGuiCol_SliderGrab]           = ImVec4(0.26f, 0.59f, 0.98f, 0.90f);
	colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.36f, 0.70f, 1.00f, 1.00f);
	colors[ImGuiCol_CheckMark]            = ImVec4(0.36f, 0.70f, 1.00f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.08f, 0.08f, 0.10f, 0.60f);
	colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.25f, 0.27f, 0.34f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.38f, 0.48f, 0.90f);
	colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.45f, 0.49f, 0.62f, 1.00f);

	// タブ
	colors[ImGuiCol_Tab]                  = ImVec4(0.14f, 0.16f, 0.20f, 0.90f);
	colors[ImGuiCol_TabHovered]           = ImVec4(0.26f, 0.34f, 0.45f, 1.00f);
	colors[ImGuiCol_TabActive]            = ImVec4(0.22f, 0.42f, 0.64f, 1.00f);
	colors[ImGuiCol_TabUnfocused]         = ImVec4(0.11f, 0.12f, 0.16f, 0.80f);
	colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.16f, 0.25f, 0.36f, 0.90f);

	// ドッキング
	colors[ImGuiCol_DockingPreview]       = ImVec4(0.26f, 0.59f, 0.98f, 0.50f);
	colors[ImGuiCol_DockingEmptyBg]       = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);

	// セパレーター & リサイズグリップ
	colors[ImGuiCol_Separator]            = ImVec4(0.24f, 0.26f, 0.32f, 0.70f);
	colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.36f, 0.42f, 0.56f, 0.85f);
	colors[ImGuiCol_SeparatorActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ResizeGrip]           = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

	// プロット・グラフ
	colors[ImGuiCol_PlotLines]            = ImVec4(0.40f, 0.75f, 1.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.36f, 0.70f, 1.00f, 1.00f);
#endif // USE_IMGUI
}

#ifdef USE_IMGUI
ImFont* ImGuiManager::GetFont(FontType type) const
{
	size_t idx = static_cast<size_t>(type);
	if (idx < static_cast<size_t>(FontType::Count) && fonts_[idx] != nullptr)
	{
		return fonts_[idx];
	}
	return fonts_[static_cast<size_t>(FontType::Default)];
}
#endif