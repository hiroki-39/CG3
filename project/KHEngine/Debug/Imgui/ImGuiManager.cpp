#include "ImGuiManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"

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
	ImGui::StyleColorsDark();

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
	ImGui::ShowDemoWindow();

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

	// ImGuiのDX12終了処理
	ImGui_ImplDX12_Shutdown();
	// ImGuiのWin32終了処理
	ImGui_ImplWin32_Shutdown();
	// ImGuiコンテキストの破棄
	ImGui::DestroyContext();

#endif // USE_IMGUI
}