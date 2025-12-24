#pragma once
#include<Windows.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<wrl.h>
#include <array>
#include <dxcapi.h>
#include <cassert>
#include <string>

#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/String/StringUtility.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "KHEngine/Core/Utility/Timer/Timer.h"

#include "KHEngine/Core/OS/WinApp.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"

struct UploadBatch
{
	Microsoft::WRL::ComPtr<ID3D12Resource> dst;
	Microsoft::WRL::ComPtr<ID3D12Resource> interm;
	std::vector<D3D12_SUBRESOURCE_DATA> sub;
};

class DirectXCommon
{
public://メンバ関数

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// デバイスの初期化
	/// </summary>
	void InitDevice();

	/// <summary>
	/// コマンド関連の初期化
	/// </summary>
	void InitCommand();

	/// <summary>
	/// スワップチェーンの生成
	/// </summary>
	void CreateSwapChain();

	/// <summary>
	/// 各種デスクリプタヒープの生成
	/// </summary>
	void CreateDescriptorHeaps();

	/// <summary>
	/// レンダーターゲットビューの初期化
	/// </summary>
	void CreateRTV();

	/// <summary>
	/// 深度ステンシルビューの初期化
	/// </summary>
	void CreateDSV();

	/// <summary>
	//フェンスの生成
	/// </summary>
	void CreateFence();

	/// <summary>
	/// ビューポート矩形の初期化
	/// </summary>
	void SetViewportRect();

	/// <summary>
	/// シザー矩形の初期化
	/// </summary>
	void SetScissorRect();

	/// DXCコンパイラの生成
	void CreateDXCCompiler();

	/// <summary>
	/// デスクリプタヒープを生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heaptype, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// 深度ステンシルテクスチャリソースを生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreatDepthStencilTextureResource(int32_t width, int32_t height);

	/// <summary>
	///　ImGuiの初期化
	/// </summary>
	void InitImGui();

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	/// <summary>
	/// 描画後処理
	/// </summary>
	void PostDraw();

	/// <summary>
	/// SRVの指定番号のCPUデスクリプタハンドルを取得
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// SRVの指定番号のGPUデスクリプタハンドルを取得
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);


	/// <summary>
	/// シェーダーのコンパイル
	/// </summary>
	Microsoft::WRL::ComPtr<IDxcBlob> compileshader(const std::wstring& filePath, const wchar_t* profile);

	/// <summary>
	/// バッファリソースの生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInbytes);

	/// <summary>
	/// テクスチャリソースの生成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metdata);

	/// <summary>
	/// テクスチャデータの転送
	/// </summary>
	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages);

	/// <summary>
	/// テクスチャアップロードバッチ開始
	/// </summary>
	void BeginTextureUploadBatch();

	/// <summary>
	/// テクスチャアップロードバッチ完了
	/// </summary>
	void AddTextureUpload(
		Microsoft::WRL::ComPtr<ID3D12Resource> texture,
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediate,
		const std::vector<D3D12_SUBRESOURCE_DATA>& subresources
	);

	/// <summary>
	/// テクスチャアップロードバッチ実行
	/// </summary>
	void ExecuteTextureUploadBatch();

	// --- Getter ---
	ID3D12Device* GetDevice() { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }
	WinApp* GetWinApp() const { return winApp; }

private://静的メンバ関数

	/// <summary>
	/// 指定番号のCPUデスクリプタハンドルを取得
	/// </summary>
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// <summary>
	/// 指定番号のGPUデスクリプタハンドルを取得
	/// </summary>
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

public:
	
	// 最大SRV数　(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount;

private://メンバ変数

	WinApp* winApp = nullptr;

	Timer timer;

	// DirectX12のデバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	// DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

	// コマンドアロケータ
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

	// コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	// コマンドキュー
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

	// スワップチェーン
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;


	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	// SRV用のDescriptorSize
	uint32_t desriptorSizeSRV;

	// RTV用のDescriptorSize
	uint32_t desriptorSizeRTV;

	// DSV用のDescriptorSize
	uint32_t desriptorSizeDSV;

	// RTVのヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;

	// SRVのヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;

	// DSVのヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	//スワップチェーンリソース
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	// RTVを2つ作るので、ディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	// RTVの生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	// RTVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;

	// 深度バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

	// フェンス
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;

	//フェンス用イベントハンドル
	HANDLE fenceEvent;

	// ビューポート
	D3D12_VIEWPORT viewport{ };

	// シザー矩形
	D3D12_RECT scissorRect{ };

	// dxcCompiler
	IDxcUtils* dxcUtils;
	IDxcCompiler3* dxcCompiler;

	// includeHandler
	IDxcIncludeHandler* includeHandler;

	// TransitionBarrier
	D3D12_RESOURCE_BARRIER barrier{};

	// フェンス値
	UINT64 fenceValue;

	// テクスチャアップロードキュー
	std::vector<UploadBatch> textureUploadQueue_;
	// アップロード用コマンド関連
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadAllocator_;
	// アップロード用コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCommandList_;
};