#include "LandAndWavesDemo.h"

#include <iostream>

#include "DebugUtil.h"
#include "GeometryUtil.h"

LandAndWavesDemo::~LandAndWavesDemo()
{
    UINT64 maxFenceWaitValue = 0u;
    for (const FrameResources& frameResources : m_frameResources)
    {
        maxFenceWaitValue = maxFenceWaitValue < frameResources.m_fenceValue ? frameResources.m_fenceValue : maxFenceWaitValue;
    }

    if (m_pFrameFence->GetCompletedValue() < maxFenceWaitValue)
    {
        HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_pFrameFence->SetEventOnCompletion(maxFenceWaitValue, hEvent);
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }
}

void LandAndWavesDemo::onMouseDown(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    SetCapture(m_hWnd);
}

void LandAndWavesDemo::onMouseUp(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    ReleaseCapture();
}

void LandAndWavesDemo::onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons)
{
    m_curMouseX = xPos;
    m_curMouseY = yPos;
    int16_t dMouseX = m_curMouseX - m_lastMouseX;
    int16_t dMouseY = m_curMouseY - m_lastMouseY;
    if (buttons & MouseButton::Left)
    {
        m_camera.updatePhi(-0.02f * dMouseX);
        m_camera.updateTheta(0.02f * dMouseY);
    }
    if (buttons & MouseButton::Middle)
    {
        m_camera.move({ -0.02f * dMouseX, 0.02f * dMouseY, 0.0f });
    }
    if (buttons & MouseButton::Right)
    {
        m_camera.updateDistance(0.02f * dMouseY);
    }
    m_lastMouseX = m_curMouseX;
    m_lastMouseY = m_curMouseY;
}

void LandAndWavesDemo::initialize()
{
    ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

    ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFrameFence)));

    constexpr float width = 25.0f;
    size_t landVertexCount;
    {
        size_t landIndexCount;
        GeometryUtil::calculateVertexIndexCountsSquare(VERTICES_PER_SIDE, landVertexCount, landIndexCount);

        std::unique_ptr<Vertex[]> p_landVertices = std::make_unique<Vertex[]>(landVertexCount);
        std::unique_ptr<uint16_t[]> p_landIndices = std::make_unique<uint16_t[]>(landIndexCount);
        GeometryUtil::createSquare(width, VERTICES_PER_SIDE, p_landVertices.get(), p_landIndices.get());

        for (uint16_t y = 0; y < VERTICES_PER_SIDE; ++y)
        {
            for (uint16_t x = 0; x < VERTICES_PER_SIDE; ++x)
            {
                Vertex& vertex = p_landVertices[y * VERTICES_PER_SIDE + x];
                float sinX = DirectX::XMScalarSinEst(vertex.pos.x * 16.0f / (width));
                float cosZ = DirectX::XMScalarCosEst(vertex.pos.z * 16.0f / (width));
                vertex.pos.y = (6.0f*(vertex.pos.z* sinX + vertex.pos.x*cosZ) + 0.5f) / width;
                if (vertex.pos.y < 0.5f)
                {
                    vertex.col = { 1.0f, 0.96f, 0.62f };
                }
                else if (vertex.pos.y < 1.0f)
                {
                    vertex.col = { 0.48f, 0.77f, 0.46f };
                }
                else if (vertex.pos.y < 2.0f)
                {
                    vertex.col = { 0.1f, 0.48f, 0.19f };
                }
                else if (vertex.pos.y < 2.5f)
                {
                    vertex.col = { 0.45f, 0.39f, 0.34f };
                }
                else
                {
                    vertex.col = { 1.0f, 1.0f, 1.0f };
                }
            }
        }

        m_landMesh.createVertexBuffer(p_landVertices.get(), landVertexCount, sizeof(Vertex), m_pCommandList.Get());
        m_landMesh.createIndexBuffer(p_landIndices.get(), landIndexCount, sizeof(uint16_t), m_pCommandList.Get());
    }

    {
        GeometryUtil::calculateVertexIndexCountsSquare(VERTICES_PER_SIDE, m_wavesVertexCount, m_wavesIndexCount);

        std::unique_ptr<uint16_t[]> pIndices = std::make_unique<uint16_t[]>(m_wavesIndexCount);
        GeometryUtil::createSquare(width, VERTICES_PER_SIDE, m_wavesVertices, pIndices.get());

        D3D12Util::createAndUploadBuffer(pIndices.get(), m_wavesIndexCount * sizeof(uint16_t), m_pCommandList.Get(), &m_pWavesIndexBuffer, &m_pWavesIndexBufferUpload);
    }

    for (FrameResources& frameResources : m_frameResources)
    {
        ThrowIfFailed(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameResources.m_pCommandAllocator)));
        frameResources.m_pCbPass = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), 1, sizeof(PassConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pCbObjects = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), 2u, sizeof(ObjectConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pDynamicVertices = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), landVertexCount, sizeof(Vertex));
    }

    {
        D3D12_ROOT_PARAMETER1 objectCbParameter = {};
        objectCbParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        objectCbParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        objectCbParameter.Descriptor.ShaderRegister = 0;

        D3D12_ROOT_PARAMETER1 passCbParameter = {};
        passCbParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        passCbParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        passCbParameter.Descriptor.ShaderRegister = 1;

        D3D12_ROOT_PARAMETER1 rootParameters[] = {
            objectCbParameter,
            passCbParameter,
        };

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rootSignatureDesc.Desc_1_1.NumParameters = sizeof(rootParameters) / sizeof(D3D12_ROOT_PARAMETER1);
        rootSignatureDesc.Desc_1_1.pParameters = rootParameters;

        Microsoft::WRL::ComPtr<ID3DBlob> pRootSignatureBlob, pRootSignatureErrorBlob;
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &pRootSignatureBlob, &pRootSignatureErrorBlob));
        ThrowIfFailed(m_pDevice->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)));
    }

    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.BlendState.AlphaToCoverageEnable = false;
        desc.BlendState.IndependentBlendEnable = false;
        desc.BlendState.RenderTarget[0].BlendEnable = false;
        desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        desc.BlendState.RenderTarget[0].LogicOpEnable = false;
        desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_DEPTH_STENCILOP_DESC defaultDepthStencilOp = {};
        defaultDepthStencilOp.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        defaultDepthStencilOp.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        defaultDepthStencilOp.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        defaultDepthStencilOp.StencilPassOp = D3D12_STENCIL_OP_KEEP;

        desc.DepthStencilState.BackFace = defaultDepthStencilOp;
        desc.DepthStencilState.FrontFace = defaultDepthStencilOp;
        desc.DepthStencilState.DepthEnable = true;
        desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthStencilState.StencilEnable = false;
        desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        desc.RasterizerState.AntialiasedLineEnable = false;
        desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.RasterizerState.DepthClipEnable = true;
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.ForcedSampleCount = 0;
        desc.RasterizerState.FrontCounterClockwise = false;
        desc.RasterizerState.MultisampleEnable = false;
        desc.RasterizerState.SlopeScaledDepthBias =  D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

        desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        D3D12_INPUT_ELEMENT_DESC vertexInputElementDesc = {};
        vertexInputElementDesc.AlignedByteOffset = 0u;
        vertexInputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        vertexInputElementDesc.InputSlot = 0u;
        vertexInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        vertexInputElementDesc.InstanceDataStepRate = 0u;
        vertexInputElementDesc.SemanticIndex = 0u;
        vertexInputElementDesc.SemanticName = "POSITION";

        D3D12_INPUT_ELEMENT_DESC colorInputElementDesc = {};
        colorInputElementDesc.AlignedByteOffset = 12u;
        colorInputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        colorInputElementDesc.InputSlot = 0u;
        colorInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        colorInputElementDesc.InstanceDataStepRate = 0u;
        colorInputElementDesc.SemanticIndex = 0u;
        colorInputElementDesc.SemanticName = "COLOR";

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            vertexInputElementDesc,
            colorInputElementDesc,
        };

        desc.InputLayout.NumElements = sizeof(inputElementDescs)/sizeof(D3D12_INPUT_ELEMENT_DESC);
        desc.InputLayout.pInputElementDescs = inputElementDescs;

        desc.pRootSignature = m_pRootSignature.Get();

        Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader = D3D12Util::compileShader(L"data/shaders/chapter07/landAndWaves.hlsl", "vs", "vs_5_1");
        desc.VS.pShaderBytecode = pVertexShader->GetBufferPointer();
        desc.VS.BytecodeLength = pVertexShader->GetBufferSize();

        Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader = D3D12Util::compileShader(L"data/shaders/chapter07/landAndWaves.hlsl", "ps", "ps_5_1");
        desc.PS.pShaderBytecode = pPixelShader->GetBufferPointer();
        desc.PS.BytecodeLength = pPixelShader->GetBufferSize();

        ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pPipelineState)));
    }

    ThrowIfFailed(m_pCommandList->Close());

    ID3D12CommandList* pCommandList = m_pCommandList.Get();
    m_pCommandQueue->ExecuteCommandLists(1, &pCommandList);
    flushCommandQueue();
    ThrowIfFailed(m_pCommandAllocator->Reset());
};

void LandAndWavesDemo::update(float dt)
{
    m_camera.update();
    m_curFrameResourcesIndex = (m_curFrameResourcesIndex + 1u) % FRAME_RESOURCES_COUNT;
    const FrameResources& curFrameResources = m_frameResources[m_curFrameResourcesIndex];

    if (m_pFrameFence->GetCompletedValue() < curFrameResources.m_fenceValue)
    {
        HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_pFrameFence->SetEventOnCompletion(curFrameResources.m_fenceValue, hEvent));
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    {
        PassConstants passConstants = {};
        passConstants.view = m_camera.m_matrix;
        DirectX::XMStoreFloat4x4(&passConstants.projection, DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(60.0f), static_cast<float>(m_windowWidth) / m_windowHeight, 0.1f, 100.0f));
        passConstants.time = m_timer.getElapsedTime();
        passConstants.dTime = dt;

        curFrameResources.m_pCbPass->copyData(&passConstants, sizeof(PassConstants));
    }

    {
        ObjectConstants objectConstants[2];
        DirectX::XMStoreFloat4x4(&objectConstants[0].world, DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
        curFrameResources.m_pCbObjects->copyData(&objectConstants[0], sizeof(ObjectConstants));

        DirectX::XMStoreFloat4x4(&objectConstants[1].world, DirectX::XMMatrixTranslation(0.0f, -0.25f, 0.0f));
        curFrameResources.m_pCbObjects->copyData(&objectConstants[1], sizeof(ObjectConstants), 256u);
    }

    {
        constexpr float scale = 0.05f;
        for (uint16_t y = 0; y < VERTICES_PER_SIDE; ++y)
        {
            for (uint16_t x = 0; x < VERTICES_PER_SIDE; ++x)
            {
                float offset = 0.0f;
                float iterationScale = 1.0f;
                float iterationCoordOffsetX = 0.0f;
                float iterationCoordOffsetY = 0.0f;
                float iterationCoordScaleX = 5.0f;
                float iterationCoordScaleY = 7.0f;
                for (uint8_t iteration = 0; iteration < 3; ++iteration)
                {
                    const float iterationX = iterationCoordScaleX * (x + iterationCoordOffsetX);
                    const float iterationY = iterationCoordScaleY * (y + iterationCoordOffsetY);
                    offset += iterationScale * DirectX::XMScalarSinEst(m_timer.getElapsedTime() + iterationX + iterationY);
                    iterationScale *= 0.65f;
                    iterationCoordOffsetX += 0.23f;
                    iterationCoordOffsetX = std::fmodf(iterationCoordScaleX, 1.0f);
                    iterationCoordOffsetY += 0.57f;
                    iterationCoordOffsetY = std::fmodf(iterationCoordScaleY, 1.0f);

                    iterationCoordScaleX = 4.25f * std::fmodf(iterationCoordScaleX * 123.0f, 23.0f);
                    iterationCoordScaleY = 4.25f * std::fmodf(iterationCoordScaleY * 123.0f, 43.0f);
                }
                m_wavesVertices[y][x].pos.y = scale*offset;
                m_wavesVertices[y][x].col = {
                    0.2f + 0.01f * offset,
                    0.3f + 0.015f * offset,
                    0.6f + 0.03f * offset
                };
            }
        }
        curFrameResources.m_pDynamicVertices->copyData(m_wavesVertices, m_wavesVertexCount * sizeof(Vertex));
    }
};

void LandAndWavesDemo::render()
{
    FrameResources& curFrameResources = m_frameResources[m_curFrameResourcesIndex];
    curFrameResources.m_fenceValue = ++m_curFrameFenceValue;

    ThrowIfFailed(curFrameResources.m_pCommandAllocator->Reset());
    ThrowIfFailed(m_pCommandList->Reset(curFrameResources.m_pCommandAllocator.Get(), m_pPipelineState.Get()));

    {
        D3D12_RESOURCE_BARRIER presentToRenderTargetTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_pCommandList->ResourceBarrier(1, &presentToRenderTargetTransition);
    }

    m_pCommandList->ClearDepthStencilView(getCurrentDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    constexpr float clearColor[4] = { 0.2f, 0.4f, 0.6f, 1.0f };
    m_pCommandList->ClearRenderTargetView(getCurrentBackBufferView(), clearColor, 0, nullptr);

    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
    m_pCommandList->SetGraphicsRootConstantBufferView(1, curFrameResources.m_pCbPass->getResource()->GetGPUVirtualAddress());

    {
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_windowWidth);
        viewport.Height = static_cast<float>(m_windowHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_pCommandList->RSSetViewports(1, &viewport);
    }

    {
        RECT scissorRect = {};
        scissorRect.right = m_windowWidth;
        scissorRect.bottom = m_windowHeight;
        m_pCommandList->RSSetScissorRects(1, &scissorRect);
    }

    auto renderTarget = getCurrentBackBufferView();
    auto depthTarget = getCurrentDepthStencilView();
    m_pCommandList->OMSetRenderTargets(1, &renderTarget, true, &depthTarget);

    {
        m_pCommandList->SetGraphicsRootConstantBufferView(0, curFrameResources.m_pCbObjects->getResource()->GetGPUVirtualAddress());

        const D3D12_INDEX_BUFFER_VIEW indexBufferView = m_landMesh.getIndexBufferView();
        m_pCommandList->IASetIndexBuffer(&indexBufferView);

        const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = m_landMesh.getVertexBufferView();
        m_pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_pCommandList->DrawIndexedInstanced(static_cast<UINT>(m_landMesh.m_indexCount), 1u, 0u, 0u, 0u);
    }
    {
        m_pCommandList->SetGraphicsRootConstantBufferView(0, curFrameResources.m_pCbObjects->getResource()->GetGPUVirtualAddress() + 256u);

        D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
        indexBufferView.BufferLocation = m_pWavesIndexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        indexBufferView.SizeInBytes = static_cast<UINT>(m_wavesIndexCount) * sizeof(uint16_t);
        m_pCommandList->IASetIndexBuffer(&indexBufferView);

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        vertexBufferView.BufferLocation = curFrameResources.m_pDynamicVertices->getResource()->GetGPUVirtualAddress();
        vertexBufferView.SizeInBytes = static_cast<UINT>(m_wavesVertexCount) * sizeof(Vertex);
        vertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(Vertex));
        m_pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_pCommandList->DrawIndexedInstanced(static_cast<UINT>(m_wavesIndexCount), 1u, 0u, 0u, 0u);
    }

    {
        D3D12_RESOURCE_BARRIER renderTargetToPresentTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_pCommandList->ResourceBarrier(1, &renderTargetToPresentTransition);
    }

    ThrowIfFailed(m_pCommandList->Close());

    ID3D12CommandList* const pCommandList = m_pCommandList.Get();
    m_pCommandQueue->ExecuteCommandLists(1, &pCommandList);

    m_pSwapChain->Present(1, 0);

    m_currenBackBufferId = (m_currenBackBufferId + 1u) % m_swapChainBufferCount;

    ThrowIfFailed(m_pCommandQueue->Signal(m_pFrameFence.Get(), curFrameResources.m_fenceValue));
};
