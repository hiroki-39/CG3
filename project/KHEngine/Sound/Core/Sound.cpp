#include "Sound.h"
#include <cstring>

void Sound::SoundPlayWave(IXAudio2* xAudio2, const SoundManager::SoundData& soundData)
{
	HRESULT result;

	soundData_ = &soundData;

	// source voice の生成（フォーマットを指定）
	result = xAudio2->CreateSourceVoice(&sourceVoice_, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	// std::vector を使っているので data() / size() を使う
	buf.pAudioData = soundData.buffer.empty() ? nullptr : soundData.buffer.data();
	buf.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// バッファが無ければ再生しない
	assert(buf.pAudioData != nullptr && buf.AudioBytes > 0);

	// 波形データの再生
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