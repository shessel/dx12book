#pragma once

#include <array>
#include <memory>
#include <vector>

#include "d3d12.h"
#include "DirectXMath.h"

#include "ArcBallCamera.h"
#include "AppBase.h"
#include "D3D12Util.h"
#include "Material.h"
#include "Mesh.h"
#include "Renderable.h"
#include "DdsTexture.h"

class Mirror : public AppBase
{
public:
    using AppBase::AppBase;
    Mirror(HINSTANCE hInst);
    ~Mirror();

    virtual void onMouseDown(int16_t xPos, int16_t yPos, uint8_t buttons) override;
    virtual void onMouseUp(int16_t xPos, int16_t yPos, uint8_t buttons) override;
    virtual void onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons) override;

    virtual void initialize() override;
    virtual void update(float dt) override;
    virtual void render() override;

private:
    static constexpr size_t MAX_LIGHT_COUNT = 16;
    struct LightData
    {
        DirectX::XMFLOAT3 position;
        float falloffBegin;
        DirectX::XMFLOAT3 direction;
        float falloffEnd;
        DirectX::XMFLOAT3 color;
        float spotPower;
    };

    struct LightConstants
    {
        LightData lightData[MAX_LIGHT_COUNT];
    };

    struct PassConstants
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;

        float time;
        float dTime;
        uint32_t directionalLightCount;
        uint32_t pointLightCount;

        DirectX::XMFLOAT3 ambientLight;
        uint32_t spotLightCount;

        DirectX::XMFLOAT3 cameraPositionW;
        float alphaClipThreshold;

        DirectX::XMFLOAT3 fogColor;
        float fogBegin;

        float fogEnd;
        float padding[3];
    };

    struct ObjectConstants
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT2 texCoordTransformColumn0;
        DirectX::XMFLOAT2 texCoordTransformColumn1;
        DirectX::XMFLOAT2 texCoordOffset;
    };

    struct MaterialConstants
    {
        DirectX::XMFLOAT4 albedoColor;
        DirectX::XMFLOAT3 fresnelR0;
        float roughness;
        DirectX::XMFLOAT2 texCoordTransformColumn0;
        DirectX::XMFLOAT2 texCoordTransformColumn1;
        DirectX::XMFLOAT2 texCoordOffset;
    };

    struct Vertex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 uv;
        DirectX::XMFLOAT3 normal;
    };

    struct FrameResources
    {
        UINT64 m_fenceValue = 0u;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;

        std::unique_ptr<D3D12Util::MappedGPUBuffer> m_pCbPass;
        std::unique_ptr<D3D12Util::MappedGPUBuffer> m_pCbObjects;
        std::unique_ptr<D3D12Util::MappedGPUBuffer> m_pCbLights;
        std::unique_ptr<D3D12Util::MappedGPUBuffer> m_pCbMaterials;
    };

    static constexpr float m_clearColor[4] = { 0.4f, 0.45f, 0.4f, 1.0f };
    static constexpr bool m_useFog = false;
    static constexpr size_t FRAME_RESOURCES_COUNT = 3u;
    static constexpr uint16_t VERTICES_PER_SIDE = 2u;
    size_t m_curFrameResourcesIndex = 0u;
    FrameResources m_frameResources[FRAME_RESOURCES_COUNT];

    std::vector<Renderable> m_sceneRenderables;
    std::vector<Renderable> m_mirrorRenderables;
    std::vector<Renderable> m_mirroredSceneRenderables;

    std::vector<Material> m_materials;
    std::vector<Mesh> m_meshes;
    std::vector<DdsTexture> m_textures;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineStateOpaque;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineStateOpaqueMirrored;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineStateAlphaBlend;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineStateStencilWrite;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pSrvHeap;

    DirectX::XMFLOAT4X4 m_mirrorMatrix;

    ArcBallCamera m_camera = ArcBallCamera(15.0f, 0.5f, 0.5f);
    UINT64 m_curFrameFenceValue = 0u;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_pFrameFence;
    
    int16_t m_lastMouseX = 0;
    int16_t m_lastMouseY = 0;
    int16_t m_curMouseX = 0;
    int16_t m_curMouseY = 0;

    static constexpr float m_gridWidth = 25.0f;
};