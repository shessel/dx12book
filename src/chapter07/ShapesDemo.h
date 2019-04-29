#pragma once

#include <memory>
#include <vector>

#include "ArcBallCamera.h"
#include "AppBase.h"
#include "D3D12Util.h"
#include "Mesh.h"
#include "Renderable.h"

class ShapesDemo : public AppBase
{
public:
    using AppBase::AppBase;
    ~ShapesDemo();

    virtual void initialize() override final;
    virtual void update(float dt) override final;
    virtual void render() override final;
    virtual void onMouseDown(int16_t xPos, int16_t yPos, uint8_t buttons) override final;
    virtual void onMouseUp(int16_t xPos, int16_t yPos, uint8_t buttons) override final;
    virtual void onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons) override final;

private:
    struct PassConstants {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        float time;
        float deltaTime;
    };

    struct ObjectConstants {
        DirectX::XMFLOAT4X4 model;
    };

    struct FrameResources
    {
        FrameResources(ID3D12Device* const pDevice, size_t passCount, size_t objectCount)
        {
            m_pCbPass = std::make_unique<D3D12Util::ConstantBuffer>(pDevice, passCount, sizeof(PassConstants));
            m_pCbObjects = std::make_unique<D3D12Util::ConstantBuffer>(pDevice, objectCount, sizeof(ObjectConstants));
            pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));
        }
        FrameResources(const FrameResources&) = delete;
        FrameResources& operator=(const FrameResources&) = delete;
        FrameResources(FrameResources&&) = default;
        FrameResources& operator=(FrameResources&&) = default;
        ~FrameResources() = default;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
        std::unique_ptr<D3D12Util::ConstantBuffer> m_pCbPass;
        std::unique_ptr<D3D12Util::ConstantBuffer> m_pCbObjects;
        UINT64 m_fenceValue = 0u;
    };

    ArcBallCamera m_camera = ArcBallCamera(3.0f);
    std::unique_ptr<Mesh> m_pMesh;

    static constexpr size_t m_frameResourcesCount = 3u;
    std::vector<FrameResources> m_frameResources;
    size_t m_curFrameResourcesIndex = 0u;
    UINT64 m_curFrameFenceValue = 0u;
    std::vector<Renderable> m_renderables;

    int16_t m_lastMouseX = 0;
    int16_t m_lastMouseY = 0;
    int16_t m_curMouseX = 0;
    int16_t m_curMouseY = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

    size_t m_cbvDescriptorsPerFrame;
};