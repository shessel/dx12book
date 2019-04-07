#include "BoxDemo.h"

#include <iterator>

#include "d3d12.h"
#include "dxgi.h"

#include "DebugUtil.h"

namespace
{
    void createAndUploadBuffer(void* data, size_t dataSize, ID3D12Device* const device,
        ID3D12GraphicsCommandList* const commandList, ID3D12Resource** buffer, ID3D12Resource** uploadBuffer)
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

        ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(buffer)));

        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer)));

        void* mappedBufferUpload = nullptr;
        // a range where end <= begin specifies no CPU read.
        // see https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map
        D3D12_RANGE readRangeNoRead{ 0,0 };
        ThrowIfFailed((*uploadBuffer)->Map(0, &readRangeNoRead,&mappedBufferUpload));
        memcpy(mappedBufferUpload, data, dataSize);
        // nullptr range specifies the CPU might have written to the whole resource
        (*uploadBuffer)->Unmap(0, nullptr);

        commandList->CopyResource(*buffer, *uploadBuffer);
    }
}

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
        {
            m_vertices =
            {
                {DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)},
                {DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)},
                {DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
                {DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)},
                {DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)},
                {DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)},
                {DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)},
                {DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)},
            };

            createAndUploadBuffer(m_vertices.data(), m_vertices.size() * sizeof(Vertex),
                m_pDevice.Get(), m_pCommandList.Get(),
                reinterpret_cast<ID3D12Resource**>(m_pVertexBuffer.GetAddressOf()),
                reinterpret_cast<ID3D12Resource**>(m_pVertexBufferUpload.GetAddressOf()));

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = m_pVertexBuffer.Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            barrier.Transition.Subresource = 0;
            m_pCommandList->ResourceBarrier(1, &barrier);
        }

        {
            m_indices = 
            {
                // front
                0, 2, 1,
                1, 2, 3,

                // back
                7, 5, 6,
                6, 5, 4,

                // left
                0, 2, 6,
                4, 0, 6,

                // right
                3, 1, 7,
                7, 1, 5,

                // top
                1, 0, 4,
                1, 4, 5,

                // bottom
                2, 6, 3,
                6, 3, 7,
            };

            createAndUploadBuffer(m_indices.data(), m_indices.size() * sizeof(std::uint16_t),
                m_pDevice.Get(), m_pCommandList.Get(),
                reinterpret_cast<ID3D12Resource**>(m_pIndexBuffer.GetAddressOf()),
                reinterpret_cast<ID3D12Resource**>(m_pIndexBufferUpload.GetAddressOf()));

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = m_pIndexBuffer.Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
            barrier.Transition.Subresource = 0;
            m_pCommandList->ResourceBarrier(1, &barrier);
        }
    }

    ThrowIfFailed(m_pCommandList->Close());

    {
        ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
        m_pCommandQueue->ExecuteCommandLists(1, commandLists);
    }

    flushCommandQueue();
}

void BoxDemo::render()
{
    m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    indexBufferView.SizeInBytes = static_cast<UINT>(m_indices.size() * sizeof(std::uint16_t));
    m_pCommandList->IASetIndexBuffer(&indexBufferView);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = static_cast<UINT>(m_vertices.size() * sizeof(Vertex));
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_pCommandList->DrawIndexedInstanced(static_cast<UINT>(m_indices.size()), 1, 0, 0, 0);

    m_pCommandList->SetPipelineState(&pipelineState);
    
    ThrowIfFailed(m_pCommandList->Close());

    {
        ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
        m_pCommandQueue->ExecuteCommandLists(1, commandLists);
    }

    flushCommandQueue();
}
