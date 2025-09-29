#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include "Transform.h"
#include "Material.h"
#include "Camera.h"
#include "Light.h"

class Model
{
public:
    void LoadFromObj(const std::string& filepath);
    void Update();
    void Draw(ID3D12GraphicsCommandList* cmdList, const Camera& camera, const Light& light);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    Transform transform_;
    Material material_;
};

