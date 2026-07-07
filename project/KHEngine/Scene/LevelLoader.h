#pragma once
#include <string>
#include <vector>
#include <memory>
#include "KHEngine/Math/Vector3.h"

struct LevelCollider {
    std::string type;
    Vector3 center;
    Vector3 size;
    float radius = 1.0f;
};

struct LevelCurvePoint {
    Vector3 position;
    Vector3 handle_left;
    Vector3 handle_right;
    float tilt;
    float speed = 20.0f;
    std::string event = "none";
};

struct LevelObjectData {
    std::string type;
    std::string name;
    Vector3 translation;
    Vector3 rotation;
    Vector3 scale;
    std::string fileName;
    float spawnProgress = 0.0f;
    std::string texturePath;
    bool isDestructible = true;
    
    // コライダー情報
    bool hasCollider = false;
    LevelCollider collider;
    
    // 子オブジェクト
    std::vector<LevelObjectData> children;
    
    // カーブ（レール）の制御点
    std::vector<LevelCurvePoint> curvePoints;
};

struct LevelData {
    std::vector<LevelObjectData> objects;
};

class LevelLoader {
public:
    /// <summary>
    /// 指定されたJSONファイルからレベルデータを読み込む
    /// </summary>
    /// <param name="filePath">JSONファイルのパス (例: "resources/json/maps/template/template.json")</param>
    /// <returns>読み込んだレベルデータ（失敗時はnullptr）</returns>
    static std::unique_ptr<LevelData> Load(const std::string& filePath);

private:
    /// <summary>
    /// 再帰的にJSONオブジェクトをパースする（実装はcppファイル内で行う）
    /// </summary>
    static void ParseObject(const void* jsonNode, LevelObjectData& objectData);
};
