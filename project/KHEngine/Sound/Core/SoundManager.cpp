#include "SoundManager.h"
#include "KHEngine/Core/Utility/String/StringUtility.h"
#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


//
SoundManager* SoundManager::GetInstance()
{
	static SoundManager instance;
	return &instance;
}

// 
void SoundManager::Initialize()
{
	HRESULT result;

	// 
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));

	// マスターボイスの生成
	result = xAudio2.Get()->CreateMasteringVoice(&masteringVoice);
	assert(SUCCEEDED(result));

	// Media Foundationの初期化
	result = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(result));
}

// 
void SoundManager::Finalize()
{
	HRESULT result;

	// 
	result = MFShutdown();
	assert(SUCCEEDED(result));

	// 
	if (masteringVoice)
	{
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}

	// XAudio2縺ｮ隗｣謾ｾ
	xAudio2.Reset();
}

SoundManager::SoundData SoundManager::SoundLoadWave(const char* filename)
{
	HRESULT result = {};

	/*---縲1. 繝輔ぃ繧､繝ｫ繧帝幕縺 ---*/
	//繝輔ぃ繧､繝ｫ蜈･蜉帙せ繝医Μ繝ｼ繝縺ｮ繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ
	std::ifstream file;

	//.wav繝輔ぃ繧､繝ｫ繧偵ヰ繧､繝翫Μ繝｢繝ｼ繝峨〒髢九￥
	file.open(filename, std::ios_base::binary);

	//縺ｨ繧翫≠縺医★髢九°縺ｪ縺九▲縺溘ｉ豁｢繧√ｋ
	assert(file.is_open());

	/*---縲2. .wav繝��繧ｿ隱ｭ縺ｿ霎ｼ縺ｿ ---*/
	//RIFF繝倥ャ繝繝ｼ縺ｮ隱ｭ縺ｿ霎ｼ縺ｿ
	RiffHeader riff;

	//繝√Ε繝ｳ繧ｯ繝倥ャ繝繝ｼ縺ｮ遒ｺ隱
	file.read((char*)&riff, sizeof(riff));

	//繝輔ぃ繧､繝ｫ縺軍IFF縺九メ繧ｧ繝�け縺吶ｋ
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
	{
		assert(0);
	}

	//繝輔ぃ繧､繝ｫ縺係AVE縺九メ繧ｧ繝�け縺吶ｋ
	if (strncmp(riff.type, "WAVE", 4) != 0)
	{
		assert(0);
	}

	//Format繝√Ε繝ｳ繧ｯ縺ｮ隱ｭ縺ｿ霎ｼ縺ｿ
	FormatChunk format = {};

	//繝√Ε繝ｳ繧ｯ繝倥ャ繝繝ｼ縺ｮ遒ｺ隱
	file.read((char*)&format, sizeof(ChunkHeader));

	//繝輔ぃ繧､繝ｫ縺掲mt縺九メ繧ｧ繝�け縺吶ｋ
	if (strncmp(format.chunk.id, "fmt ", 4) != 0)
	{
		assert(0);
	}

	//繝√Ε繝ｳ繧ｯ譛ｬ菴薙�隱ｭ縺ｿ霎ｼ縺ｿ
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	//Data繝√Ε繝ｳ繧ｯ縺ｮ隱ｭ縺ｿ霎ｼ縺ｿ
	ChunkHeader data;

	//繝√Ε繝ｳ繧ｯ繝倥ャ繝繝ｼ縺ｮ遒ｺ隱
	file.read((char*)&data, sizeof(data));

	//JUNK繝√Ε繝ｳ繧ｯ繧呈､懷�縺励◆蝣ｴ蜷
	if (strncmp(data.id, "JUNK", 4) == 0)
	{
		//隱ｭ縺ｿ蜿悶ｊ菴咲ｽｮ繧谷UNK繝√Ε繝ｳ繧ｯ縺ｮ邨ゅｏ繧翫∪縺ｧ騾ｲ繧√ｋ
		file.seekg(data.size, std::ios_base::cur);

		//蜀崎ｪｭ縺ｿ霎ｼ縺ｿ
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0)
	{
		assert(0);
	}

	//Data繝√Ε繝ｳ繧ｯ縺ｮ繝��繧ｿ驛ｨ(豕｢蠖｢繝��繧ｿ)縺ｮ隱ｭ縺ｿ霎ｼ縺ｿ
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	/*---縲3. 繝輔ぃ繧､繝ｫ繧帝哩縺倥ｋ ---*/
	//Wave繝輔ぃ繧､繝ｫ繧帝哩縺倥ｋ
	file.close();

	/*--- 4. 隱ｭ縺ｿ霎ｼ繧薙□髻ｳ螢ｰ繝��繧ｿ繧池eturn縺吶ｋ ---*/
	//return縺吶ｋ縺溘ａ縺ｮ髻ｳ螢ｰ繝��繧ｿ
	SoundData soundData = {};

	//豕｢蠖｢繝輔か繝ｼ繝槭ャ繝
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

	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->buffersize = 0;

	soundData->wfex = {};
}

IXAudio2* SoundManager::GetXAudio2() const
{
	return xAudio2.Get();
}