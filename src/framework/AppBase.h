#pragma once

#include <array>

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "windowsx.h"

#include "wrl.h"
#include "dxgi1_6.h"
#include "d3d12.h"

#include "Timer.h"

class AppBase
{
public:
    AppBase(HINSTANCE hInstance);
    ~AppBase();
    AppBase(const AppBase& other) = delete;
    AppBase(AppBase&& other) = delete;
    AppBase& operator=(const AppBase& other) = delete;
    AppBase& operator=(AppBase&& other) = delete;

    enum MouseButton : uint8_t
    {
        Left = 1u,
        Middle = 1u << 1u,
        Right = 1u << 2u,
    };

    int run();
    virtual void initialize() = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
    virtual void onMouseDown(int16_t xPos, int16_t yPos, uint8_t buttons) = 0;
    virtual void onMouseUp(int16_t xPos, int16_t yPos, uint8_t buttons) = 0;
    virtual void onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons) = 0;
protected:
    void createWindow();
    void initializeDirect3D();
    ID3D12Resource* const getCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE getCurrentDepthStencilView() const;
    void flushCommandQueue();

    static constexpr size_t m_staticSamplerCount = 6;
    std::array<D3D12_STATIC_SAMPLER_DESC, m_staticSamplerCount> createDefaultStaticSamplerDescs() const;

    HINSTANCE m_hInstance;
    HWND m_hWnd;
    UINT m_windowWidth = 1280u;
    UINT m_windowHeight = 720u;

    Microsoft::WRL::ComPtr<IDXGIFactory7> m_pFactory;
    Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence1> m_pFence;

    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_cbvSrvUavDescriptorSize;
    UINT m_msaaQualityLevel;
    UINT m_currenBackBufferId = 0;
    UINT64 m_flushFenceValue = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pCommandList;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_pSwapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;

    static constexpr UINT m_swapChainBufferCount = 2;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffers[m_swapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

    Timer m_timer;
};