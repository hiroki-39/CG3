#include "SoundManager.h"
#include "KHEngine/Core/Utility/String/StringUtility.h"
#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


// シングルトンインスタンスの取得
SoundManager* SoundManager::GetInstance()
{
	static SoundManager instance;
	return &instance;
}

// 初期化
void SoundManager::Initialize()
{
	HRESULT result;

	// XAudio2の初期化
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));

	// マスターボイスの生成
	result = xAudio2.Get()->CreateMasteringVoice(&masteringVoice);
	assert(SUCCEEDED(result));

	// Media Foundationの初期化
	result = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(result));
}

// 終了処理
void SoundManager::Finalize()
{
	HRESULT result;

	// Media Foundationの終了処理
	result = MFShutdown();
	assert(SUCCEEDED(result));

	// マスターボイスの破棄
	if (masteringVoice)
	{
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}

	// XAudio2の解放
	xAudio2.Reset();
}

SoundManager::SoundData SoundManager::SoundLoadWave(const char* filename)
{
	HRESULT result = {};

	/*---　1. ファイルを開く ---*/
	//ファイル入力ストリームのインスタンス
	std::ifstream file;

	//.wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);

	//とりあえず開かなかったら止める
	assert(file.is_open());

	/*---　2. .wavデータ読み込み ---*/
	//RIFFヘッダーの読み込み
	RiffHeader riff;

	//チャンクヘッダーの確認
	file.read((char*)&riff, sizeof(riff));

	//ファイルがRIFFかチェックする
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
	{
		assert(0);
	}

	//ファイルがWAVEかチェックする
	if (strncmp(riff.type, "WAVE", 4) != 0)
	{
		assert(0);
	}

	//Formatチャンクの読み込み
	FormatChunk format = {};

	//チャンクヘッダーの確認
	file.read((char*)&format, sizeof(ChunkHeader));

	//ファイルがfmtかチェックする
	if (strncmp(format.chunk.id, "fmt ", 4) != 0)
	{
		assert(0);
	}

	//チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	//Dataチャンクの読み込み
	ChunkHeader data;

	//チャンクヘッダーの確認
	file.read((char*)&data, sizeof(data));

	//JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0)
	{
		//読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);

		//再読み込み
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0)
	{
		assert(0);
	}

	//Dataチャンクのデータ部(波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	/*---　3. ファイルを閉じる ---*/
	//Waveファイルを閉じる
	file.close();

	/*--- 4. 読み込んだ音声データをreturnする ---*/
	//returnするための音声データ
	SoundData soundData = {};

	//波形フォーマット
	soundData.wfex = format.fmt;
	////波形データ
	//soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	////波形データのサイズ
	//soundData.buffersize = data.size;

	return soundData;
}

SoundManager::SoundData SoundManager::SoundLoadFile(const std::string& filename)
{
	// フルパスをワイド文字列に変換
	std::wstring wfilename = StringUtility::ConvertString(filename);
	HRESULT result;

	// SourceReaderの作成
	Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
	result = MFCreateSourceReaderFromURL(wfilename.c_str(), nullptr, &pReader);
	assert(SUCCEEDED(result));

	// PCM形式にフォーマット指定する
	Microsoft::WRL::ComPtr<IMFMediaType> pPCMType;
	MFCreateMediaType(&pPCMType);
	pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	result = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());
	assert(SUCCEEDED(result));

	// 実際にセットされたメディアタイプを取得する
	Microsoft::WRL::ComPtr<IMFMediaType> pCurrentType;
	result = pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pCurrentType);

	// Waveフォーマットを取得する
	WAVEFORMATEX* waveFormat = nullptr;
	MFCreateWaveFormatExFromMFMediaType(pCurrentType.Get(), &waveFormat, nullptr);

	// コンテナに格納する音声データ
	SoundData soundData = {};
	if (waveFormat)
	{
		soundData.wfex = *waveFormat;
		CoTaskMemFree(waveFormat);
	}

	// PCMデータのバッファを構築
	while (true)
	{
		Microsoft::WRL::ComPtr<IMFSample> pSample;
		DWORD streamIndex = 0;
		DWORD flags = 0;
		LONGLONG llTimestamp = 0;

		// サンプルの読み込み
		result = pReader->ReadSample(
			MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			0,
			&streamIndex,
			&flags,
			&llTimestamp,
			&pSample);

		if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			break;
		}

		if (pSample)
		{
			Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
			// サンプルに含まれるサウンドデータのバッファを一繋ぎにして取得
			pSample->ConvertToContiguousBuffer(&pBuffer);

			// データ読み取り用ポインタ
			BYTE* pData = nullptr;
			DWORD maxLength = 0;
			DWORD currentLength = 0;
			// バッファ読み込み用にロック
			pBuffer->Lock(&pData, &maxLength, &currentLength);

			// バッファの末尾にデータを追加
			soundData.buffer.insert(soundData.buffer.end(), pData, pData + currentLength);
			pBuffer->Unlock();
		}
	}

	return soundData;
}

void SoundManager::SoundUnload(SoundData* soundData)
{
	soundData->buffer.clear();
	soundData->wfex = {};
}

IXAudio2* SoundManager::GetXAudio2() const
{
	return xAudio2.Get();
}