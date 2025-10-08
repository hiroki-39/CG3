#include "DirectXCommon.h"

void DirectXCommon::Intialaize()
{
	/*--- DirectXの初期化 ---*/

// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;

	/*HRESULTはWindows系のエラーコードであり
	 関数が成功したかSUCCEEDEDマクロで判定できる*/
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	HRESULT result;

	/*初期化の根本的な部分でエラーが出た場合ｈプログラムが間違っているか、
	どうにも出来ない場合が多いのでassertにしておく*/
	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数。
	Microsoft::WRL::ComPtr <IDXGIAdapter4> useAdapter = nullptr;

	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i)
	{
		//アダプタの情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		//ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
		{
			//採用したアダプタの情報をログに出力
			Log(logStream, ConvertString(std::format(
				L"Use Adapater: {}\n", adapterDesc.Description)));
			break;
		}
		//ソフトウェアアダプタは見なかったことにする
		useAdapter = nullptr;
	}

	//アダプタが見つからなかった場合
	assert(useAdapter != nullptr);

	/*--- D3D12Deviceの生成 ---*/

	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};

	const char* featureLevelStrings[] =
	{
		"12.2",
		"12.1",
		"12.0",
	};

	//機能レベルを順に試す
	for (int i = 0; i < _countof(featureLevels); ++i)
	{
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(
			useAdapter.Get(),                     //アダプタ
			featureLevels[i],               //機能レベル
			IID_PPV_ARGS(&device));         //デバイスのポインタ

		//指定した機能レベルでデバイスが生成できたか確認
		if (SUCCEEDED(hr))
		{
			//機能レベルが採用されたので、ループを抜ける
			Log(logStream, std::format("FeatureLevel: {} \n", featureLevelStrings[i]));
			break;
		}
	}

	//デバイスが生成できなかった場合
	assert(device != nullptr);
	//初期化完了のメッセージを出力
	Log(logStream, "Complete create D3D12Device!!!!\n");

	//キー入力の初期化

	//ポインタ
	Input* input = nullptr;

	//入力の初期化
	input = new Input();
	input->Initialize(winApp);

#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		//ヤバイエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] =
		{
			//デバッグレイヤーの警告
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		};

		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			//警告
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		//指定したメッセージの表示を抑制
		infoQueue->AddStorageFilterEntries(&filter);


	}

#endif 

	//コマンドキューの生成
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	//コマンドキューの生成に失敗した場合起動できない
	assert(SUCCEEDED(hr));

	//コマンドアロケータの生成
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));

	//コマンドリストの生成
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	//コマンドリストの生成に失敗した場合起動できない
	assert(SUCCEEDED(hr));

	//スワップチェーンの生成
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	//ウィンドウの幅
	swapChainDesc.Width = WinApp::kClientWidth;
	//ウィンドウの高さ
	swapChainDesc.Height = WinApp::kClientHeight;
	//色の形式
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//マルチサンプルしない
	swapChainDesc.SampleDesc.Count = 1;
	//描画をターゲットとして利用
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//ダブルバッファ
	swapChainDesc.BufferCount = 2;
	//モニタにうつしたら、中身を破壊
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	//コマンドキュー、ウィンドウハンドル、設定を渡して生成
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(),     //コマンドキュー
		winApp->GetHwnd(),      //ウィンドウハンドル
		&swapChainDesc,         //スワップチェーンの設定
		nullptr,                //モニタの設定
		nullptr,                //コンシューマーの設定
		reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()) //スワップチェーンのポインタ
	);

	//スワップチェーンの生成に失敗した場合起動できない
	assert(SUCCEEDED(hr));

	//RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	//SRV用のヒープでディスクリプタの数は128。RTVはShader内で触るものではないので、ShaderVisibleはtrue
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	//DSV用のヒープでディスクリプタの数は1。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	//SwapChainからResourceを引っ張ってくる
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	//失敗した場合起動できない
	assert(SUCCEEDED(hr));

	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	//失敗した場合起動できない
	assert(SUCCEEDED(hr));

	//RTVの生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	//出力結果をSRGBに変換して書き込む
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//2Dテクスチャとして書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
		rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//RTVを2つ作るので、ディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	//1つ目作成
	rtvHandles[0] = rtvHandle;
	device->CreateRenderTargetView(
		swapChainResources[0].Get(), //リソース
		&rtvDesc,              //RTVの設定
		rtvHandles[0]);        //RTVのハンドル

	//2つ目のディスクリプタハンドルを得る
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//2つ目作成
	device->CreateRenderTargetView(
		swapChainResources[1].Get(), //リソース
		&rtvDesc,              //RTVの設定
		rtvHandles[1]);        //RTVのハンドル

	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	//0から始まる
	descriptorRange[0].BaseShaderRegister = 0;
	//数は1つ
	descriptorRange[0].NumDescriptors = 1;
	//SRVを使う
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//Offsetを自動計算
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	//初期値0でFanceを作る
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	UINT64 fenceValue = 0;

	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence));

	//フェンスの生成に失敗した場合起動できない
	assert(SUCCEEDED(hr));

	//Fanceのイベントを作成
	HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	//イベントの生成に失敗した場合起動できない
	assert(fenceEvent != nullptr);

	//dxcCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現時点でincludeはしないが、includeに対応する為の設定を行う
	IDxcIncludeHandler* includehandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler);
	assert(SUCCEEDED(hr));

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descripitionRootSignature{};
	descripitionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/*---RootSignature作成---*/
	D3D12_ROOT_PARAMETER rootPrameters[4] = {};
	//CBVを使う
	rootPrameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//prixelShederを使う
	rootPrameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//レジスタ番号0とバインド
	rootPrameters[0].Descriptor.ShaderRegister = 0;

	//CBVを使う
	rootPrameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//VertexShederを使う
	rootPrameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	//レジスタ番号0とバインド
	rootPrameters[1].Descriptor.ShaderRegister = 0;

	//DescriptorTableを使う
	rootPrameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//PixelShaderで使う
	rootPrameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//Tableの中身の配列を指定
	rootPrameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	//Tableで利用する数
	rootPrameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	//CBVを使う
	rootPrameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//Pixelshaderで使う
	rootPrameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//レジスタ番号1を使う
	rootPrameters[3].Descriptor.ShaderRegister = 1;

	//ルートパラメータ配列へのポインタ
	descripitionRootSignature.pParameters = rootPrameters;
	//配列の長さ
	descripitionRootSignature.NumParameters = _countof(rootPrameters);


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
	//バイリニアフィルタ
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//0~1の範囲外をリピート
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//比較しない
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	//ありったけのMipMapを使う
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	//レジスタ番号0を使う
	staticSamplers[0].ShaderRegister = 0;
	//PixelShaderで使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descripitionRootSignature.pStaticSamplers = staticSamplers;
	descripitionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&descripitionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(hr))
	{
		Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	//バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	//inputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//BlenderStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//ブレンディングを有効化
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;


	//RasiterZerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	//裏面を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//shaderをコンパイルする
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = compileshader(L"object3D.VS.hlsl",
		L"vs_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = compileshader(L"object3D.PS.hlsl",
		L"ps_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
	assert(pixelShaderBlob != nullptr);

	//PSOを生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	//RootSignature
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();

	//InputLayout
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;

	//BlenderState
	graphicsPipelineStateDesc.BlendState = blendDesc;

	//RasterizerState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	//VertexShader
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };

	//PixelShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//機能を有効化
	depthStencilDesc.DepthEnable = true;
	//書き込み
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//利用する形状のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//どのように画面に色を打ち込むかの設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//実際に作成
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//ビューポート
	D3D12_VIEWPORT viewport{ };

	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = winApp->kClientWidth;
	viewport.Height = winApp->kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//シザー矩形
	D3D12_RECT scissorRect{ };

	//基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = winApp->kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = winApp->kClientHeight;


	/*-------------- 初期化 --------------*/

	//ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);


	//DescriptorSizeを取得
	const uint32_t desriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t desriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t desriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

}
