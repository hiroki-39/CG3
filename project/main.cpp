#include<Windows.h>
#include <string>
#include <format>
#include<dbghelp.h>
#include <strsafe.h>
#include<dxgidebug.h>
#include"Math.h"
#include"externals/DirectXTex/d3dx12.h"
#include<vector>
#include <numbers>
#include <iomanip>
#include<wrl.h>
#include <functional>
#include<array>
#include<xaudio2.h>
#include <unordered_map>
#include <cassert>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib,"dxcompiler.lib")

#pragma comment(lib,"xaudio2.lib")


#include "externals/DirectXTex/DirectXTex.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Core/OS/WinApp.h"
#include <cstdint>
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Core/Graphics/D3DResourceLeakChecker.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Graphics/Resource/TextureManager.h"
#include "KHEngine/Graphics/3d/Model/ModelCommon.h"
#include "KHEngine/Graphics/3d/Model/Model.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"



//チャンクヘッダ
struct ChunkHeader
{
	//チャンク前のID
	char id[4];

	//チャンクサイズ
	int32_t size;
};

//Riffヘッダーチャンク
struct RiffHeader
{
	//RIFF
	ChunkHeader chunk;
	//WAVE
	char type[4];
};

//FMTチャンク
struct FormatChunk
{
	//fmt
	ChunkHeader chunk;
	//波形フォーマット
	WAVEFORMATEX fmt;
};

//音声データ
struct SoundData
{
	//波形フォーマット
	WAVEFORMATEX wfex;

	//バッファの先頭アドレス
	BYTE* pBuffer;

	//バッファのサイズ
	unsigned int buffersize;
};

void Log(std::ostream& os, const std::string& message);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

void CreateWhiteTexture(DirectX::ScratchImage& outImage);

//SoundData SoundLoadWave(const char* filename);
//
////音声データ解放
//void SoundUnload(SoundData* soundData);
//
////音声再生
//void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	D3DResourceLeakChecker leakcheck;

	//COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	//例外発生時にコールバックする関数を指定
	SetUnhandledExceptionFilter(ExportDump);

#pragma region 基盤システムの初期化

	//ポインタ
	WinApp* winApp = nullptr;

	//windowsAPIの初期化
	winApp = new WinApp();
	winApp->Initialize();

#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		//デバックレイヤーの有効化
		debugController->EnableDebugLayer();
		//GPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif 

	// ポインタ
	DirectXCommon* dxCommon = nullptr;

	// DirectX初期化
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	//ModelCommonの初期化
	ModelManager::GetInstance()->Initialize(dxCommon);

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon);

	// 入力のポインタ
	Input* input = nullptr;

	// 入力の初期化
	input = new Input();
	input->Initialize(winApp);

	// スプライトの共通部分のポインタ
	SpriteCommon* spriteCommon = nullptr;

	// スプライトの共通部分の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	Object3dCommon* object3dCommon = nullptr;
	// 3Dオブジェクトの共通部分の初期化
	object3dCommon = new Object3dCommon();
	object3dCommon->Initialize(dxCommon);

	Camera* camera = new Camera();
	camera->SetTranslate({ 0.0f, 0.0f, -20.0f });
	camera->SetRotation({ 0.0f, 0.0f, 0.0f });
	object3dCommon->SetDefaultCamera(camera);

#pragma endregion 


	TextureManager* texManager = TextureManager::GetInstance();

	// テクスチャアップロードの開始
	dxCommon->BeginTextureUploadBatch();

	ModelManager::GetInstance()->LoadModel("plane.obj");

	// 既存スプライト用テクスチャの読み込み
	texManager->LoadTexture("resources/uvChecker.png");
	texManager->LoadTexture("resources/monsterBall.png");
	texManager->LoadTexture("resources/checkerBoard.png");

	// テクスチャアップロードの実行
	texManager->ExecuteUploadCommands();

	// TextureIndex を取得
	uint32_t uvCheckerTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/uvChecker.png");
	uint32_t monsterBallTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/monsterBall.png");
	uint32_t checkerBoardTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/checkerBoard.png");

	// 中間リソースをまとめて解放
	texManager->ClearIntermediateResources();


#pragma region 最初のシーンの初期化

	//スプライトのポインタ
	std::vector<Sprite*> sprites;

	//スプライトの初期化（5個作ってそれぞれ挫折を見られるように設定）
	const int kSpriteCount = 5;
	std::array<uint32_t, 3> textureIndices = { uvCheckerTex, monsterBallTex, checkerBoardTex };
	std::array<const char*, 3> textureNames = { "uvChecker.png", "monsterBall.png", "checkerBoard.png" };

	// 作成して初期設定
	for (int i = 0; i < kSpriteCount; ++i)
	{
		Sprite* s = new Sprite();
		// テクスチャは順繰りに割り当て
		s->Initialize(spriteCommon, textureIndices[i % textureIndices.size()]);

		// 初期位置を見やすく分散
		s->SetPosition(Vector2(50.0f + i * 140.0f, 120.0f + (i % 2) * 140.0f));
		// サイズを共通に（必要に応じて変えてください）
		s->SetSize(Vector2(128.0f, 128.0f));

		// アンカーポイントをバリエーション付与
		switch (i)
		{
		case 0: s->SetAnchorPoint(Vector2(0.0f, 0.0f)); break;   // 左上
		case 1: s->SetAnchorPoint(Vector2(0.5f, 0.5f)); break;   // 中央
		case 2: s->SetAnchorPoint(Vector2(1.0f, 1.0f)); break;   // 右下
		case 3: s->SetAnchorPoint(Vector2(0.0f, 0.5f)); break;   // 左中央
		case 4: s->SetAnchorPoint(Vector2(0.5f, 0.0f)); break;   // 上中央
		}

		// フリップを一部のスプライトで有効化して確認しやすくする
		s->SetFlipX(i == 2 || i == 4);
		s->SetFlipY(i == 1 || i == 3);

		// テクスチャの範囲指定（左上とサイズ）を一部に設定
		// ここではピクセル単位として指定している想定（Spriteの実装に合わせて必要なら正規化値に変更）
		if (i == 3)
		{
			s->SetTextureLeftTop(Vector2(0.0f, 0.0f));
			s->SetTextureSize(Vector2(64.0f, 64.0f));
		}
		else if (i == 4)
		{
			s->SetTextureLeftTop(Vector2(64.0f, 64.0f));
			s->SetTextureSize(Vector2(64.0f, 64.0f));
		}
		// その他はデフォルト（全体）を使用

		sprites.push_back(s);
	}

	// 選択用インデックス（ImGuiで操作するための状態）
	int selectedSpriteIndex = 0;

	// ---------- 複数モデル用に変更 ----------
	const int kModelCount = 3;
	std::vector<Object3d*> modelInstances;
	modelInstances.reserve(kModelCount);
	for (int i = 0; i < kModelCount; ++i)
	{
		Object3d* obj = new Object3d();
		obj->Initialize(object3dCommon);
		obj->SetModel("plane.obj");
		// 位置を少しずらして並べる（見やすさのため）
		obj->SetTranslate(Vector3(float(i) * 2.5f, 0.0f, 0.0f));
		obj->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
		obj->SetScale(Vector3(1.0f, 1.0f, 1.0f));
		modelInstances.push_back(obj);
	}

	// UI 用の編集状態は各インスタンスの現在値を直接取得して編集する方式にする
	bool syncTransforms = false; // 編集を他インスタンスに同期するか
	// ---------- /複数モデル用ここまで ----------
#pragma endregion


	int32_t selectedModel = 0;

	bool isDisplaySprite = true;

	/*---メインループ---*/

	//ゲームループ
	while (true)
	{
		//Windowsのメッセージ処理
		if (winApp->ProcessMessage())
		{
			//ゲームループを抜ける
			break;
		}

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		//入力の更新
		input->Update();


		/*-------------- ↓更新処理ここから↓ --------------*/

		camera->Update();

		/*--- 各モデルの更新処理 ---*/
		for (auto obj : modelInstances)
		{
			obj->Update();
		}

		/*--- Spriteの更新処理 ---*/
		for (uint32_t i = 0; i < 5; i++)
		{
			sprites[i]->Update();
		}


		//開発用UIの処理

		ImGui::Begin("window");



		// スプライト表示・編集のグループ
		if (isDisplaySprite)
		{
			if (ImGui::CollapsingHeader("Sprite"))
			{
				// スプライト一覧と選択
				if (ImGui::CollapsingHeader("Sprite Instances"))
				{
					// スプライト選択
					ImGui::SliderInt("Selected Sprite", &selectedSpriteIndex, 0, int(sprites.size()) - 1);

					// 簡易表示: 各スプライトのテクスチャ名・位置を表示（デバッグ用）
					for (int i = 0; i < (int)sprites.size(); ++i)
					{
						ImGui::Text("Sprite %d: pos=(%.1f,%.1f) size=(%.1f,%.1f) anchor=(%.2f,%.2f) flipX=%d flipY=%d",
							i,
							sprites[i]->GetPosition().x, sprites[i]->GetPosition().y,
							sprites[i]->GetSize().x, sprites[i]->GetSize().y,
							sprites[i]->GetAnchorPoint().x, sprites[i]->GetAnchorPoint().y,
							sprites[i]->IsFlipX() ? 1 : 0, sprites[i]->IsFlipY() ? 1 : 0);
					}
				}

				// 選択中スプライトの詳細編集
				Sprite* cur = sprites[selectedSpriteIndex];
				if (cur)
				{
					ImGui::Separator();
					ImGui::Text("Edit Sprite %d", selectedSpriteIndex);

					// 位置・回転・サイズ（既存のAPIを使う）
					Vector2 pos = cur->GetPosition();
					if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) cur->SetPosition(pos);

					float rot = cur->GetRotation();
					if (ImGui::SliderAngle("Rotation", &rot)) cur->SetRotation(rot);

					Vector2 size = cur->GetSize();
					if (ImGui::DragFloat2("Size", &size.x, 1.0f, 1.0f, 4096.0f)) cur->SetSize(size);

					// アンカーポイント
					Vector2 anchor = cur->GetAnchorPoint();
					if (ImGui::DragFloat2("Anchor (0..1)", &anchor.x, 0.01f, 0.0f, 1.0f))
					{
						cur->SetAnchorPoint(anchor);
					}

					// フリップ
					bool flipX = cur->IsFlipX();
					if (ImGui::Checkbox("Flip X", &flipX)) cur->SetFlipX(flipX);
					bool flipY = cur->IsFlipY();
					if (ImGui::Checkbox("Flip Y", &flipY)) cur->SetFlipY(flipY);

					// テクスチャ範囲（左上ピクセルと幅高さ）
					Vector2 texLeftTop = cur->GetTextureLeftTop();
					Vector2 texSize = cur->GetTextureSize();
					if (ImGui::DragFloat2("Texture LeftTop (px)", &texLeftTop.x, 1.0f, 0.0f, 4096.0f)) cur->SetTextureLeftTop(texLeftTop);
					if (ImGui::DragFloat2("Texture Size (px)", &texSize.x, 1.0f, 1.0f, 4096.0f)) cur->SetTextureSize(texSize);

					// テクスチャ切替
					static int selectedTex = 0;
					if (ImGui::Combo("Texture", &selectedTex, textureNames.data(), (int)textureNames.size()))
					{
						cur->SetTexture(textureIndices[selectedTex]);
					}

					// ボタンで次のテクスチャに切り替える
					if (ImGui::Button("Cycle Texture"))
					{
						int currentTex = selectedTex;
						currentTex = (currentTex + 1) % int(textureIndices.size());
						selectedTex = currentTex;
						cur->SetTexture(textureIndices[selectedTex]);
					}

					// 表示中の値（読み取り専用）
					ImGui::Text("Current texture leftTop=(%.1f,%.1f) size=(%.1f,%.1f)",
						cur->GetTextureLeftTop().x, cur->GetTextureLeftTop().y,
						cur->GetTextureSize().x, cur->GetTextureSize().y);
				}
			}
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Model Controls"))
		{
			ImGui::Text("Instances: %d", (int)modelInstances.size());
			// 選択
			ImGui::SliderInt("Selected Model", &selectedModel, 0, (int)modelInstances.size() - 1);

			// 同期フラグ
			ImGui::Checkbox("Sync transforms to other instances", &syncTransforms);

			// 選択中インスタンスの編集（現在値を取得して編集）
			if (selectedModel >= 0 && selectedModel < (int)modelInstances.size())
			{
				Object3d* curModel = modelInstances[selectedModel];

				Vector3 editTranslate = curModel->GetTranslate();
				Vector3 editRotation = curModel->GetRotation();
				Vector3 editScale = curModel->GetScale();

				bool changed = false;
				if (ImGui::DragFloat3("Position (X,Y,Z)", &editTranslate.x, 0.1f, -10000.0f, 10000.0f))
				{
					curModel->SetTranslate(editTranslate);
					changed = true;
				}
				if (ImGui::DragFloat3("Rotation (X,Y,Z deg)", &editRotation.x, 0.5f, -360.0f, 360.0f))
				{
					curModel->SetRotation(editRotation);
					changed = true;
				}
				if (ImGui::DragFloat3("Scale (X,Y,Z)", &editScale.x, 0.01f, 0.001f, 100.0f))
				{
					curModel->SetScale(editScale);
					changed = true;
				}

				// 変更があれば同期処理（オンの場合）
				if (changed && syncTransforms)
				{
					for (int i = 0; i < (int)modelInstances.size(); ++i)
					{
						if (i == selectedModel) continue;
						modelInstances[i]->SetTranslate(editTranslate);
						modelInstances[i]->SetRotation(editRotation);
						modelInstances[i]->SetScale(editScale);
					}
				}
			}
		}

		ImGui::End();


		/*-------------- ↓描画処理ここから↓ --------------*/

		//ImGuiの内部コマンドを生成
		ImGui::Render();

		dxCommon->PreDraw();

		object3dCommon->SetCommonDrawSetting();

		// 複数インスタンスの描画
		for (auto obj : modelInstances)
		{
			obj->Draw();
		}

		spriteCommon->SetCommonDrawSetting();

		if (isDisplaySprite)
		{
			/*--- Sprite ---*/

			for (uint32_t i = 0; i < 5; i++)
			{
				sprites[i]->Draw();
			}
		}


		//実際のCommandListのImGuiの描画コマンドを積む
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

		dxCommon->PostDraw();

	}

	//ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();


	//XAudio2の解放
	/*xAudio2.Reset();*/

	// モデルマネージャーの解放
	ModelManager::GetInstance()->Finalize();

	// テクスチャマネージャの解放
	TextureManager::GetInstance()->Finalize();

	//入力の解放
	delete input;

	//WindowsAPIの終了処理
	winApp->Finalize();

	//WindowsAPIの解放
	delete winApp;
	winApp = nullptr;

	//DirectX12の解放
	delete dxCommon;

	//スプライトの解放
	delete spriteCommon;

	//スプライトの解放
	for (uint32_t i = 0; i < 5; i++)
	{
		delete sprites[i];
	}

	// モデルインスタンスの解放
	for (auto obj : modelInstances)
	{
		delete obj;
	}
	modelInstances.clear();

	delete object3dCommon;

	////音声データ解放
	//SoundUnload(&soundData1);

	////解放処理
	//CloseHandle(fenceEvent);


	return 0;
}

//---ここから下は関数の実装---//

//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

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



static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{
	//時刻を取得して、時刻を名前に入れたファイルを作成
	SYSTEMTIME time;
	GetLocalTime(&time);

	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH,
		L"./Dumps/%04d-%02d-%02d-%02d-%02d.dmp",
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




void CreateWhiteTexture(DirectX::ScratchImage& outImage)
{
	//白色
	constexpr uint8_t whitePixel[4] = { 255, 255, 255, 255 };

	DirectX::Image img{};

	//Textureの幅
	img.width = 1;
	//Textureの高さ
	img.height = 1;

	img.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	img.pixels = const_cast<uint8_t*>(whitePixel);

	img.rowPitch = 4;

	img.slicePitch = 4;

	outImage.InitializeFromImage(img);
}


//SoundData SoundLoadWave(const char* filename)
//{
//	HRESULT result = {};
//
//	/*---　1. ファイルを開く ---*/
//	//ファイル入力ストリームのインスタンス
//	std::ifstream file;
//
//	//.wavファイルをバイナリモードで開く
//	file.open(filename, std::ios_base::binary);
//
//	//とりあえず開かなかったら止める
//	assert(file.is_open());
//
//	/*---　2. .wavデータ読み込み ---*/
//	//RIFFヘッダーの読み込み
//	RiffHeader riff;
//
//	//チャンクヘッダーの確認
//	file.read((char*)&riff, sizeof(riff));
//
//	//ファイルがRIFFかチェックする
//	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//ファイルがWAVEかチェックする
//	if (strncmp(riff.type, "WAVE", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//Formatチャンクの読み込み
//	FormatChunk format = {};
//
//	//チャンクヘッダーの確認
//	file.read((char*)&format, sizeof(ChunkHeader));
//
//	//ファイルがfmtかチェックする
//	if (strncmp(format.chunk.id, "fmt ", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//チャンク本体の読み込み
//	assert(format.chunk.size <= sizeof(format.fmt));
//	file.read((char*)&format.fmt, format.chunk.size);
//
//	//Dataチャンクの読み込み
//	ChunkHeader data;
//
//	//チャンクヘッダーの確認
//	file.read((char*)&data, sizeof(data));
//
//	//JUNKチャンクを検出した場合
//	if (strncmp(data.id, "JUNK", 4) == 0)
//	{
//		//読み取り位置をJUNKチャンクの終わりまで進める
//		file.seekg(data.size, std::ios_base::cur);
//
//		//再読み込み
//		file.read((char*)&data, sizeof(data));
//	}
//
//	if (strncmp(data.id, "data", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//Dataチャンクのデータ部(波形データ)の読み込み
//	char* pBuffer = new char[data.size];
//	file.read(pBuffer, data.size);
//
//	/*---　3. ファイルを閉じる ---*/
//	//Waveファイルを閉じる
//	file.close();
//
//	/*--- 4. 読み込んだ音声データをreturnする ---*/
//	//returnするための音声データ
//	SoundData soundData = {};
//
//	//波形フォーマット
//	soundData.wfex = format.fmt;
//	//波形データ
//	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
//	//波形データのサイズ
//	soundData.buffersize = data.size;
//
//	return soundData;
//}
//
////音声データ解放
//void SoundUnload(SoundData* soundData)
//{
//	//バッファのメモリーを解放
//	delete[] soundData->pBuffer;
//
//	soundData->pBuffer = 0;
//	soundData->buffersize = 0;
//	soundData->wfex = {};
//}
//
////音声再生
//void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
//{
//	HRESULT result;
//
//	//波形フォーマットを元にSourceVoiceを生成
//	IXAudio2SourceVoice* pSourceVoice = nullptr;
//
//	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
//	assert(SUCCEEDED(result));
//
//	//再生する波形データの設定
//	XAUDIO2_BUFFER buf{};
//	buf.pAudioData = soundData.pBuffer;
//	buf.AudioBytes = soundData.buffersize;
//	buf.Flags = XAUDIO2_END_OF_STREAM;
//
//	//波形データの再生
//	result = pSourceVoice->SubmitSourceBuffer(&buf);
//	result = pSourceVoice->Start();
//}