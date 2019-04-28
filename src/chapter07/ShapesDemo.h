#pragma once

#include <memory>
#include <vector>

#include "AppBase.h"
#include "D3D12Util.h"

class ShapesDemo : public AppBase
{
public:
    using AppBase::AppBase;
    virtual void initialize() override final;
    virtual void update(float dt) override final;
    virtual void render() override final;
    virtual void onMouseDown(int16_t xPos, int16_t yPos, uint8_t buttons) override final;
    virtual void onMouseUp(int16_t xPos, int16_t yPos, uint8_t buttons) override final;
    virtual void onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons) override final;

private:
    struct PassConstants {};
    struct ObjectConstants {};
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

    static constexpr size_t m_frameResourcesCount = 3u;
    std::vector<FrameResources> m_frameResources;
    size_t m_curFrameResourcesIndex = 0;
};