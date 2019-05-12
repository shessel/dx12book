#pragma once

#include <array>
#include <memory>
#include <vector>

#include "d3d12.h"
#include "DirectXMath.h"

#include "ArcBallCamera.h"
#include "AppBase.h"
#include "D3D12Util.h"
#include "Mesh.h"
#include "Renderable.h"

class LandAndWavesDemoLit : public AppBase
{
public:
    using AppBase::AppBase;
    ~LandAndWavesDemoLit();

    virtual void onMouseDown(int16_t xPos, int16_t yPos, uint8_t buttons) override;
    virtual void onMouseUp(int16_t xPos, int16_t yPos, uint8_t buttons) override;
    virtual void onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons) override;

    virtual void initialize() override;
    virtual void update(float dt) override;
    virtual void render() override;

private:
    struct PassConstants
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        float time;
        float dTime;
    };

    struct ObjectConstants
    {
        DirectX::XMFLOAT4X4 world;
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
        std::unique_ptr<D3D12Util::MappedGPUBuffer> m_pDynamicVertices;
    };

    static constexpr size_t FRAME_RESOURCES_COUNT = 3u;
    static constexpr uint16_t VERTICES_PER_SIDE = 100u;
    size_t m_curFrameResourcesIndex = 0u;
    FrameResources m_frameResources[FRAME_RESOURCES_COUNT];
    std::vector<Renderable> m_renderables;

    Mesh m_landMesh;
    Vertex m_wavesVertices[VERTICES_PER_SIDE][VERTICES_PER_SIDE];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pWavesIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pWavesIndexBufferUpload;
    size_t m_wavesVertexCount;
    size_t m_wavesIndexCount;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

    ArcBallCamera m_camera = ArcBallCamera(3.0f, 0.0f, 0.5f);
    UINT64 m_curFrameFenceValue = 0u;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_pFrameFence;
    
    int16_t m_lastMouseX = 0;
    int16_t m_lastMouseY = 0;
    int16_t m_curMouseX = 0;
    int16_t m_curMouseY = 0;

    static constexpr float m_gridWidth = 25.0f;
};