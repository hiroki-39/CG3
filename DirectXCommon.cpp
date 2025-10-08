#include "DirectXCommon.h"

void DirectXCommon::Intialaize()
{
	/*--- DirectX�̏����� ---*/

// DXGI�t�@�N�g���[�̐���
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;

	/*HRESULT��Windows�n�̃G���[�R�[�h�ł���
	 �֐�������������SUCCEEDED�}�N���Ŕ���ł���*/
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	HRESULT result;

	/*�������̍��{�I�ȕ����ŃG���[���o���ꍇ���v���O�������Ԉ���Ă��邩�A
	�ǂ��ɂ��o���Ȃ��ꍇ�������̂�assert�ɂ��Ă���*/
	assert(SUCCEEDED(hr));

	//�g�p����A�_�v�^�p�̕ϐ��B
	Microsoft::WRL::ComPtr <IDXGIAdapter4> useAdapter = nullptr;

	//�ǂ����ɃA�_�v�^�𗊂�
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i)
	{
		//�A�_�v�^�̏����擾
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		//�\�t�g�E�F�A�A�_�v�^�łȂ���΍̗p
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
		{
			//�̗p�����A�_�v�^�̏������O�ɏo��
			Log(logStream, ConvertString(std::format(
				L"Use Adapater: {}\n", adapterDesc.Description)));
			break;
		}
		//�\�t�g�E�F�A�A�_�v�^�͌��Ȃ��������Ƃɂ���
		useAdapter = nullptr;
	}

	//�A�_�v�^��������Ȃ������ꍇ
	assert(useAdapter != nullptr);

	/*--- D3D12Device�̐��� ---*/

	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

	//�@�\���x���ƃ��O�o�͗p�̕�����
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

	//�@�\���x�������Ɏ���
	for (int i = 0; i < _countof(featureLevels); ++i)
	{
		//�̗p�����A�_�v�^�[�Ńf�o�C�X�𐶐�
		hr = D3D12CreateDevice(
			useAdapter.Get(),                     //�A�_�v�^
			featureLevels[i],               //�@�\���x��
			IID_PPV_ARGS(&device));         //�f�o�C�X�̃|�C���^

		//�w�肵���@�\���x���Ńf�o�C�X�������ł������m�F
		if (SUCCEEDED(hr))
		{
			//�@�\���x�����̗p���ꂽ�̂ŁA���[�v�𔲂���
			Log(logStream, std::format("FeatureLevel: {} \n", featureLevelStrings[i]));
			break;
		}
	}

	//�f�o�C�X�������ł��Ȃ������ꍇ
	assert(device != nullptr);
	//�����������̃��b�Z�[�W���o��
	Log(logStream, "Complete create D3D12Device!!!!\n");

	//�L�[���͂̏�����

	//�|�C���^
	Input* input = nullptr;

	//���͂̏�����
	input = new Input();
	input->Initialize(winApp);

#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		//���o�C�G���[���Ɏ~�܂�
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//�G���[���Ɏ~�܂�
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//�x�����Ɏ~�܂�
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		//�}�����郁�b�Z�[�W��ID
		D3D12_MESSAGE_ID denyIds[] =
		{
			//�f�o�b�O���C���[�̌x��
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		};

		//�}�����郌�x��
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			//�x��
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		//�w�肵�����b�Z�[�W�̕\����}��
		infoQueue->AddStorageFilterEntries(&filter);


	}

#endif 

	//�R�}���h�L���[�̐���
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	//�R�}���h�L���[�̐����Ɏ��s�����ꍇ�N���ł��Ȃ�
	assert(SUCCEEDED(hr));

	//�R�}���h�A���P�[�^�̐���
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));

	//�R�}���h���X�g�̐���
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	//�R�}���h���X�g�̐����Ɏ��s�����ꍇ�N���ł��Ȃ�
	assert(SUCCEEDED(hr));

	//�X���b�v�`�F�[���̐���
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	//�E�B���h�E�̕�
	swapChainDesc.Width = WinApp::kClientWidth;
	//�E�B���h�E�̍���
	swapChainDesc.Height = WinApp::kClientHeight;
	//�F�̌`��
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//�}���`�T���v�����Ȃ�
	swapChainDesc.SampleDesc.Count = 1;
	//�`����^�[�Q�b�g�Ƃ��ė��p
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//�_�u���o�b�t�@
	swapChainDesc.BufferCount = 2;
	//���j�^�ɂ�������A���g��j��
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	//�R�}���h�L���[�A�E�B���h�E�n���h���A�ݒ��n���Đ���
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(),     //�R�}���h�L���[
		winApp->GetHwnd(),      //�E�B���h�E�n���h��
		&swapChainDesc,         //�X���b�v�`�F�[���̐ݒ�
		nullptr,                //���j�^�̐ݒ�
		nullptr,                //�R���V���[�}�[�̐ݒ�
		reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()) //�X���b�v�`�F�[���̃|�C���^
	);

	//�X���b�v�`�F�[���̐����Ɏ��s�����ꍇ�N���ł��Ȃ�
	assert(SUCCEEDED(hr));

	//RTV�p�̃q�[�v�Ńf�B�X�N���v�^�̐���2�BRTV��Shader���ŐG����̂ł͂Ȃ��̂ŁAShaderVisible��false
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	//SRV�p�̃q�[�v�Ńf�B�X�N���v�^�̐���128�BRTV��Shader���ŐG����̂ł͂Ȃ��̂ŁAShaderVisible��true
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	//DSV�p�̃q�[�v�Ńf�B�X�N���v�^�̐���1�BRTV��Shader���ŐG����̂ł͂Ȃ��̂ŁAShaderVisible��false
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	//SwapChain����Resource�����������Ă���
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	//���s�����ꍇ�N���ł��Ȃ�
	assert(SUCCEEDED(hr));

	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	//���s�����ꍇ�N���ł��Ȃ�
	assert(SUCCEEDED(hr));

	//RTV�̐���
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	//�o�͌��ʂ�SRGB�ɕϊ����ď�������
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//2D�e�N�X�`���Ƃ��ď�������
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//�f�B�X�N���v�^�̐擪���擾
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
		rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//RTV��2���̂ŁA�f�B�X�N���v�^��2�p��
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	//1�ڍ쐬
	rtvHandles[0] = rtvHandle;
	device->CreateRenderTargetView(
		swapChainResources[0].Get(), //���\�[�X
		&rtvDesc,              //RTV�̐ݒ�
		rtvHandles[0]);        //RTV�̃n���h��

	//2�ڂ̃f�B�X�N���v�^�n���h���𓾂�
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//2�ڍ쐬
	device->CreateRenderTargetView(
		swapChainResources[1].Get(), //���\�[�X
		&rtvDesc,              //RTV�̐ݒ�
		rtvHandles[1]);        //RTV�̃n���h��

	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	//0����n�܂�
	descriptorRange[0].BaseShaderRegister = 0;
	//����1��
	descriptorRange[0].NumDescriptors = 1;
	//SRV���g��
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//Offset�������v�Z
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	//�����l0��Fance�����
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	UINT64 fenceValue = 0;

	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence));

	//�t�F���X�̐����Ɏ��s�����ꍇ�N���ł��Ȃ�
	assert(SUCCEEDED(hr));

	//Fance�̃C�x���g���쐬
	HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	//�C�x���g�̐����Ɏ��s�����ꍇ�N���ł��Ȃ�
	assert(fenceEvent != nullptr);

	//dxcCompiler��������
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//�����_��include�͂��Ȃ����Ainclude�ɑΉ�����ׂ̐ݒ���s��
	IDxcIncludeHandler* includehandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler);
	assert(SUCCEEDED(hr));

	//RootSignature�쐬
	D3D12_ROOT_SIGNATURE_DESC descripitionRootSignature{};
	descripitionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/*---RootSignature�쐬---*/
	D3D12_ROOT_PARAMETER rootPrameters[4] = {};
	//CBV���g��
	rootPrameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//prixelSheder���g��
	rootPrameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//���W�X�^�ԍ�0�ƃo�C���h
	rootPrameters[0].Descriptor.ShaderRegister = 0;

	//CBV���g��
	rootPrameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//VertexSheder���g��
	rootPrameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	//���W�X�^�ԍ�0�ƃo�C���h
	rootPrameters[1].Descriptor.ShaderRegister = 0;

	//DescriptorTable���g��
	rootPrameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//PixelShader�Ŏg��
	rootPrameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//Table�̒��g�̔z����w��
	rootPrameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	//Table�ŗ��p���鐔
	rootPrameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	//CBV���g��
	rootPrameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//Pixelshader�Ŏg��
	rootPrameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//���W�X�^�ԍ�1���g��
	rootPrameters[3].Descriptor.ShaderRegister = 1;

	//���[�g�p�����[�^�z��ւ̃|�C���^
	descripitionRootSignature.pParameters = rootPrameters;
	//�z��̒���
	descripitionRootSignature.NumParameters = _countof(rootPrameters);


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
	//�o�C���j�A�t�B���^
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//0~1�͈̔͊O�����s�[�g
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//��r���Ȃ�
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	//�����������MipMap���g��
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	//���W�X�^�ԍ�0���g��
	staticSamplers[0].ShaderRegister = 0;
	//PixelShader�Ŏg��
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descripitionRootSignature.pStaticSamplers = staticSamplers;
	descripitionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	//�V���A���C�Y���ăo�C�i���ɂ���
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&descripitionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(hr))
	{
		Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	//�o�C�i�������ɐ���
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

	//BlenderState�̐ݒ�
	D3D12_BLEND_DESC blendDesc{};
	//�S�Ă̐F�v�f����������
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//�u�����f�B���O��L����
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;


	//RasiterZerState�̐ݒ�
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	//���ʂ�\�����Ȃ�
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	//�O�p�`�̒���h��Ԃ�
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//shader���R���p�C������
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = compileshader(L"object3D.VS.hlsl",
		L"vs_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = compileshader(L"object3D.PS.hlsl",
		L"ps_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
	assert(pixelShaderBlob != nullptr);

	//PSO�𐶐�
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

	//DepthStencilState�̐ݒ�
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//�@�\��L����
	depthStencilDesc.DepthEnable = true;
	//��������
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//DepthStencil�̐ݒ�
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//��������RTV�̏��
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//���p����`��̃^�C�v�B�O�p�`
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//�ǂ̂悤�ɉ�ʂɐF��ł����ނ��̐ݒ�
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//���ۂɍ쐬
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//�r���[�|�[�g
	D3D12_VIEWPORT viewport{ };

	//�N���C�A���g�̈�̃T�C�Y�ƈꏏ�ɂ��ĉ�ʑS�̂ɕ\��
	viewport.Width = winApp->kClientWidth;
	viewport.Height = winApp->kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//�V�U�[��`
	D3D12_RECT scissorRect{ };

	//��{�I�Ƀr���[�|�[�g�Ɠ�����`���\�������悤�ɂ���
	scissorRect.left = 0;
	scissorRect.right = winApp->kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = winApp->kClientHeight;


	/*-------------- ������ --------------*/

	//ImGui�̏�����
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


	//DescriptorSize���擾
	const uint32_t desriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t desriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t desriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

}
