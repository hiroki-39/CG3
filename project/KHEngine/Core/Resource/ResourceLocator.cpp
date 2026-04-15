#include "KHEngine/Core/Resource/ResourceLocator.h"
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cassert>

namespace fs = std::filesystem;

std::string ResourceLocator::Resolve(const std::string& logicalName, ResourceLocator::AssetType type)
{
	// そのまま存在するパスならそれを返す
	try
	{
		fs::path p(logicalName);
		if (p.is_absolute() || logicalName.rfind("resources", 0) == 0 || logicalName.find('/') != std::string::npos || logicalName.find('\\') != std::string::npos)
		{
			if (fs::exists(p))
			{
				return p.string();
			}
		}
	}
	catch (...)
	{
		// 無視して探索へ（存在チェックで例外が起きたら後続で探す）
	}

	// 型ごとの探索パス（必要に応じて調整）
	std::vector<std::string> candidates;
	switch (type)
	{
	case AssetType::Model3D:
		candidates = {
			"resources/3dModels",
			"resources"
		};
		break;
	case AssetType::Texture:
		candidates = {
			"resources/textures",
			"resources/sprites",
			"resources/ui",
			"resources"
		};
		break;
	case AssetType::Audio:
		candidates = {
			"resources/audio",
			"resources/sounds",
			"resources",
		};
		break;
	case AssetType::Sprite:
		candidates = {
			"resources/sprites",
			"resources/textures",
			"resources"
		};
		break;
	default:
		candidates = { "resources" };
		break;
	}

	// 拡張子が無ければワイルドカードとして一般的な拡張子も試す（テクスチャ/音声で有用）
	std::vector<std::string> tryNames;
	tryNames.push_back(logicalName);

	// もし拡張子がない場合は典型的な候補を追加
	auto hasExt = fs::path(logicalName).has_extension();
	if (!hasExt)
	{
		// テクスチャ候補
		if (type == AssetType::Texture || type == AssetType::Sprite)
		{
			tryNames.push_back(logicalName + ".png");
			tryNames.push_back(logicalName + ".jpg");
			tryNames.push_back(logicalName + ".jpeg");
			tryNames.push_back(logicalName + ".bmp");
		}
		// モデル候補
		if (type == AssetType::Model3D)
		{
			tryNames.push_back(logicalName + ".obj");
			tryNames.push_back(logicalName + ".gltf");
			tryNames.push_back(logicalName + ".fbx");
		}
		// 音声候補
		if (type == AssetType::Audio)
		{
			tryNames.push_back(logicalName + ".mp3");
			tryNames.push_back(logicalName + ".wav");
			tryNames.push_back(logicalName + ".ogg");
		}
	}

	// basename（例: "plane.obj" -> "plane"）を用意
	std::string baseName = fs::path(logicalName).stem().string();

	// 探索
	for (const auto& dir : candidates)
	{
		for (const auto& name : tryNames)
		{
			fs::path cand = fs::path(dir) / name;
			if (fs::exists(cand))
			{
				return cand.string();
			}
		}

		// モデルは「ディレクトリ/モデル名/ファイル名」の構造で置くことが多いので試す
		if (type == AssetType::Model3D)
		{
			for (const auto& name : tryNames)
			{
				fs::path nested = fs::path(dir) / baseName / name;
				if (fs::exists(nested))
				{
					return nested.string();
				}
			}
		}
	}

	// 見つからなかったら空文字
	return std::string();
}