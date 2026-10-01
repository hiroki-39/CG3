#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Sound/Core/SoundManager.h"

#ifdef ENABLE_EDITOR
void EngineServices::SetGamePlaying(bool playing)
{
	if (isGamePlaying_ == playing) return;
	isGamePlaying_ = playing;
	
	// サウンドマネージャーへ再生状態変更を通知（BGM/SEの連動）
	if (auto soundMgr = SoundManager::GetInstance())
	{
		soundMgr->OnGamePlayStateChanged(playing);
	}
}
#endif
