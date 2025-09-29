#pragma once
#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
//#include "Model.h"
#include "Material.h"
//#include "Camera.h"
//#include "Light.h"
//#include "ShaderCompiler.h"
//#include "SoundManager.h"

class Engine
{
public:
    void Initialize(HWND hwnd);
    void Update();
    void Draw();
    void Finalize();
private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    // ���ɂ��X���b�v�`�F�[���⃊�\�[�X�֘A

    ShaderCompiler shaderCompiler_;
    SoundManager soundManager_;
    Camera camera_;
    Light light_;
    Model model_;
};

