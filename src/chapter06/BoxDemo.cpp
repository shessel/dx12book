#include "BoxDemo.h"

#include <iterator>

#include "d3d12.h"
#include "dxgi.h"

#include "DebugUtil.h"
#include "D3D12Util.h"

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

    m_pMesh = std::make_unique<Mesh>();
    {
        std::vector<DirectX::XMFLOAT3> vertices =
        {
            DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f),
            DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),
            DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f),
            DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),
            DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f),
            DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f),
            DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f),
            DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f),
        };

        m_pMesh->createVertexBuffer(vertices.data(), vertices.size(), sizeof(DirectX::XMFLOAT3), m_pCommandList.Get(), 0);
    }

    {
        std::vector<DirectX::XMFLOAT3> colors =
        {
            DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f),
            DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f),
            DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f),
            DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
        };

        m_pMesh->createVertexBuffer(colors.data(), colors.size(), sizeof(DirectX::XMFLOAT3), m_pCommandList.Get(), 1);
    }

    {
        std::vector<std::uint16_t> indices =
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

        m_pMesh->createIndexBuffer(indices.data(), indices.size(), sizeof(std::uint16_t), m_pCommandList.Get());
    }

    m_pVertexShader = D3D12Util::compileShader(L"data/shaders/chapter06/simple.hlsl", "vs", "vs_5_1");
    m_pPixelShader = D3D12Util::compileShader(L"data/shaders/chapter06/simple.hlsl", "ps", "ps_5_1");

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
        colorElementDesc.AlignedByteOffset = 0;
        colorElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        colorElementDesc.InputSlot = 1;
        colorElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        colorElementDesc.InstanceDataStepRate = 0;
        colorElementDesc.SemanticName = "COLOR";
        colorElementDesc.SemanticIndex = 0;

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = { positionElementDesc, colorElementDesc };
        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
        inputLayoutDesc.NumElements = sizeof(inputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputElementDescs;

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

void BoxDemo::update(float /*dt*/)
{
    float sinPhi, cosPhi, sinTheta, cosTheta;
    DirectX::XMScalarSinCos(&sinPhi, &cosPhi, m_camAnglePhi);
    DirectX::XMScalarSinCos(&sinTheta, &cosTheta, m_camAngleTheta);

    float x = m_camDistance * sinPhi * cosTheta;
    float y = m_camDistance * sinTheta;
    float z = m_camDistance * cosPhi * cosTheta;;

    DirectX::XMVECTOR camera = { x, y, z };
    DirectX::XMVECTOR focus = { 0.0f, 0.0f, 0.0f };
    DirectX::XMVECTOR up = { 0.0f, 1.0f, 0.0f };

    PerObjectConstants perObjectConstants = {};
    DirectX::XMStoreFloat4x4(&perObjectConstants.model, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&perObjectConstants.view, DirectX::XMMatrixLookAtRH(camera, focus, up));
    DirectX::XMStoreFloat4x4(&perObjectConstants.projection, DirectX::XMMatrixPerspectiveFovRH(30.0f, static_cast<float>(m_windowWidth) / m_windowHeight, 0.1f, 10.0f));
    perObjectConstants.time = m_timer.getElapsedTime();
    m_pConstantBuffer->copyData(static_cast<void*>(&perObjectConstants), sizeof(PerObjectConstants));
}

void BoxDemo::render()
{
    ThrowIfFailed(m_pCommandAllocator->Reset());
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

    ID3D12DescriptorHeap* const descriptorHeaps = { m_pCbvHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(sizeof(descriptorHeaps) / sizeof(ID3D12DescriptorHeap*), &descriptorHeaps);
    m_pCommandList->SetGraphicsRootDescriptorTable(0, m_pCbvHeap->GetGPUDescriptorHandleForHeapStart());

    const D3D12_INDEX_BUFFER_VIEW indexBufferView = m_pMesh->getIndexBufferView();
    m_pCommandList->IASetIndexBuffer(&indexBufferView);

    const D3D12_VERTEX_BUFFER_VIEW vertexBufferViewVertices = m_pMesh->getVertexBufferView(0);
    const D3D12_VERTEX_BUFFER_VIEW vertexBufferViewColors = m_pMesh->getVertexBufferView(1);
    const D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[] = { vertexBufferViewVertices, vertexBufferViewColors };
    m_pCommandList->IASetVertexBuffers(0, 2, vertexBufferViews);

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

    m_pCommandList->DrawIndexedInstanced(static_cast<UINT>(m_pMesh->m_indexCount), 1, 0, 0, 0);

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

    // use GetCurrentBackBufferIndex?
    m_currenBackBufferId = (m_currenBackBufferId + 1) % m_swapChainBufferCount;

    flushCommandQueue();
}

void BoxDemo::onMouseDown(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
}

void BoxDemo::onMouseUp(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
}

void BoxDemo::onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons)
{
    m_curMouseX = xPos;
    m_curMouseY = yPos;
    int16_t dMouseX = m_curMouseX - m_lastMouseX;
    int16_t dMouseY = m_curMouseY - m_lastMouseY;
    if (buttons & MouseButton::Left)
    {
        m_camAnglePhi += 0.02f * dMouseX;
        m_camAngleTheta += 0.02f * -dMouseY;
        m_camAngleTheta = std::fmax(m_camAngleTheta, -DirectX::XM_PIDIV2 + 0.001f);
        m_camAngleTheta = std::fmin(m_camAngleTheta, DirectX::XM_PIDIV2 - 0.001f);
    }
    if (buttons & MouseButton::Right)
    {
        m_camDistance += 0.02f * dMouseY;
        m_camDistance = max(m_camDistance, 0.1f); 
    }
    m_lastMouseX = m_curMouseX;
    m_lastMouseY = m_curMouseY;
}
