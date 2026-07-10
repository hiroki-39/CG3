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
    
    // 敵プロパティ
    bool isEnemy = false;
    std::string enemyType = "RUSHER";
    std::string enemyTargetName = "";
    Vector3 enemyTargetPos;
    float enemyMaxY = 10.0f;
    float enemyMinY = -10.0f;
    int enemyFormationId = -1;

    // スポナー設定
    int spawnCount = 1;
    int spawnInterval = 30;
    std::string formationType = "NONE";
    float formationSpacing = 10.0f;

    // 敵拡張プロパティ
    std::string enemyBehavior = "STRAIGHT";
    float enemySpeed = 1.0f;
    int enemyShootInterval = 180;
    float enemySpawnDist = 800.0f;
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
