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
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NumDescriptors = 1;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pCbvHeap.GetAddressOf())));
    }

    {
        m_pConstantBuffer = std::make_unique<D3D12Util::ConstantBuffer>(m_pDevice.Get(), 1, sizeof(PerObjectConstants));

        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = m_pConstantBuffer->getResource()->GetGPUVirtualAddress();
        desc.SizeInBytes = m_pConstantBuffer->getSize();
        m_pDevice->CreateConstantBufferView(&desc, m_pCbvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    {
        D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
        descriptorRange.BaseShaderRegister = 0;
        descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        descriptorRange.NumDescriptors = 1;
        descriptorRange.OffsetInDescriptorsFromTableStart = 0; // D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND?
        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        descriptorRange.RegisterSpace = 0;

        D3D12_ROOT_PARAMETER1 parameter = {};
        parameter.DescriptorTable.NumDescriptorRanges = 1;
        parameter.DescriptorTable.pDescriptorRanges = &descriptorRange;
        parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rootSignatureDesc.NumParameters = 1;
        rootSignatureDesc.pParameters = &parameter;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
        versionedRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        versionedRootSignatureDesc.Desc_1_1 = rootSignatureDesc;

        Microsoft::WRL::ComPtr<ID3DBlob> pBlob, pErrorBlob;
        D3D12SerializeVersionedRootSignature(&versionedRootSignatureDesc, &pBlob, &pErrorBlob);
        m_pDevice->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
    }

    m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
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

            const D3D12_RESOURCE_BARRIER barrier = D3D12Util::TransitionBarrier(m_pVertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            m_pCommandList->ResourceBarrier(1, &barrier);
        }

        {
            m_indices = 
            {
                // front
                0, 1, 2,
                1, 3, 2,

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
                2, 3, 6,
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

    m_pVertexShader = D3D12Util::CompileShader(L"data/shaders/chapter06/simple.hlsl", "vs", "vs_5_1");
    m_pPixelShader = D3D12Util::CompileShader(L"data/shaders/chapter06/simple.hlsl", "ps", "ps_5_1");

    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        desc.RasterizerState.FrontCounterClockwise = FALSE;
        desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.RasterizerState.DepthClipEnable = TRUE;
        desc.RasterizerState.MultisampleEnable = FALSE;
        desc.RasterizerState.AntialiasedLineEnable = FALSE;
        desc.RasterizerState.ForcedSampleCount = 0;
        desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        desc.BlendState.AlphaToCoverageEnable = FALSE;
        desc.BlendState.IndependentBlendEnable = FALSE;
        D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
        {
            FALSE,FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };
        desc.BlendState.RenderTarget[0] = defaultRenderTargetBlendDesc;

        desc.DepthStencilState.DepthEnable = TRUE;
        desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        desc.DepthStencilState.StencilEnable = FALSE;
        desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
            { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        desc.DepthStencilState.FrontFace = defaultStencilOp;
        desc.DepthStencilState.BackFace = defaultStencilOp;

        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        desc.InputLayout = inputLayoutDesc;
        desc.NumRenderTargets = 1;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.pRootSignature = m_pRootSignature.Get();
        desc.PS.BytecodeLength = m_pPixelShader->GetBufferSize();
        desc.PS.pShaderBytecode = m_pPixelShader->GetBufferPointer();
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleMask = UINT_MAX;
        desc.VS.BytecodeLength = m_pVertexShader->GetBufferSize();
        desc.VS.pShaderBytecode = m_pVertexShader->GetBufferPointer();

        ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pPipelineState)));
    }

    ThrowIfFailed(m_pCommandList->Close());

    {
        ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
        m_pCommandQueue->ExecuteCommandLists(1, commandLists);
    }

    flushCommandQueue();
}

void BoxDemo::update(float dt)
{
    PerObjectConstants perObjectConstants = {};
    static float t = 0.0f;
    float x, y, z;
    DirectX::XMScalarSinCos(&x, &z, t);
    y = 2.0f*x;

    DirectX::XMVECTOR camera = { 2.0f*x, y, 2.0f*z };
    DirectX::XMVECTOR focus = { 0.0f, 0.0f, 0.0f };
    DirectX::XMVECTOR up = { 0.0f, 1.0f, 0.0f };

    DirectX::XMStoreFloat4x4(&perObjectConstants.model, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&perObjectConstants.view, DirectX::XMMatrixLookAtRH(camera, focus, up));
    DirectX::XMStoreFloat4x4(&perObjectConstants.projection, DirectX::XMMatrixPerspectiveFovRH(30.0f, static_cast<float>(m_windowWidth) / m_windowHeight, 0.1f, 5.0f));
    m_pConstantBuffer->copyData(static_cast<void*>(&perObjectConstants), sizeof(PerObjectConstants));

    t += dt;
}

void BoxDemo::render()
{
    ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get()));

    {
        D3D12_RESOURCE_BARRIER presentToRenderTargetTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_pCommandList->ResourceBarrier(1, &presentToRenderTargetTransition);
    }

    {
        const float clearColorRgba[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_pCommandList->ClearRenderTargetView(getCurrentBackBufferView(), clearColorRgba, 0, nullptr);

        m_pCommandList->ClearDepthStencilView(getCurrentDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
    ID3D12DescriptorHeap* descriptorHeaps = { m_pCbvHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(sizeof(descriptorHeaps) / sizeof(ID3D12DescriptorHeap*), &descriptorHeaps);
    m_pCommandList->SetGraphicsRootDescriptorTable(0, m_pCbvHeap->GetGPUDescriptorHandleForHeapStart());

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

    m_pCommandList->DrawIndexedInstanced(static_cast<UINT>(m_indices.size()), 1, 0, 0, 0);

    {
        D3D12_RESOURCE_BARRIER renderTargetToPresentTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_pCommandList->ResourceBarrier(1, &renderTargetToPresentTransition);
    }
    
    ThrowIfFailed(m_pCommandList->Close());

    {
        ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
        m_pCommandQueue->ExecuteCommandLists(1, commandLists);
    }

    ThrowIfFailed(m_pSwapChain->Present(1, 0));

    flushCommandQueue();
    ThrowIfFailed(m_pCommandAllocator->Reset());

    m_currenBackBufferId = (m_currenBackBufferId + 1) % m_swapChainBufferCount;
}
