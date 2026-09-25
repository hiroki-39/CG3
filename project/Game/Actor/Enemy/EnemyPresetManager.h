#pragma once
#include "KHEngine/Math/Vector3.h"
#include "KHEngine/Math/Vector4.h"
#include "KHEngine/Scene/LevelLoader.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

/**
 * @brief 敵プリセットデータ（ゲーム側でオーサリングする敵設定）
 */
struct EnemyPresetData {
    std::string typeName = "RUSHER";             // プリセット識別子（Blender側 enemyType と紐付け）
    std::string displayName = "突撃型エネミー";   // エディタ表示名
    std::string modelName = "suzanne.obj";       // 使用する3Dモデル（resources/3dModels/<name>/）
    Vector3 scale = { 1.0f, 1.0f, 1.0f };       // スケール
    Vector3 modelPosOffset = { 0.0f, 0.0f, 0.0f }; // モデル配置位置オフセット（接地・ピボット調整用）
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // カラー乗数
    int hp = 2;                                 // 耐久値
    float moveSpeed = 1.0f;                     // 移動速度
    float moveAmplitude = 10.0f;                // 移動幅・振幅（左右・上下往復の移動範囲）
    float shootInterval = 180.0f;               // 射撃間隔（フレーム数、0で射撃なし）
    std::string behavior = "STRAIGHT";          // 行動パターン (PATROL_H, PATROL_V, HOVER_SHOOT, SIN_WAVE, STRAIGHT, TURRET, TRAIN_LOCO, etc.)
    bool isHomingBullet = false;                // 発射する弾がホーミング弾かどうか
    float bulletSpeed = 2.0f;                   // 弾速

    // 当たり判定（コライダー）設定 - ゲーム側で視覚的に設定
    std::string colliderType = "SPHERE";        // "SPHERE" または "BOX"
    Vector3 colliderCenter = { 0.0f, 0.0f, 0.0f }; // 中心オフセット
    float colliderRadius = 2.0f;                // 球コライダー半径
    Vector3 colliderSize = { 2.0f, 2.0f, 2.0f }; // 直方体コライダーサイズ

    // 陣形（Formation）設定
    std::string formationType = "NONE";         // "NONE", "LINE_H", "LINE_V", "V_SHAPE", "BOX", "CIRCLE"
    int formationCount = 1;                     // 編隊の機体数
    float formationSpacing = 12.0f;             // 機体間隔
};

/**
 * @brief 敵プリセットマネージャー
 */
class EnemyPresetManager {
public:
    static EnemyPresetManager* GetInstance();

    void Initialize();
    bool LoadPresets(const std::string& filePath = "resources/json/enemy/enemy_presets.json");
    bool SavePresets(const std::string& filePath = "resources/json/enemy/enemy_presets.json");

    const EnemyPresetData* GetPreset(const std::string& typeName) const;
    EnemyPresetData* GetPreset(const std::string& typeName);
    void SetPreset(const std::string& typeName, const EnemyPresetData& data);
    void DeletePreset(const std::string& typeName);

    const std::vector<std::string>& GetPresetNames() const { return presetNames_; }
    // ヘルパー：日本語別名や旧名称を正規キーに解決
    std::string ResolveTypeAlias(const std::string& typeName) const;

    // ヘルパー：陣形の各機体のローカルオフセット座標を計算
    static std::vector<Vector3> CalculateFormationOffsets(const std::string& formationType, int count, float spacing);

private:
    EnemyPresetManager() = default;
    ~EnemyPresetManager() = default;
    EnemyPresetManager(const EnemyPresetManager&) = delete;
    EnemyPresetManager& operator=(const EnemyPresetManager&) = delete;

    void CreateDefaultPresets();
    void RebuildNameList();

private:
    std::unordered_map<std::string, EnemyPresetData> presets_;
    std::vector<std::string> presetNames_;
};
