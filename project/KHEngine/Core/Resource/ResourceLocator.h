#pragma once
#include <string>

class ResourceLocator
{
public:
	
	enum class AssetType
	{
		Model3D,
		Texture,
		Audio,
		Sprite,
		Unknown
	};

	// 論理名（または相対/絶対パス）を受け取り、実ファイルパスを返す。見つからなければ空文字を返す
	static std::string Resolve(const std::string& logicalName, AssetType type = AssetType::Unknown);
};

