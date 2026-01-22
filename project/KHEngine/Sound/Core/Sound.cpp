#include "Sound.h"
#include <cstring>

void Sound::SoundPlayWave(IXAudio2* xAudio2, const SoundManager::SoundData& soundData)
{
	HRESULT result;

	soundData_ = &soundData;

	result = xAudio2->CreateSourceVoice(&sourceVoice_, &soundData.wfex);
	assert(SUCCEEDED(result));

	//再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.buffersize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	//波形データの再生
	sourceVoice_->SubmitSourceBuffer(&buf);
	sourceVoice_->Start();

	loaded_ = true;
}

void Sound::Stop()
{
	if (!sourceVoice_) { return; }

	sourceVoice_->Stop();
	sourceVoice_->FlushSourceBuffers();
	sourceVoice_->DestroyVoice();
	sourceVoice_ = nullptr;
	loaded_ = false;
}