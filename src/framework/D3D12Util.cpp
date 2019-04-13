#include "D3D12Util.h"

#include "DebugUtil.h"

namespace D3D12Util
{
    const D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource, UINT subresource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;
        return barrier;
    }

    const D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
    {
        return TransitionBarrier(resource, 0, stateBefore, stateAfter);
    }

    ConstantBuffer::ConstantBuffer(ID3D12Device* const device, const size_t elementCount, const size_t elementSize)
    {
        const size_t elementSizeMultipleOf256 = (elementSize + 0xffull) & (~0xffull);
        m_sizeInBytes = static_cast<UINT>(elementCount * elementSizeMultipleOf256);

        D3D12_RESOURCE_DESC desc = {};
        desc.Alignment = 0;
        desc.DepthOrArraySize = 1;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Height = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Width = m_sizeInBytes;

        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_pUploadBuffer.GetAddressOf())));

        ThrowIfFailed(m_pUploadBuffer->Map(0, nullptr, &m_pMappedBuffer));
    }

    ConstantBuffer::~ConstantBuffer()
    {
        if (m_pUploadBuffer)
        {
            m_pUploadBuffer->Unmap(0, nullptr);
        }
    }

    void ConstantBuffer::copyData(const void* const data, const size_t dataSize)
    {
        memcpy(m_pMappedBuffer, data, dataSize);
    }
}