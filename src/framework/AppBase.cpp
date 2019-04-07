#include "AppBase.h"

#include <cassert>
#include <exception>

#include "DebugUtil.h"

namespace
{
    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_ENTERSIZEMOVE:
            // pause
            break;
        case WM_EXITSIZEMOVE:
            // resize
            break;
        case WM_GETMINMAXINFO:
            // min window size
            break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            // mouse down
            // GET_X_LPARAM(lParam)
            break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        return 0;
    }
}

AppBase::AppBase(HINSTANCE hInstance) : m_hInstance(hInstance)
{
    createWindow();
    initializeDirect3D();
}

int AppBase::run()
{
    ShowWindow(m_hWnd, SW_SHOWDEFAULT);

    MSG msg = {};
    m_timer.reset();

    while (msg.message != WM_QUIT) {
        m_timer.tick();

        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

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

        ++m_frameId;
        ThrowIfFailed(m_pCommandQueue->Signal(m_pFence.Get(), m_frameId));
        if (m_pFence->GetCompletedValue() < m_frameId)
        {
            HANDLE eventHandle = CreateEventEx(nullptr, L"Flush Command Queue Event", 0, EVENT_ALL_ACCESS);
            ThrowIfFailed(m_pFence->SetEventOnCompletion(m_frameId, eventHandle));
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
		ThrowIfFailed(m_pCommandAllocator->Reset());

        m_currenBackBufferId = (m_currenBackBufferId + 1) % m_swapChainBufferCount;
    }

    return static_cast<int>(msg.wParam);
}

void AppBase::createWindow()
{
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = MsgProc;
    windowClass.hInstance = m_hInstance;
    windowClass.lpszClassName = L"d3dWindow";
    RegisterClass(&windowClass);

    m_hWnd = CreateWindow(L"d3dWindow", L"d3dWindow", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, m_windowWidth, m_windowHeight,
        0, 0, m_hInstance, 0);
}


void AppBase::initializeDirect3D()
{
    // general config
    constexpr DXGI_FORMAT backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
    constexpr UINT msaaSampleCount = 1;
    constexpr D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    // enable d3d12 debug layers in debug builds
#ifndef NDEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug3> debugInterface;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
    debugInterface->EnableDebugLayer();
#endif

    CreateDXGIFactory1(IID_PPV_ARGS(&m_pFactory));

    // try to create device with minimum feature level
    HRESULT hr = D3D12CreateDevice(nullptr, minimumFeatureLevel, IID_PPV_ARGS(&m_pDevice));

    if (FAILED(hr))
    {
        // try to create fallback device
        Microsoft::WRL::ComPtr<IDXGIAdapter4> pWarpAdapter;
        m_pFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));
        ThrowIfFailed(D3D12CreateDevice(nullptr, minimumFeatureLevel, IID_PPV_ARGS(&m_pDevice)));
    }

    ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));

    m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvSrvUavDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaFeatureData = {};
        msaaFeatureData.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        msaaFeatureData.Format = backbufferFormat;
        msaaFeatureData.SampleCount = msaaSampleCount;
        ThrowIfFailed(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaFeatureData, sizeof(msaaFeatureData)));

        // all feature level 11.0 adapters need to support 4xMSAA
        assert(msaaFeatureData.NumQualityLevels > 0);
        m_msaaQualityLevel = msaaFeatureData.NumQualityLevels - 1;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pCommandQueue)));
    }

    ThrowIfFailed(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator)));

    ThrowIfFailed(m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pCommandList)));
    ThrowIfFailed(m_pCommandList->Close());

    {
        m_pSwapChain.Reset();
        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferCount = m_swapChainBufferCount;
        desc.BufferDesc.Format = backbufferFormat;
        desc.BufferDesc.Width = m_windowWidth;
        desc.BufferDesc.Height = m_windowHeight;
        desc.BufferDesc.RefreshRate.Numerator = 60;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        desc.OutputWindow = m_hWnd;
        desc.SampleDesc.Count = msaaSampleCount;
        desc.SampleDesc.Quality = m_msaaQualityLevel;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Windowed = true;

        ThrowIfFailed(m_pFactory->CreateSwapChain(m_pCommandQueue.Get(), &desc, &m_pSwapChain));
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = m_swapChainBufferCount;
        ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = 1;
        ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvDescriptorHeap)));
    }
    
    for (UINT i = 0; i < m_swapChainBufferCount; ++i)
    {
        ThrowIfFailed(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i])));
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += i * m_rtvDescriptorSize;

        // pDesc can be nullptr if resource was not created as typeless
        m_pDevice->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHandle);
    }

    {
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc = {};
        desc.Alignment = 0;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        desc.Format = depthStencilFormat;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.SampleDesc.Count = msaaSampleCount;
        desc.SampleDesc.Quality = m_msaaQualityLevel;
        desc.Width = m_windowWidth;
        desc.Height = m_windowHeight;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = depthStencilFormat;
        clearValue.DepthStencil.Depth = 1.0f;

        ThrowIfFailed(m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&m_depthStencilBuffer)));

        // pDesc can be nullptr if resource was not created as typeless
        m_pDevice->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, getCurrentDepthStencilView());
    }
}

ID3D12Resource* const AppBase::getCurrentBackBuffer() const {
    return m_swapChainBuffers[m_currenBackBufferId].Get();
};

D3D12_CPU_DESCRIPTOR_HANDLE AppBase::getCurrentBackBufferView() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += m_currenBackBufferId * m_rtvDescriptorSize;
    return handle;
};

D3D12_CPU_DESCRIPTOR_HANDLE AppBase::getCurrentDepthStencilView() const {
    return m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
};