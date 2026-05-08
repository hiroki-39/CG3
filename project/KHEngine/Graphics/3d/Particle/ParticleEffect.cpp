#include "ParticleEffect.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "externals/imgui/imgui.h"
#include <algorithm>

void ParticleEffect::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;
}

void ParticleEffect::AddNode(const std::string& name, int shapeType)
{
    auto node = std::make_unique<Node>();
    strncpy_s(node->name, name.c_str(), sizeof(node->name) - 1);
    node->shapeType = shapeType;
    
    if (shapeType == 0) node->emitter.SetParameter(ParticleEmitter::CreateExplosionPreset());
    else if (shapeType == 1) node->emitter.SetParameter(ParticleEmitter::CreateRingPreset());
    else if (shapeType == 2) node->emitter.SetParameter(ParticleEmitter::CreateCylinderPreset());

    node->emitter.SetTextureName("gradationLine.png");
    node->emitter.SetBlendMode(BlendMode::Additive);
    
    SetupRendererForNode(node.get());
    
    nodes_.push_back(std::move(node));
}

void ParticleEffect::SetupRendererForNode(Node* node)
{
    std::string assetName = "quad";
    if (node->shapeType == 1) assetName = "ring";
    else if (node->shapeType == 2) assetName = "Cylinder";

    ParticleManager::GetInstance()->SetupRendererFromAsset(node->renderer, assetName, dxCommon_, srvManager_, maxInstances_);
    node->instancingData = node->renderer.GetInstancingData();
}

void ParticleEffect::Play()
{
    for (auto& node : nodes_)
    {
        node->emitter.Emit(node->emitter.GetParameter().count);
    }
}

void ParticleEffect::Update(float dt, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Matrix4x4& billboardMatrix)
{
    for (auto& node : nodes_)
    {
        node->emitter.Update(dt);

        // UV Scroll & Color Anim
        node->uvScrollOffset += node->uvScrollSpeed * dt;
        if (node->uvScrollOffset > 1.0f) node->uvScrollOffset -= 1.0f;
        if (node->uvScrollOffset < -1.0f) node->uvScrollOffset += 1.0f;

        Vector4 currentColor = node->baseColor;
        if (node->enableColorAnim)
        {
            node->colorTimer += node->colorAnimSpeed * dt;
            float t = (std::sin(node->colorTimer) + 1.0f) * 0.5f;
            currentColor = {
                std::lerp(node->startColor.x, node->endColor.x, t),
                std::lerp(node->startColor.y, node->endColor.y, t),
                std::lerp(node->startColor.z, node->endColor.z, t),
                std::lerp(node->startColor.w, node->endColor.w, t)
            };
        }

        Matrix4x4 uvTransform = Matrix4x4::Translation({ node->uvScrollOffset, 0.0f, 0.0f });
        ParticleManager::GetInstance()->UpdateMaterial(node->renderer, currentColor, uvTransform);

        node->numInstance = node->emitter.FillInstancingBuffer(node->instancingData, maxInstances_, viewMatrix, projectionMatrix, billboardMatrix);
    }
}

void ParticleEffect::Draw()
{
    for (auto& node : nodes_)
    {
        if (node->numInstance == 0) continue;
        uint32_t texIndex = TextureManager::GetInstance()->GetSrvIndex(node->emitter.GetTextureName());
        int blendIndex = static_cast<int>(node->emitter.GetBlendMode());
        node->renderer.Draw(node->numInstance, texIndex, blendIndex);
    }
}

void ParticleEffect::DrawImGui()
{
    ImGui::Begin("Effect Editor");

    if (ImGui::Button("Play Effect"))
    {
        Play();
    }
    
    ImGui::Separator();
    ImGui::Text("Add New Node");
    static int newShapeType = 0;
    const char* shapes[] = { "Quad (Plane)", "Ring", "Cylinder" };
    ImGui::Combo("Shape", &newShapeType, shapes, 3);
    if (ImGui::Button("Add Node"))
    {
        AddNode("New Node", newShapeType);
    }

    ImGui::Separator();
    
    for (size_t i = 0; i < nodes_.size(); ++i)
    {
        auto& node = nodes_[i];
        ImGui::PushID(static_cast<int>(i));
        
        if (ImGui::CollapsingHeader(node->name, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputText("Name", node->name, sizeof(node->name));
            
            // Shape Type
            if (ImGui::Combo("Shape Type", &node->shapeType, shapes, 3))
            {
                SetupRendererForNode(node.get());
            }

            // Emitter params
            auto param = node->emitter.GetParameter();
            int count = static_cast<int>(param.count);
            if (ImGui::DragInt("Count", &count, 1.0f, 1, 100))
            {
                param.count = count;
                node->emitter.SetParameter(param);
            }
            float life[2] = { param.minLifeTime, param.maxLifeTime };
            if (ImGui::DragFloat2("LifeTime", life, 0.1f, 0.1f, 10.0f))
            {
                param.minLifeTime = life[0];
                param.maxLifeTime = life[1];
                node->emitter.SetParameter(param);
            }

            // Animation
            ImGui::DragFloat("UV Scroll Speed", &node->uvScrollSpeed, 0.05f, -5.0f, 5.0f);
            
            ImGui::Checkbox("Color Animation", &node->enableColorAnim);
            if (node->enableColorAnim)
            {
                ImGui::DragFloat("Color Anim Speed", &node->colorAnimSpeed, 0.1f, 0.0f, 10.0f);
                float sc[4] = { node->startColor.x, node->startColor.y, node->startColor.z, node->startColor.w };
                if (ImGui::ColorEdit4("Start Color", sc)) node->startColor = { sc[0], sc[1], sc[2], sc[3] };
                float ec[4] = { node->endColor.x, node->endColor.y, node->endColor.z, node->endColor.w };
                if (ImGui::ColorEdit4("End Color", ec)) node->endColor = { ec[0], ec[1], ec[2], ec[3] };
            }
            else
            {
                float bc[4] = { node->startColor.x, node->startColor.y, node->startColor.z, node->startColor.w };
                if (ImGui::ColorEdit4("Color", bc)) node->startColor = { bc[0], bc[1], bc[2], bc[3] };
                node->baseColor = node->startColor;
            }

            // Blend Mode
            int mode = static_cast<int>(node->emitter.GetBlendMode());
            const char* blendNames[] = { "None", "Alpha", "Additive", "Multiply", "PreMultiplied" };
            if (ImGui::Combo("Blend Mode", &mode, blendNames, 5))
            {
                node->emitter.SetBlendMode(static_cast<BlendMode>(mode));
            }

            // Delete
            if (ImGui::Button("Delete Node"))
            {
                nodes_.erase(nodes_.begin() + i);
                ImGui::PopID();
                break; // Break loop to avoid invalid iterator
            }
        }
        ImGui::PopID();
    }

    ImGui::End();
}
