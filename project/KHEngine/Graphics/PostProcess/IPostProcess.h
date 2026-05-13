#pragma once
#include <string>

// 今後増えるエフェクト用パラメータをすべて詰め込む定数バッファ構造体
struct PostProcessData {
  int enableGrayscale; // 0 or 1
  int enableSepia;     // 0 or 1
  float sepiaStrength;
  float padding; // 16バイトアライメント
};

class IPostProcess {
public:
  virtual ~IPostProcess() = default;

  virtual void Initialize() = 0;
  virtual void DrawImGui() = 0;

  bool isActive_ = false;
  std::string name_ = "None";
};
