#pragma once
#include "KHEngine/Sound/Core/SoundManager.h"
#include <xaudio2.h>
#include <cassert>

class Sound
{
public:

	// サウンドの再生
	void SoundPlayWave(IXAudio2* xAudio2, const SoundManager::SoundData& soundData);

	///<summary>
	// サウンドの停止
	///</summary>
	void Stop();

private:

	const SoundManager::SoundData* soundData_ = nullptr;

	IXAudio2SourceVoice* sourceVoice_ = nullptr;

	bool loaded_ = false;
};