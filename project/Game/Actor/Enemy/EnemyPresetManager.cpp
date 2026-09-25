#include "EnemyPresetManager.h"
#include "externals/nlohmann/json.hpp"
#include <fstream>
#include <filesystem>
#include <cmath>

using json = nlohmann::json;

EnemyPresetManager* EnemyPresetManager::GetInstance() {
    static EnemyPresetManager instance;
    return &instance;
}

void EnemyPresetManager::Initialize() {
    CreateDefaultPresets();
    // 既存のJSON設定があればロードして上書き、なければデフォルトを保存
    if (!LoadPresets()) {
        SavePresets();
    }
}

void EnemyPresetManager::CreateDefaultPresets() {
    presets_.clear();

    // 1. 突撃型
    {
        EnemyPresetData data;
        data.typeName = "RUSHER";
        data.displayName = "突撃型エネミー (RUSHER)";
        data.modelName = "suzanne.obj";
        data.scale = { 1.2f, 1.2f, 1.2f };
        data.color = { 1.0f, 0.4f, 0.4f, 1.0f };
        data.hp = 1; // 1撃で爽快に撃破
        data.moveSpeed = 1.0f;
        data.moveAmplitude = 10.0f;
        data.shootInterval = 0.0f; // 撃たない
        data.behavior = "STRAIGHT";
        data.colliderType = "SPHERE";
        data.colliderCenter = { 0.0f, 0.0f, 0.0f };
        data.colliderRadius = 2.2f;
        data.formationType = "V_SHAPE";
        data.formationCount = 3;
        data.formationSpacing = 12.0f;
        presets_[data.typeName] = data;
    }

    // 2. 左右往復型（水平パトロール）
    {
        EnemyPresetData data;
        data.typeName = "PATROL_H";
        data.displayName = "左右往復エネミー (PATROL_H)";
        data.modelName = "suzanne.obj";
        data.scale = { 1.4f, 1.4f, 1.4f };
        data.color = { 0.2f, 0.8f, 1.0f, 1.0f }; // 水色
        data.hp = 2;
        data.moveSpeed = 1.0f;
        data.moveAmplitude = 15.0f; // 左右15mの幅で往復
        data.shootInterval = 120.0f;
        data.behavior = "PATROL_H";
        data.isHomingBullet = false;
        data.bulletSpeed = 1.8f;
        data.colliderType = "SPHERE";
        data.colliderCenter = { 0.0f, 0.0f, 0.0f };
        data.colliderRadius = 2.4f;
        data.formationType = "NONE";
        data.formationCount = 1;
        data.formationSpacing = 10.0f;
        presets_[data.typeName] = data;
    }

    // 3. 上下往復型（垂直パトロール）
    {
        EnemyPresetData data;
        data.typeName = "PATROL_V";
        data.displayName = "上下往復エネミー (PATROL_V)";
        data.modelName = "suzanne.obj";
        data.scale = { 1.4f, 1.4f, 1.4f };
        data.color = { 0.4f, 1.0f, 0.4f, 1.0f }; // 緑色
        data.hp = 2;
        data.moveSpeed = 1.0f;
        data.moveAmplitude = 8.0f; // 上下8mの幅で往復
        data.shootInterval = 120.0f;
        data.behavior = "PATROL_V";
        data.isHomingBullet = false;
        data.bulletSpeed = 1.8f;
        data.colliderType = "SPHERE";
        data.colliderCenter = { 0.0f, 0.0f, 0.0f };
        data.colliderRadius = 2.4f;
        data.formationType = "NONE";
        data.formationCount = 1;
        data.formationSpacing = 10.0f;
        presets_[data.typeName] = data;
    }

    // 4. 射撃型（サイン波前進）
    {
        EnemyPresetData data;
        data.typeName = "SHOOTER";
        data.displayName = "射撃型エネミー (SHOOTER)";
        data.modelName = "suzanne.obj";
        data.scale = { 1.4f, 1.4f, 1.4f };
        data.color = { 1.0f, 0.8f, 0.2f, 1.0f };
        data.hp = 2;
        data.moveSpeed = 0.8f;
        data.moveAmplitude = 12.0f;
        data.shootInterval = 120.0f;
        data.behavior = "SIN_WAVE";
        data.isHomingBullet = false;
        data.bulletSpeed = 1.8f;
        data.colliderType = "SPHERE";
        data.colliderCenter = { 0.0f, 0.0f, 0.0f };
        data.colliderRadius = 2.4f;
        data.formationType = "LINE_H";
        data.formationCount = 3;
        data.formationSpacing = 14.0f;
        presets_[data.typeName] = data;
    }

    // 5. 誘導弾・滞空浮遊型
    {
        EnemyPresetData data;
        data.typeName = "HOMING";
        data.displayName = "誘導弾エネミー (HOMING)";
        data.modelName = "suzanne.obj";
        data.scale = { 1.5f, 1.5f, 1.5f };
        data.color = { 0.8f, 0.2f, 1.0f, 1.0f };
        data.hp = 3;
        data.moveSpeed = 0.6f;
        data.moveAmplitude = 8.0f;
        data.shootInterval = 160.0f;
        data.behavior = "HOVER_SHOOT";
        data.isHomingBullet = true;
        data.bulletSpeed = 1.4f;
        data.colliderType = "SPHERE";
        data.colliderCenter = { 0.0f, 0.0f, 0.0f };
        data.colliderRadius = 2.5f;
        data.formationType = "NONE";
        data.formationCount = 1;
        data.formationSpacing = 10.0f;
        presets_[data.typeName] = data;
    }

    // 6. 固定砲台
    {
        EnemyPresetData data;
        data.typeName = "TURRET";
        data.displayName = "地上砲台 (TURRET)";
        data.modelName = "cube.obj";
        data.scale = { 4.0f, 4.0f, 4.0f };
        data.modelPosOffset = { 0.0f, 1.0f, 0.0f }; // Blenderの2mダミーCube接地位置に合わせ、底面を地面の上に持ち上げる
        data.color = { 0.6f, 0.6f, 0.7f, 1.0f };
        data.hp = 2;
        data.moveSpeed = 0.0f;
        data.moveAmplitude = 0.0f;
        data.shootInterval = 90.0f;
        data.behavior = "TURRET";
        data.isHomingBullet = false;
        data.bulletSpeed = 2.2f;
        data.colliderType = "BOX";
        data.colliderCenter = { 0.0f, 1.0f, 0.0f };
        data.colliderSize = { 3.0f, 3.0f, 3.0f };
        data.formationType = "NONE";
        data.formationCount = 1;
        data.formationSpacing = 10.0f;
        presets_[data.typeName] = data;
    }

    // 5. 装甲列車・機関車（先頭車両）
    {
        EnemyPresetData data;
        data.typeName = "ARMORED_TRAIN_LOCO";
        data.displayName = "装甲列車・機関車 (先頭車両)";
        data.modelName = "cube.obj";
        data.scale = { 4.0f, 3.2f, 12.0f };
        data.color = { 0.35f, 0.35f, 0.4f, 1.0f };
        data.hp = 50;
        data.moveSpeed = 0.5f;
        data.shootInterval = 60.0f;
        data.behavior = "TRAIN_LOCO";
        data.isHomingBullet = false;
        data.bulletSpeed = 2.5f;
        data.colliderType = "BOX";
        data.colliderCenter = { 0.0f, 1.5f, 0.0f };
        data.colliderSize = { 5.0f, 4.0f, 13.0f };
        data.formationType = "NONE";
        data.formationCount = 1;
        data.formationSpacing = 15.0f;
        presets_[data.typeName] = data;
    }

    // 6. 装甲列車・主砲塔車
    {
        EnemyPresetData data;
        data.typeName = "ARMORED_TRAIN_TURRET";
        data.displayName = "装甲列車・旋回砲塔車";
        data.modelName = "cube.obj";
        data.scale = { 3.6f, 2.6f, 10.0f };
        data.color = { 0.4f, 0.42f, 0.45f, 1.0f };
        data.hp = 30;
        data.moveSpeed = 0.5f;
        data.shootInterval = 80.0f;
        data.behavior = "TRAIN_TURRET";
        data.isHomingBullet = false;
        data.bulletSpeed = 2.0f;
        data.colliderType = "BOX";
        data.colliderCenter = { 0.0f, 1.3f, 0.0f };
        data.colliderSize = { 4.5f, 3.5f, 11.0f };
        data.formationType = "NONE";
        data.formationCount = 1;
        data.formationSpacing = 15.0f;
        presets_[data.typeName] = data;
    }

    // 7. 装甲列車・ミサイルコンテナ車
    {
        EnemyPresetData data;
        data.typeName = "ARMORED_TRAIN_MISSILE";
        data.displayName = "装甲列車・ミサイルコンテナ車";
        data.modelName = "cube.obj";
        data.scale = { 3.6f, 3.0f, 10.0f };
        data.color = { 0.45f, 0.38f, 0.38f, 1.0f };
        data.hp = 30;
        data.moveSpeed = 0.5f;
        data.shootInterval = 140.0f;
        data.behavior = "TRAIN_MISSILE";
        data.isHomingBullet = true;
        data.bulletSpeed = 1.3f;
        data.colliderType = "BOX";
        data.colliderCenter = { 0.0f, 1.5f, 0.0f };
        data.colliderSize = { 4.5f, 3.8f, 11.0f };
        data.formationType = "NONE";
        data.formationCount = 1;
        data.formationSpacing = 15.0f;
        presets_[data.typeName] = data;
    }

    RebuildNameList();
}

void EnemyPresetManager::RebuildNameList() {
    presetNames_.clear();
    for (const auto& [name, _] : presets_) {
        presetNames_.push_back(name);
    }
}

bool EnemyPresetManager::LoadPresets(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return false;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    try {
        json root;
        file >> root;

        if (!root.contains("presets") || !root["presets"].is_object()) {
            return false;
        }

        presets_.clear();
        for (auto& [key, item] : root["presets"].items()) {
            EnemyPresetData data;
            data.typeName = key;
            if (item.contains("displayName")) data.displayName = item["displayName"].get<std::string>();
            if (item.contains("modelName")) data.modelName = item["modelName"].get<std::string>();

            if (item.contains("scale") && item["scale"].is_array() && item["scale"].size() >= 3) {
                data.scale = { item["scale"][0].get<float>(), item["scale"][1].get<float>(), item["scale"][2].get<float>() };
            }
            if (item.contains("modelPosOffset") && item["modelPosOffset"].is_array() && item["modelPosOffset"].size() >= 3) {
                data.modelPosOffset = { item["modelPosOffset"][0].get<float>(), item["modelPosOffset"][1].get<float>(), item["modelPosOffset"][2].get<float>() };
            }
            if (item.contains("color") && item["color"].is_array() && item["color"].size() >= 4) {
                data.color = { item["color"][0].get<float>(), item["color"][1].get<float>(), item["color"][2].get<float>(), item["color"][3].get<float>() };
            }

            if (item.contains("hp")) data.hp = item["hp"].get<int>();
            if (item.contains("moveSpeed")) data.moveSpeed = item["moveSpeed"].get<float>();
            if (item.contains("moveAmplitude")) data.moveAmplitude = item["moveAmplitude"].get<float>();
            if (item.contains("shootInterval")) data.shootInterval = item["shootInterval"].get<float>();
            if (item.contains("behavior")) data.behavior = item["behavior"].get<std::string>();
            if (item.contains("isHomingBullet")) data.isHomingBullet = item["isHomingBullet"].get<bool>();
            if (item.contains("bulletSpeed")) data.bulletSpeed = item["bulletSpeed"].get<float>();

            // コライダー
            if (item.contains("colliderType")) data.colliderType = item["colliderType"].get<std::string>();
            if (item.contains("colliderCenter") && item["colliderCenter"].is_array() && item["colliderCenter"].size() >= 3) {
                data.colliderCenter = { item["colliderCenter"][0].get<float>(), item["colliderCenter"][1].get<float>(), item["colliderCenter"][2].get<float>() };
            }
            if (item.contains("colliderRadius")) data.colliderRadius = item["colliderRadius"].get<float>();
            if (item.contains("colliderSize") && item["colliderSize"].is_array() && item["colliderSize"].size() >= 3) {
                data.colliderSize = { item["colliderSize"][0].get<float>(), item["colliderSize"][1].get<float>(), item["colliderSize"][2].get<float>() };
            }

            // 陣形
            if (item.contains("formationType")) data.formationType = item["formationType"].get<std::string>();
            if (item.contains("formationCount")) data.formationCount = item["formationCount"].get<int>();
            if (item.contains("formationSpacing")) data.formationSpacing = item["formationSpacing"].get<float>();

            presets_[key] = data;
        }

        RebuildNameList();
        return true;
    } catch (...) {
        return false;
    }
}

bool EnemyPresetManager::SavePresets(const std::string& filePath) {
    try {
        std::filesystem::path path(filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        json root;
        json presetsObj = json::object();

        for (const auto& [key, data] : presets_) {
            json item;
            item["displayName"] = data.displayName;
            item["modelName"] = data.modelName;
            item["scale"] = { data.scale.x, data.scale.y, data.scale.z };
            item["modelPosOffset"] = { data.modelPosOffset.x, data.modelPosOffset.y, data.modelPosOffset.z };
            item["color"] = { data.color.x, data.color.y, data.color.z, data.color.w };
            item["hp"] = data.hp;
            item["moveSpeed"] = data.moveSpeed;
            item["moveAmplitude"] = data.moveAmplitude;
            item["shootInterval"] = data.shootInterval;
            item["behavior"] = data.behavior;
            item["isHomingBullet"] = data.isHomingBullet;
            item["bulletSpeed"] = data.bulletSpeed;

            // コライダー
            item["colliderType"] = data.colliderType;
            item["colliderCenter"] = { data.colliderCenter.x, data.colliderCenter.y, data.colliderCenter.z };
            item["colliderRadius"] = data.colliderRadius;
            item["colliderSize"] = { data.colliderSize.x, data.colliderSize.y, data.colliderSize.z };

            // 陣形
            item["formationType"] = data.formationType;
            item["formationCount"] = data.formationCount;
            item["formationSpacing"] = data.formationSpacing;

            presetsObj[key] = item;
        }

        root["presets"] = presetsObj;

        std::ofstream file(filePath);
        if (!file.is_open()) return false;

        file << root.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

std::string EnemyPresetManager::ResolveTypeAlias(const std::string& typeName) const {
    if (typeName.empty()) return "RUSHER";

    // 突撃型
    if (typeName == "RUSHER" || typeName == "突撃型" || typeName == "突進" || typeName == "突進エネミー" || typeName == "突撃型エネミー") {
        return "RUSHER";
    }
    // 左右往復型
    if (typeName == "PATROL_H" || typeName == "左右往復" || typeName == "左右移動" || typeName == "水平往復" || typeName == "左右パトロール" || typeName == "左右往復エネミー") {
        return "PATROL_H";
    }
    // 上下往復型
    if (typeName == "PATROL_V" || typeName == "上下往復" || typeName == "上下移動" || typeName == "垂直往復" || typeName == "上下パトロール" || typeName == "上下往復エネミー") {
        return "PATROL_V";
    }
    // 射撃型
    if (typeName == "SHOOTER" || typeName == "射撃型" || typeName == "射撃" || typeName == "射撃エネミー" || typeName == "射撃型エネミー") {
        return "SHOOTER";
    }
    // 誘導弾型
    if (typeName == "HOMING" || typeName == "誘導弾型" || typeName == "誘導弾" || typeName == "ホーミング" || typeName == "誘導弾エネミー") {
        return "HOMING";
    }
    // 固定砲台
    if (typeName == "TURRET" || typeName == "固定砲台" || typeName == "地上砲台" || typeName == "砲台") {
        return "TURRET";
    }
    // 装甲列車・機関車
    if (typeName == "ARMORED_TRAIN_LOCO" || typeName == "装甲列車" || typeName == "装甲列車_機関車" || typeName == "機関車" || typeName == "装甲列車ボス" || typeName == "BOSS_TRAIN") {
        return "ARMORED_TRAIN_LOCO";
    }
    // 装甲列車・砲塔車
    if (typeName == "ARMORED_TRAIN_TURRET" || typeName == "装甲列車_砲塔車" || typeName == "砲塔車") {
        return "ARMORED_TRAIN_TURRET";
    }
    // 装甲列車・ミサイル車
    if (typeName == "ARMORED_TRAIN_MISSILE" || typeName == "装甲列車_ミサイル車" || typeName == "ミサイル車") {
        return "ARMORED_TRAIN_MISSILE";
    }
    return typeName;
}

const EnemyPresetData* EnemyPresetManager::GetPreset(const std::string& typeName) const {
    if (presets_.empty()) {
        const_cast<EnemyPresetManager*>(this)->Initialize();
    }
    auto it = presets_.find(typeName);
    if (it != presets_.end()) {
        return &it->second;
    }
    std::string alias = ResolveTypeAlias(typeName);
    it = presets_.find(alias);
    if (it != presets_.end()) {
        return &it->second;
    }
    for (const auto& [key, data] : presets_) {
        if (data.displayName == typeName || data.displayName.find(typeName) != std::string::npos) {
            return &data;
        }
    }
    auto fallback = presets_.find("RUSHER");
    if (fallback != presets_.end()) {
        return &fallback->second;
    }
    if (!presets_.empty()) {
        return &presets_.begin()->second;
    }
    return nullptr;
}

EnemyPresetData* EnemyPresetManager::GetPreset(const std::string& typeName) {
    if (presets_.empty()) {
        Initialize();
    }
    auto it = presets_.find(typeName);
    if (it != presets_.end()) {
        return &it->second;
    }
    std::string alias = ResolveTypeAlias(typeName);
    it = presets_.find(alias);
    if (it != presets_.end()) {
        return &it->second;
    }
    for (auto& [key, data] : presets_) {
        if (data.displayName == typeName || data.displayName.find(typeName) != std::string::npos) {
            return &data;
        }
    }
    auto fallback = presets_.find("RUSHER");
    if (fallback != presets_.end()) {
        return &fallback->second;
    }
    if (!presets_.empty()) {
        return &presets_.begin()->second;
    }
    return nullptr;
}

void EnemyPresetManager::SetPreset(const std::string& typeName, const EnemyPresetData& data) {
    presets_[typeName] = data;
    RebuildNameList();
}

void EnemyPresetManager::DeletePreset(const std::string& typeName) {
    presets_.erase(typeName);
    RebuildNameList();
}

std::vector<Vector3> EnemyPresetManager::CalculateFormationOffsets(const std::string& formationType, int count, float spacing) {
    std::vector<Vector3> offsets;
    if (count <= 1 || formationType == "NONE") {
        offsets.push_back({ 0.0f, 0.0f, 0.0f });
        return offsets;
    }

    offsets.resize(count);

    if (formationType == "LINE_H") {
        // 水平横一列 (X方向に対称展開)
        float totalWidth = (count - 1) * spacing;
        float startX = -totalWidth * 0.5f;
        for (int i = 0; i < count; ++i) {
            offsets[i] = { startX + i * spacing, 0.0f, 0.0f };
        }
    } else if (formationType == "LINE_V") {
        // 垂直縦一列 (Y方向に対称展開)
        float totalHeight = (count - 1) * spacing;
        float startY = -totalHeight * 0.5f;
        for (int i = 0; i < count; ++i) {
            offsets[i] = { 0.0f, startY + i * spacing, 0.0f };
        }
    } else if (formationType == "V_SHAPE") {
        // V字編隊 (先頭が原点、左右後方に翼を展開)
        offsets[0] = { 0.0f, 0.0f, 0.0f };
        for (int i = 1; i < count; ++i) {
            int rank = (i + 1) / 2; // 翼の段数 (1, 1, 2, 2, ...)
            float side = (i % 2 == 1) ? -1.0f : 1.0f; // 左右
            offsets[i] = {
                side * rank * spacing * 0.75f,
                0.0f,
                -rank * spacing * 0.65f
            };
        }
    } else if (formationType == "BOX") {
        // 格子配置
        int cols = static_cast<int>(std::ceil(std::sqrt(count)));
        int rows = static_cast<int>(std::ceil(static_cast<float>(count) / cols));
        float startX = -(cols - 1) * spacing * 0.5f;
        float startZ = -(rows - 1) * spacing * 0.5f;
        for (int i = 0; i < count; ++i) {
            int r = i / cols;
            int c = i % cols;
            offsets[i] = { startX + c * spacing, 0.0f, startZ + r * spacing };
        }
    } else if (formationType == "CIRCLE") {
        // 円形配置
        float radius = spacing * count / (2.0f * 3.14159265f);
        if (radius < spacing) radius = spacing;
        for (int i = 0; i < count; ++i) {
            float angle = (2.0f * 3.14159265f * i) / count;
            offsets[i] = { std::cos(angle) * radius, 0.0f, std::sin(angle) * radius };
        }
    } else {
        offsets[0] = { 0.0f, 0.0f, 0.0f };
    }

    return offsets;
}
