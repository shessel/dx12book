#include "DdsTexture.h"

#include "DDSTextureLoader12.h"
#include "d3dx12.h"

#include "DebugUtil.h"
#include "D3D12Util.h"

void DdsTexture::createFromFileAndUpload(ID3D12GraphicsCommandList* const commandList, const wchar_t* const filename)
{
    Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
    ThrowIfFailed(commandList->GetDevice(IID_PPV_ARGS(&pDevice)));

    ThrowIfFailed(DirectX::LoadDDSTextureFromFile(pDevice.Get(), filename, &m_pResource, m_pDdsData, m_subresources));
    
    UINT subresourceCount = static_cast<UINT>(m_subresources.size());
    UINT64 dataSize = GetRequiredIntermediateSize(m_pResource.Get(), 0u, subresourceCount);

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Alignment = 0;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadDesc.Height = 1;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uploadDesc.MipLevels = 1;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.SampleDesc.Quality = 0;
    uploadDesc.Width = dataSize;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    D3D12_HEAP_FLAGS heapFlags;
    m_pResource->GetHeapProperties(&heapProperties, &heapFlags);
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    ThrowIfFailed(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&m_pUploadResource)));

    UpdateSubresources<16>(commandList, m_pResource.Get(), m_pUploadResource.Get(), 0u, 0u, subresourceCount, m_subresources.data());

    auto transitionBarrier = D3D12Util::TransitionBarrier(m_pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    transitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &transitionBarrier);

    return;
}
