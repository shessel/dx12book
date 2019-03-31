#include <cassert>
#include <exception>
#include <iostream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "wrl.h"
#include "dxgi1_6.h"
#include "d3d12.h"


inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WinMain(HINSTANCE hinst, HINSTANCE /*hprev*/, LPSTR /*cmdline*/, int show)
{
    // general config
    constexpr DXGI_FORMAT backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr UINT swapChainBufferCount = 2;
    constexpr UINT msaaSampleCount = 1;
    constexpr UINT windowWidth = 1280;
    constexpr UINT windowHeight = 720;
    constexpr D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    {
        WNDCLASS windowClass = {};
        windowClass.lpfnWndProc = MsgProc;
        windowClass.hInstance = hinst;
        //windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
        //windowClass.hCursor = LoadCursor(0, IDC_ARROW);
        //windowClass.hbrBackground = GetStockObject(WHITE_BRUSH);
        windowClass.lpszClassName = "d3dWindow";
        RegisterClass(&windowClass);
    }

    HWND hwnd = CreateWindow("d3dWindow", "d3dWindow", WS_OVERLAPPEDWINDOW,
                   CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
                   0, 0, hinst, 0);

    // enable d3d12 debug layers in debug builds
#ifndef NDEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug3> debugInterface;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
    debugInterface->EnableDebugLayer();
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory7> pFactory;
    CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));

    // try to create device with minimum feature level
    Microsoft::WRL::ComPtr<ID3D12Device5> pDevice;
    HRESULT hr = D3D12CreateDevice(nullptr, minimumFeatureLevel, IID_PPV_ARGS(&pDevice));

    if (FAILED(hr))
    {
        // try to create fallback device
        Microsoft::WRL::ComPtr<IDXGIAdapter4> pWarpAdapter;
        pFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));
        ThrowIfFailed(D3D12CreateDevice(nullptr, minimumFeatureLevel, IID_PPV_ARGS(&pDevice)));
    }

    Microsoft::WRL::ComPtr<ID3D12Fence1> pFence;
    ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));

    UINT rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    //UINT dsvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    //UINT cbvSrvUavDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaFeatureData = {};
    msaaFeatureData.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msaaFeatureData.Format = backbufferFormat;
    msaaFeatureData.SampleCount = msaaSampleCount;
    ThrowIfFailed(pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaFeatureData, sizeof(msaaFeatureData)));

    // all feature level 11.0 adapters need to support 4xMSAA
    assert(msaaFeatureData.NumQualityLevels > 0);
    const UINT msaaQualityLevel = msaaFeatureData.NumQualityLevels - 1;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue;
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&pCommandQueue)));
    }

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;
    ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)));

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> pCommandList;
    ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList)));
    ThrowIfFailed(pCommandList->Close());

    Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
    {
        pSwapChain.Reset();
        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferCount = swapChainBufferCount;
        desc.BufferDesc.Format = backbufferFormat;
        desc.BufferDesc.Width = windowWidth;
        desc.BufferDesc.Height = windowHeight;
        desc.BufferDesc.RefreshRate.Numerator = 60;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        desc.OutputWindow = hwnd;
        desc.SampleDesc.Count = msaaSampleCount;
        desc.SampleDesc.Quality = msaaQualityLevel;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Windowed = true;

        ThrowIfFailed(pFactory->CreateSwapChain(pCommandQueue.Get(), &desc, &pSwapChain));
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = swapChainBufferCount;
        ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescriptorHeap)));
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = 1;
        ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvDescriptorHeap)));
    }

    UINT currenBackBufferId = 0;

    const auto getCurrentBackBufferView = [&]() {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += currenBackBufferId*rtvDescriptorSize;
    };

    const auto getCurrentDepthStencilView = [&]() {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffers[swapChainBufferCount];
    for (UINT i = 0; i < swapChainBufferCount; ++i)
    {
        ThrowIfFailed(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffers[i])));
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        // pDesc can be nullptr if resource was not created as typeless
        pDevice->CreateRenderTargetView(swapChainBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }

    ShowWindow(hwnd, show);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}