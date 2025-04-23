#include<Windows.h>
#include<cstdint>
#include<d3d12.h>
#include<dxgi1_6.h>
#include <cassert>
#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include <chrono>
#include<dbghelp.h>
#include <strsafe.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")

void Log(std::ostream& os, const std::string& message);

std::wstring ConvertString(const std::string& str);

std::string ConvertString(const std::wstring& str);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//例外発生時にコールバックする関数を指定
	SetUnhandledExceptionFilter(ExportDump);

	//ログのディレクトリを用意
	std::filesystem::create_directory("log");

	//現在時刻を取得(UTC時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	//ログファイルの名前にコンマ何秒はいらないので、秒のみにする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	//日本時間(pcの設定時間)に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
	//format関数で、年月日_時分秒の文字列に変換
	std::string datestring = std::format("{:%Y%m%d_%H%M%S}", localTime);
	//時刻を使ってファイル名を作成
	std::string logFilePath = std::format("log/") + datestring + ".log";
	//ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);



	/*---ウィンドウクラスの登録---*/
	WNDCLASS wc{};
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス名
	wc.lpszClassName = L"CG2MyWindowClass";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスの登録
	RegisterClass(&wc);

	//クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//ウィンドサイズを表す構造体にクライアント領域のサイズを入れる
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	//クライアント領域のサイズをウィンドウサイズに変換する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,     //クラス名
		L"CG2_LE2C_07_カトウ_ヒロキ",       //タイトルバー名
		WS_OVERLAPPEDWINDOW,  //ウィンドウスタイル
		CW_USEDEFAULT,        //表示座X標(Windowsに任せる)
		CW_USEDEFAULT,        //表示Y座標(WindowsOSに任せる)
		wrc.right - wrc.left, //ウィンドウ横幅
		wrc.bottom - wrc.top, //ウィンドウ縦幅
		nullptr,              //親ウィンドウハンドル
		nullptr,              //メニューハンドル
		wc.hInstance,         //インスタンスハンドル
		nullptr               //オプション
	);

	//ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	Log(logStream, "Hello DirectX!\n");
	Log(logStream, ConvertString(std::format(L"clientSize:{},{}\n", kClientWidth, kClientHeight)));

	/*--- DXGIファクトリーの生成 ---*/
	IDXGIFactory7* dxgiFactory = nullptr;

	/*HRESULTはWindows系のエラーコードであり
	 関数が成功したかSUCCEEDEDマクロで判定できる*/
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	/*初期化の根本的な部分でエラーが出た場合ｈプログラムが間違っているか、
	どうにも出来ない場合が多いのでassertにしておく*/
	assert(SUCCEEDED(hr));

	/*--- 使用するアダプタを決定 ---*/
	//使用するアダプタ用の変数。
	IDXGIAdapter4* useAdapter = nullptr;
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
	ID3D12Device* device = nullptr;
	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};

	const char* featureLevelStrings[] = {
		"12.2",
		"12.1",
		"12.0",
	};

	//機能レベルを順に試す
	for (int i = 0; i < _countof(featureLevels); ++i)
	{
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(
			useAdapter,                     //アダプタ
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

	//コマンドキューの生成
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	//コマンドキューの生成に失敗した場合起動できない
	assert(SUCCEEDED(hr));

	//コマンドアロケータの生成
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));

	//コマンドリストの生成
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

	//コマンドリストの生成に失敗した場合起動できない
	assert(SUCCEEDED(hr));

	//スワップチェーンの生成
	IDXGISwapChain1* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	//ウィンドウの幅
	swapChainDesc.Width = kClientWidth;
	//ウィンドウの高さ
	swapChainDesc.Height = kClientHeight;
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
		commandQueue,            //コマンドキュー
		hwnd,                    //ウィンドウハンドル
		&swapChainDesc,         //スワップチェーンの設定
		nullptr,                //モニタの設定
		nullptr,                //コンシューマーの設定
		reinterpret_cast<IDXGISwapChain1**>(&swapChain) //スワップチェーンのポインタ
	);

	//スワップチェーンの生成に失敗した場合起動できない
	assert(SUCCEEDED(hr));


	MSG msg{};

	//ウィンドウのxボタンが押されるまでループ
	while (msg.message != WM_QUIT)
	{
		//windowsにメッセージが来たら最優先で処理する
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else
		{
			//ゲームの更新処理


			//ゲームの描画処理
		}
	}

	return 0;
}

//---ここから下は関数の実装---//

//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//出力ウィンドウにメッセージを出力する
void Log(std::ostream& os, const std::string& message)
{
	os << message << std::endl;

	//標準出力にメッセージを出力
	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	//時刻を取得して、時刻を名前に入れたファイルを作成
	SYSTEMTIME time;
	GetLocalTime(&time);

	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-02d%02d-%02d%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);

	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ |
		GENERIC_WRITE, FILE_SHARE_WRITE |
		FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	//processIdとクラッシュの発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	//設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = true;

	//Dumpを出力
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
		MiniDumpNormal, &minidumpInformation, nullptr, nullptr);


	return EXCEPTION_EXECUTE_HANDLER;

}