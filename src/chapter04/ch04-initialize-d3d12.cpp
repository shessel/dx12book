#include "AppBase.h"
#include "DebugUtil.h"

class InitializeDemo : public AppBase
{
public:
    using AppBase::AppBase;

    virtual void initialize() {};
    virtual void update(float /*dt*/) {};
    virtual void render()
    {
        ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

        {
            D3D12_RESOURCE_BARRIER presentToRenderTargetTransition = {};
            presentToRenderTargetTransition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            presentToRenderTargetTransition.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            presentToRenderTargetTransition.Transition.pResource = getCurrentBackBuffer();
            presentToRenderTargetTransition.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            presentToRenderTargetTransition.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            m_pCommandList->ResourceBarrier(1, &presentToRenderTargetTransition);
        }

        {
            const float clearColorRgba[] = { 0.0f, 0.0f, 0.5f, 1.0f };
            m_pCommandList->ClearRenderTargetView(getCurrentBackBufferView(), clearColorRgba, 0, nullptr);

            m_pCommandList->ClearDepthStencilView(getCurrentDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        }

        {
            D3D12_VIEWPORT viewport = {};
            viewport.Width = static_cast<float>(m_windowWidth);
            viewport.Height = static_cast<float>(m_windowHeight);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;

            m_pCommandList->RSSetViewports(1, &viewport);
        }

        {
            D3D12_RECT scissorRect = {};
            scissorRect.bottom = m_windowHeight;
            scissorRect.right = m_windowWidth;
            m_pCommandList->RSSetScissorRects(1, &scissorRect);
        }

        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = getCurrentBackBufferView();
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = getCurrentDepthStencilView();
            m_pCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
        }

        {
            D3D12_RESOURCE_BARRIER renderTargetToPresentTransition = {};
            renderTargetToPresentTransition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            renderTargetToPresentTransition.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            renderTargetToPresentTransition.Transition.pResource = getCurrentBackBuffer();
            renderTargetToPresentTransition.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            renderTargetToPresentTransition.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            m_pCommandList->ResourceBarrier(1, &renderTargetToPresentTransition);
        }

        ThrowIfFailed(m_pCommandList->Close());

        {
            ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
            m_pCommandQueue->ExecuteCommandLists(1, commandLists);
        }

        ThrowIfFailed(m_pSwapChain->Present(0, 0));

        flushCommandQueue();
        ThrowIfFailed(m_pCommandAllocator->Reset());

        m_currenBackBufferId = (m_currenBackBufferId + 1) % m_swapChainBufferCount;
    }
};

int WinMain(HINSTANCE hinst, HINSTANCE /*hprev*/, LPSTR /*cmdline*/, int /*show*/)
{
    try
    {
        InitializeDemo demo(hinst);
        return demo.run();
    }
    catch (DxDebugException e)
    {
        OutputDebugStringDxDebugException(e);
        return 1;
    }
}