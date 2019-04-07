#include "d3d12.h"
#include "wrl.h"

namespace D3D12Util
{
    const D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource, UINT subresource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    const D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    class ConstantBuffer
    {
        ConstantBuffer(ID3D12Device* const device, const size_t elementCount, const size_t elementSize);
        ~ConstantBuffer();

        ConstantBuffer(const ConstantBuffer& other) = delete;
        ConstantBuffer& operator=(const ConstantBuffer& other) = delete;

        ID3D12Resource1* getResource() const { return m_pUploadBuffer.Get(); }
        void copyData(const void* const data, const size_t dataSize);
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource1> m_pUploadBuffer;
        void *m_pMappedBuffer = nullptr;
    };
}