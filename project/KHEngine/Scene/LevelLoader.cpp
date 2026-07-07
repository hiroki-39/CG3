#include "LevelLoader.h"
#include <fstream>
#include <iostream>
// json.hppは外部ライブラリなので適宜パスを調整
#include "externals/nlohmann/json.hpp"
#include "KHEngine/Core/Utility/Log/Logger.h"

// nlohmann::json のエイリアス
using json = nlohmann::json;

void LevelLoader::ParseObject(const void* jsonNodePtr, LevelObjectData& objectData) {
    const json& objJson = *(static_cast<const json*>(jsonNodePtr));

    // 型と名前の取得
    if (objJson.contains("type")) {
        objectData.type = objJson["type"].get<std::string>();
    }
    if (objJson.contains("name")) {
        objectData.name = objJson["name"].get<std::string>();
    }
    if (objJson.contains("file_name")) {
        objectData.fileName = objJson["file_name"].get<std::string>();
    }
    if (objJson.contains("spawn_progress")) {
        objectData.spawnProgress = objJson["spawn_progress"].get<float>();
    }
    if (objJson.contains("texture_path")) {
        objectData.texturePath = objJson["texture_path"].get<std::string>();
    }
    if (objJson.contains("is_destructible")) {
        objectData.isDestructible = objJson["is_destructible"].get<bool>();
    }

    // トランスフォーム
    if (objJson.contains("transform")) {
        const auto& transform = objJson["transform"];
        if (transform.contains("translation")) {
            auto t = transform["translation"];
            // Blender(Z-up)からDirectX(Y-up)への変換: YとZを入れ替える
            objectData.translation = Vector3(t[0], t[2], t[1]);
        }
        if (transform.contains("rotation")) {
            auto r = transform["rotation"];
            // 回転もYとZを入れ替える (ロール・ピッチ・ヨーの対応)
            objectData.rotation = Vector3(r[0], r[2], r[1]);
        }
        if (transform.contains("scale")) {
            auto s = transform["scale"];
            objectData.scale = Vector3(s[0], s[2], s[1]);
        }
    } else {
        objectData.scale = Vector3(1.0f, 1.0f, 1.0f);
    }

    // コライダー
    if (objJson.contains("collider")) {
        const auto& collider = objJson["collider"];
        objectData.hasCollider = true;
        if (collider.contains("type")) {
            objectData.collider.type = collider["type"].get<std::string>();
        }
        if (collider.contains("center")) {
            auto c = collider["center"];
            objectData.collider.center = Vector3(c[0], c[2], c[1]);
        }
        if (collider.contains("size")) {
            auto s = collider["size"];
            objectData.collider.size = Vector3(s[0], s[2], s[1]);
        }
        if (collider.contains("radius")) {
            objectData.collider.radius = collider["radius"].get<float>();
        }
    }

    // カーブ制御点
    if (objJson.contains("curve_points") && objJson["curve_points"].is_array()) {
        for (const auto& ptJson : objJson["curve_points"]) {
            LevelCurvePoint pt;
            if (ptJson.contains("position")) {
                auto p = ptJson["position"];
                pt.position = Vector3(p["x"], p["z"], p["y"]);
            }
            if (ptJson.contains("handle_left")) {
                auto hl = ptJson["handle_left"];
                pt.handle_left = Vector3(hl["x"], hl["z"], hl["y"]);
            } else {
                pt.handle_left = pt.position;
            }
            if (ptJson.contains("handle_right")) {
                auto hr = ptJson["handle_right"];
                pt.handle_right = Vector3(hr["x"], hr["z"], hr["y"]);
            } else {
                pt.handle_right = pt.position;
            }
            if (ptJson.contains("tilt")) {
                pt.tilt = ptJson["tilt"].get<float>();
            } else {
                pt.tilt = 0.0f;
            }
            if (ptJson.contains("speed")) {
                pt.speed = ptJson["speed"].get<float>();
            }
            if (ptJson.contains("event")) {
                pt.event = ptJson["event"].get<std::string>();
            }
            objectData.curvePoints.push_back(pt);
        }
    }

    // 子オブジェクト（再帰）
    if (objJson.contains("children") && objJson["children"].is_array()) {
        for (const auto& childJson : objJson["children"]) {
            LevelObjectData childData;
            ParseObject(&childJson, childData);
            objectData.children.push_back(std::move(childData));
        }
    }
}

std::unique_ptr<LevelData> LevelLoader::Load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("LevelLoader: Failed to open file: " + filePath + "\n");
        return nullptr;
    }

    json rootJson;
    try {
        file >> rootJson;
    } catch (json::parse_error& e) {
        Logger::Log("LevelLoader: JSON parse error: " + std::string(e.what()) + "\n");
        return nullptr;
    }

    auto levelData = std::make_unique<LevelData>();

    if (rootJson.contains("objects") && rootJson["objects"].is_array()) {
        for (const auto& objJson : rootJson["objects"]) {
            LevelObjectData objData;
            ParseObject(&objJson, objData);
            levelData->objects.push_back(std::move(objData));
        }
    }

    Logger::Log("LevelLoader: Successfully loaded " + filePath + "\n");
    return levelData;
}
