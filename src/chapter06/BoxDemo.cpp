#include "BoxDemo.h"

#include <iterator>

#include "d3d12.h"
#include "dxgi.h"

#include "DebugUtil.h"
#include "D3D12Util.h"

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

            const D3D12_RESOURCE_BARRIER barrier = D3D12Util::TransitionBarrier(m_pIndexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
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

            const D3D12_RESOURCE_BARRIER barrier = D3D12Util::TransitionBarrier(m_pIndexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
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
    ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

    {
        D3D12_RESOURCE_BARRIER presentToRenderTargetTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_pCommandList->ResourceBarrier(1, &presentToRenderTargetTransition);
    }

    {
        const float clearColorRgba[] = { 0.0f, 0.0f, 0.5f, 1.0f };
        m_pCommandList->ClearRenderTargetView(getCurrentBackBufferView(), clearColorRgba, 0, nullptr);

        m_pCommandList->ClearDepthStencilView(getCurrentDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

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
        D3D12_RESOURCE_BARRIER renderTargetToPresentTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_pCommandList->ResourceBarrier(1, &renderTargetToPresentTransition);
    }

    m_pCommandList->DrawIndexedInstanced(static_cast<UINT>(m_indices.size()), 1, 0, 0, 0);
    
    ThrowIfFailed(m_pCommandList->Close());

    {
        ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
        m_pCommandQueue->ExecuteCommandLists(1, commandLists);
    }

    ThrowIfFailed(m_pSwapChain->Present(0, 0));

    flushCommandQueue();

    m_currenBackBufferId = (m_currenBackBufferId + 1) % m_swapChainBufferCount;
}
