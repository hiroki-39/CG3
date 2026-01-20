#pragma once
#include <xaudio2.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>

#pragma comment(lib,"xaudio2.lib")

class SoundManager
{
public:

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

		// バッファ
		std::vector<BYTE> buffer;
	};

public:
		// サウンドマネージャーの取得
		static SoundManager* GetInstance();
		
		///<summary>
		/// 初期化
		///</summary>
		void Initialize();

		///<summary>
		/// 終了処理
		///</summary>
		void Finalize();

		///<summary>
		/// 音声データ読み込み（WAV 用・未使用）
		///</summary>
		SoundData SoundLoadWave(const char* filename);

		/// <summary>
		/// 音声ファイル読み込み（MP3 等を PCM に変換して返す）
		/// </summary>
		/// <param name="filename">音声ファイル名</param>
		SoundData SoundLoadFile(const std::string& filename);

		///<summary>
		/// 音声データ解放
		///</summary>
		void SoundUnload(SoundData* soundData);

		///<summary>
		/// XAudio2の取得
		/// </summary>
		IXAudio2* GetXAudio2() const;

private:

	// 外部からインスタンス生成させない
	SoundManager() = default;
	~SoundManager() = default;
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

private:

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
};

