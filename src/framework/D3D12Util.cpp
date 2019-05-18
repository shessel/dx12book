#include "D3D12Util.h"

#include <cinttypes>

#include "DebugUtil.h"
#include "d3dcompiler.h"

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

    MappedGPUBuffer::MappedGPUBuffer(ID3D12Device* const device, const size_t elementCount, const size_t elementSize, const uint8_t flags)
    {
        // constant buffer size needs to be multiple of 256. CBVs also need to point to 256 byte aligned adresses,
        // so if we want multiple CBVs into one buffer each element needs to be aligned to 256 bytes as well.
        m_elementSizeInBytes = (flags & Flags::ConstantBuffer) ? static_cast<UINT>((elementSize) + 0xffu) & (~0xffu) : static_cast<UINT>(elementSize);
        m_sizeInBytes = static_cast<UINT>(elementCount) * m_elementSizeInBytes;

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

    MappedGPUBuffer::~MappedGPUBuffer()
    {
        if (m_pUploadBuffer)
        {
            m_pUploadBuffer->Unmap(0, nullptr);
        }
    }

    void MappedGPUBuffer::copyData(const void* const data, const size_t dataSize, const size_t byteOffset)
    {
        std::uint8_t* pMappedBufferOffset = static_cast<std::uint8_t*>(m_pMappedBuffer) + byteOffset;
        memcpy(pMappedBufferOffset, data, dataSize);
    }

    Microsoft::WRL::ComPtr<ID3DBlob> compileShader(const wchar_t* const fileName, const char* const entryPoint, const char* const target)
    {
        UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if defined(DEBUG) || defined (_DEBUG)
        shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        UINT effectFlags = 0;
        Microsoft::WRL::ComPtr<ID3DBlob> pCode, pError;
        HRESULT hr = D3DCompileFromFile(fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, shaderFlags, effectFlags, pCode.GetAddressOf(), pError.GetAddressOf());

        if (FAILED(hr))
        {
            OutputDebugStringW(L"Error in D3D12Util::compileShader:\n");
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                OutputDebugStringW(L"Could not open file \"");
                OutputDebugStringW(fileName);
                OutputDebugStringW(L"\"\n");
            }
            else
            {
                OutputDebugStringA(reinterpret_cast<char*>(pError->GetBufferPointer()));
            }
            ThrowIfFailed(hr);
        }

        return pCode;
    }
    void createAndUploadBuffer(const void* const data, const size_t dataSize, ID3D12GraphicsCommandList* const commandList, ID3D12Resource** buffer, ID3D12Resource** uploadBuffer)
    {
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

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
        desc.Width = dataSize;

        Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
        ThrowIfFailed(commandList->GetDevice(IID_PPV_ARGS(&pDevice)));

        ThrowIfFailed(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(buffer)));

        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        ThrowIfFailed(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer)));

        void* mappedBufferUpload = nullptr;
        // a range where end <= begin specifies no CPU read.
        // see https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map
        D3D12_RANGE readRangeNoRead{ 0,0 };
        ThrowIfFailed((*uploadBuffer)->Map(0, &readRangeNoRead, &mappedBufferUpload));
        memcpy(mappedBufferUpload, data, dataSize);
        // nullptr range specifies the CPU might have written to the whole resource
        (*uploadBuffer)->Unmap(0, nullptr);

        commandList->CopyResource(*buffer, *uploadBuffer);
    }
}