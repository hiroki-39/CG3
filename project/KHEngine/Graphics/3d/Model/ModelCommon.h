#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"

class ModelCommon
{
public://　メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	// --- Getter ---
	DirectXCommon* GetDirectXCommon() const { return dxCommon_; }

private://　メンバ変数
	
	DirectXCommon* dxCommon_;

};

