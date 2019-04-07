#include "BoxDemo.h"

#include <iterator>

#include "d3d12.h"
#include "dxgi.h"

#include "DebugUtil.h"

void BoxDemo::initialize()
{
    m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

    {
        D3D12_INPUT_ELEMENT_DESC positionElementDesc = {};
        positionElementDesc.AlignedByteOffset = 0;
        positionElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        positionElementDesc.InputSlot = 0;
        positionElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        positionElementDesc.InstanceDataStepRate = 0;
        positionElementDesc.SemanticName = "POSITION";
        positionElementDesc.SemanticIndex = 0;

        D3D12_INPUT_ELEMENT_DESC colorElementDesc = {};
        colorElementDesc.AlignedByteOffset = 12;
        colorElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        colorElementDesc.InputSlot = 0;
        colorElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        colorElementDesc.InstanceDataStepRate = 0;
        colorElementDesc.SemanticName = "COLOR";
        colorElementDesc.SemanticIndex = 0;

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = { positionElementDesc, colorElementDesc };
        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
        inputLayoutDesc.NumElements = sizeof(inputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputElementDescs;
    }

    {
        m_vertices =
        {
            {DirectX::XMFLOAT3( 1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)},
            {DirectX::XMFLOAT3( 1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)},
            {DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)},
            {DirectX::XMFLOAT3( 1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)},
            {DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
            {DirectX::XMFLOAT3( 1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)},
            {DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)},
        };

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
        desc.Width = m_vertices.size() * sizeof(Vertex);

        ThrowIfFailed(m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer)));
        
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        ThrowIfFailed(m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBufferUpload)));

        Vertex* mappedBufferUpload = nullptr;
        // a range where end <= begin specifies no CPU read, see https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map
        D3D12_RANGE readRangeNoRead{ 0,0 };
        ThrowIfFailed(m_vertexBufferUpload->Map(0, &readRangeNoRead, reinterpret_cast<void**>(&mappedBufferUpload)));
        memcpy(mappedBufferUpload, m_vertices.data(), m_vertices.size() * sizeof(Vertex));
        // nullptr range specifies the CPU might have written to the whole resource
        m_vertexBufferUpload->Unmap(0, nullptr);

        m_pCommandList->CopyResource(m_vertexBuffer.Get(), m_vertexBufferUpload.Get());

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_vertexBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        barrier.Transition.Subresource = 0;
        m_pCommandList->ResourceBarrier(1, &barrier);
    }

    ThrowIfFailed(m_pCommandList->Close());
    {
        ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
        m_pCommandQueue->ExecuteCommandLists(1, commandLists);
    }

    flushCommandQueue();
}