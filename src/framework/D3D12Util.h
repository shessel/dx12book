#pragma once

#include <cinttypes>

#include "d3d12.h"
#include "wrl.h"

namespace D3D12Util
{
    const D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource, UINT subresource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    const D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    class MappedGPUBuffer
    {
    public:

        struct Flags
        {
            enum Enum : uint8_t {
                None = 0u,
                ConstantBuffer = 1u,
            };
        };

        MappedGPUBuffer(ID3D12Device* const device, const size_t elementCount, const size_t elementSize,
            const uint8_t flags = Flags::None);
        ~MappedGPUBuffer();

        MappedGPUBuffer(const MappedGPUBuffer& other) = delete;
        MappedGPUBuffer& operator=(const MappedGPUBuffer& other) = delete;

        ID3D12Resource1* getResource() const { return m_pUploadBuffer.Get(); }
        Microsoft::WRL::ComPtr<ID3D12Resource1> getResourceComPtr() const { return m_pUploadBuffer; }
        void copyData(const void* const data, const size_t dataSize, const size_t byteOffset = 0);
        const UINT getSize() const { return m_sizeInBytes; }
        const UINT getElementSize() const { return m_elementSizeInBytes; }
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource1> m_pUploadBuffer;
        UINT m_sizeInBytes = 0;
        UINT m_elementSizeInBytes = 0;
        void *m_pMappedBuffer = nullptr;
    };

    Microsoft::WRL::ComPtr<ID3DBlob> compileShader(const wchar_t* const fileName, const char* const entryPoint,
        const char* const target, const D3D_SHADER_MACRO* const pDefines = nullptr);

    void createAndUploadBuffer(const void* const data, const size_t dataSize,
        ID3D12GraphicsCommandList* const commandList, ID3D12Resource** buffer, ID3D12Resource** uploadBuffer);
}