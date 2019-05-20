#include "ShapesDemo.h"

#include "DebugUtil.h"
#include "GeometryUtil.h"
#include "Renderable.h"

ShapesDemo::~ShapesDemo()
{
    for (const FrameResources& frameResources : m_frameResources)
    {
        // wait for GPU to finish before releasing ComPtrs
        if (frameResources.m_fenceValue != 0 && frameResources.m_fenceValue > m_pFence->GetCompletedValue())
        {
            HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
            m_pFence->SetEventOnCompletion(frameResources.m_fenceValue, hEvent);
            WaitForSingleObject(hEvent, INFINITE);
            CloseHandle(hEvent);
        }
    }
}

void ShapesDemo::initialize()
{
    m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

    {

        static constexpr GeometryUtil::VertexAttributeDesc attributeCollection[] =
        {
            {GeometryUtil::VertexAttributeType::POSITION, 0u},
            {GeometryUtil::VertexAttributeType::NORMAL, 12u},
        };

        static constexpr GeometryUtil::VertexDesc vertexDesc = {
            sizeof(attributeCollection) / sizeof(GeometryUtil::VertexAttributeDesc), attributeCollection, 24u
        };

        m_pMesh = std::make_unique<Mesh>();
        {
            struct Vertex {
                DirectX::XMFLOAT3 pos;
                DirectX::XMFLOAT3 col;
            };
            std::vector<Vertex> vertices =
            {
                // cube
                {DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)},
                {DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f),  DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)},
                {DirectX::XMFLOAT3(-1.0f,  1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
                {DirectX::XMFLOAT3(1.0f,  1.0f, -1.0f),  DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)},
                {DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)},
                {DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f),  DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)},
                {DirectX::XMFLOAT3(-1.0f,  1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)},
                {DirectX::XMFLOAT3(1.0f,  1.0f,  1.0f),  DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)},
                                                                                            
                // pyramid                                                                  
                {DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)},
                {DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)},
                {DirectX::XMFLOAT3(-1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
                {DirectX::XMFLOAT3(1.0f, -1.0f,  1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)},
                {DirectX::XMFLOAT3(0.0f,  1.0f,  0.0f), DirectX::XMFLOAT3(0.5f, 0.5f, 1.0f)},
            };
            
            std::vector<std::uint16_t> indices =
            {
                // cube front
                0, 1, 2,
                1, 3, 2,

                // cube back
                7, 5, 6,
                6, 5, 4,

                // cube left
                0, 2, 6,
                4, 0, 6,

                // cube right
                3, 1, 7,
                7, 1, 5,

                // cube top
                1, 0, 4,
                1, 4, 5,

                // cube bottom
                2, 3, 6,
                6, 3, 7,

                // pyramid bottom
                0, 2, 1,
                2, 3, 1,

                // pyramid front
                4, 0, 1,
                // pyramid right
                4, 1, 3,
                // pyramid back
                4, 3, 2,
                // pyramid left
                4, 2, 0,
            };

            {
                // square
                size_t vertexCountSquare;
                size_t indexCountSquare;
                GeometryUtil::calculateVertexIndexCountsSquare(15, vertexCountSquare, indexCountSquare);

                size_t startVertex = vertices.size();
                size_t startIndex = indices.size();
                vertices.resize(vertices.size() + vertexCountSquare);
                indices.resize(indices.size() + indexCountSquare);
                GeometryUtil::createSquare(10.0f, 15, vertices.data() + startVertex, indices.data() + startIndex, vertexDesc);

                {
                    Renderable renderable;
                    renderable.m_cbIndex = 0u;
                    renderable.m_startIndex = static_cast<UINT>(startIndex);
                    renderable.m_baseVertex = static_cast<UINT>(startVertex);
                    renderable.m_indexCount = static_cast<UINT>(indexCountSquare);
                    DirectX::XMStoreFloat4x4(&renderable.m_model, DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
                    m_renderables.emplace_back(renderable);
                }
            }

            constexpr uint8_t objectCount = 7u;
            constexpr float distanceFromCenter = 3.5f;

            {
                // cube
                Renderable renderable;
                renderable.m_cbIndex = 1u;
                renderable.m_startIndex = 0u;
                renderable.m_baseVertex = 0u;
                renderable.m_indexCount = 36u;
                DirectX::XMMATRIX model = DirectX::XMMatrixRotationY(renderable.m_cbIndex * DirectX::g_XMTwoPi[0]/objectCount);
                model = DirectX::XMMatrixMultiply(DirectX::XMMatrixTranslation(0.0f, 1.0f, distanceFromCenter), model);
                DirectX::XMStoreFloat4x4(&renderable.m_model, model);
                m_renderables.emplace_back(renderable);
            }

            {
                // pyramid
                Renderable renderable;
                renderable.m_cbIndex = 2u;
                renderable.m_startIndex = 36u;
                renderable.m_baseVertex = 8u;
                renderable.m_indexCount = 18u;
                DirectX::XMMATRIX model = DirectX::XMMatrixRotationY(renderable.m_cbIndex * DirectX::g_XMTwoPi[0] / objectCount);
                model = DirectX::XMMatrixMultiply(DirectX::XMMatrixTranslation(0.0f, 1.0f, distanceFromCenter), model);
                DirectX::XMStoreFloat4x4(&renderable.m_model, model);
                m_renderables.emplace_back(renderable);
            }

            for (uint8_t i = 0; i < objectCount - 2u; ++i)
            {
                size_t vertexCountGeoSphere;
                size_t indexCountGeoSphere;
                GeometryUtil::calculateVertexIndexCountsGeoSphere(i, vertexCountGeoSphere, indexCountGeoSphere);

                size_t startVertex = vertices.size();
                size_t startIndex = indices.size();
                vertices.resize(vertices.size() + vertexCountGeoSphere);
                indices.resize(indices.size() + indexCountGeoSphere);
                GeometryUtil::createGeoSphere(1.0f, i, vertices.data() + startVertex, indices.data() + startIndex, vertexDesc);

                {
                    Renderable renderable;
                    renderable.m_cbIndex = 3u + i;
                    renderable.m_startIndex = static_cast<UINT>(startIndex);
                    renderable.m_baseVertex = static_cast<UINT>(startVertex);
                    renderable.m_indexCount = static_cast<UINT>(indexCountGeoSphere);
                    DirectX::XMMATRIX model = DirectX::XMMatrixRotationY(renderable.m_cbIndex * DirectX::g_XMTwoPi[0] / objectCount);
                    model = DirectX::XMMatrixMultiply(DirectX::XMMatrixTranslation(0.0f, 1.0f, distanceFromCenter), model);
                    DirectX::XMStoreFloat4x4(&renderable.m_model, model);
                    m_renderables.emplace_back(renderable);
                }
            }

            m_pMesh->createVertexBuffer(vertices.data(), vertices.size(), sizeof(Vertex), m_pCommandList.Get(), 0);
            m_pMesh->createIndexBuffer(indices.data(), indices.size(), sizeof(std::uint16_t), m_pCommandList.Get());
        }
    }
    
    {
        m_cbvDescriptorsPerFrame = 1u + m_renderables.size();

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NumDescriptors = static_cast<UINT>(m_frameResourcesCount * m_cbvDescriptorsPerFrame);
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap)));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    for (size_t i = 0; i < m_frameResourcesCount; ++i)
    {
        m_frameResources.emplace_back(m_pDevice.Get(), 1, m_renderables.size());
    }

    for (const FrameResources& frameResources : m_frameResources)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.SizeInBytes = frameResources.m_pCbObjects->getElementSize();
        desc.BufferLocation = frameResources.m_pCbObjects->getResource()->GetGPUVirtualAddress();

        for (size_t j = 0; j < m_renderables.size(); ++j)
        {
            m_pDevice->CreateConstantBufferView(&desc, handle);
            desc.BufferLocation += desc.SizeInBytes;
            handle.ptr += m_cbvSrvUavDescriptorSize;
        }
    }

    for (const FrameResources& frameResources : m_frameResources)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = frameResources.m_pCbPass->getResource()->GetGPUVirtualAddress();
        desc.SizeInBytes = frameResources.m_pCbPass->getSize();
        m_pDevice->CreateConstantBufferView(&desc, handle);
        handle.ptr += m_cbvSrvUavDescriptorSize;
    }

    {
        D3D12_DESCRIPTOR_RANGE1 objectsDescriptorRange = {};
        objectsDescriptorRange.BaseShaderRegister = 0;
        objectsDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        objectsDescriptorRange.NumDescriptors = 1;
        objectsDescriptorRange.OffsetInDescriptorsFromTableStart = 0;
        objectsDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        objectsDescriptorRange.RegisterSpace = 0;

        D3D12_ROOT_PARAMETER1 rpObjects = {};
        rpObjects.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rpObjects.DescriptorTable.NumDescriptorRanges = 1;
        rpObjects.DescriptorTable.pDescriptorRanges = &objectsDescriptorRange;

        D3D12_DESCRIPTOR_RANGE1 passDescriptorRange = {};
        passDescriptorRange.BaseShaderRegister = 1;
        passDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        passDescriptorRange.NumDescriptors = 1;
        passDescriptorRange.OffsetInDescriptorsFromTableStart = 0;
        passDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        passDescriptorRange.RegisterSpace = 0;

        D3D12_ROOT_PARAMETER1 rpPass = {};
        rpPass.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rpPass.DescriptorTable.NumDescriptorRanges = 1;
        rpPass.DescriptorTable.pDescriptorRanges = &passDescriptorRange;

        D3D12_ROOT_PARAMETER1 parameters[] = { rpObjects, rpPass,  };

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc.Desc_1_1.NumParameters = 2;
        desc.Desc_1_1.pParameters = parameters;

        Microsoft::WRL::ComPtr<ID3DBlob> pBlob, pErrorBlob;
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&desc, &pBlob, &pErrorBlob));
        ThrowIfFailed(m_pDevice->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)));
    }

    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.BlendState.AlphaToCoverageEnable = false;
        desc.BlendState.IndependentBlendEnable = false;
        desc.BlendState.RenderTarget[0].BlendEnable = false;
        desc.BlendState.RenderTarget[0].LogicOpEnable = false;
        desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_DEPTH_STENCILOP_DESC defaultDepthStencilOpDesc = {};
        defaultDepthStencilOpDesc.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        defaultDepthStencilOpDesc.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        defaultDepthStencilOpDesc.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        defaultDepthStencilOpDesc.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.DepthStencilState.BackFace = defaultDepthStencilOpDesc;
        desc.DepthStencilState.DepthEnable = true;
        desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthStencilState.FrontFace = defaultDepthStencilOpDesc;
        desc.DepthStencilState.StencilEnable = false;
        desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

        desc.RasterizerState.AntialiasedLineEnable = false;
        desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.RasterizerState.DepthClipEnable = false;
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.ForcedSampleCount = 0;
        desc.RasterizerState.FrontCounterClockwise = false;
        desc.RasterizerState.MultisampleEnable = false;
        desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

        D3D12_INPUT_ELEMENT_DESC positionInputElementDesc = {};
        positionInputElementDesc.AlignedByteOffset = 0;
        positionInputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        positionInputElementDesc.InputSlot = 0;
        positionInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        positionInputElementDesc.InstanceDataStepRate = 0;
        positionInputElementDesc.SemanticIndex = 0;
        positionInputElementDesc.SemanticName = "POSITION";

        D3D12_INPUT_ELEMENT_DESC colorInputElementDesc = {};
        colorInputElementDesc.AlignedByteOffset = 12;
        colorInputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        colorInputElementDesc.InputSlot = 0;
        colorInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        colorInputElementDesc.InstanceDataStepRate = 0;
        colorInputElementDesc.SemanticIndex = 0;
        colorInputElementDesc.SemanticName = "COLOR";

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = { positionInputElementDesc, colorInputElementDesc };

        desc.InputLayout.NumElements = sizeof(inputElementDescs) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        desc.InputLayout.pInputElementDescs = inputElementDescs;

        desc.NodeMask = 0;
        desc.NumRenderTargets = 1;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.pRootSignature = m_pRootSignature.Get();

        Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader = D3D12Util::compileShader(L"data/shaders/chapter07/shapesDemo.hlsl", "ps", "ps_5_1");
        desc.PS.pShaderBytecode = pPixelShader->GetBufferPointer();
        desc.PS.BytecodeLength = pPixelShader->GetBufferSize();

        Microsoft::WRL::ComPtr<ID3D10Blob> pVertexShader = D3D12Util::compileShader(L"data/shaders/chapter07/shapesDemo.hlsl", "vs", "vs_5_1");
        desc.VS.pShaderBytecode = pVertexShader->GetBufferPointer();
        desc.VS.BytecodeLength = pVertexShader->GetBufferSize();

        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleMask = UINT_MAX;

        ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pPipelineState)));
    }

    ThrowIfFailed(m_pCommandList->Close());

    {
        ID3D12CommandList* const commandLists[] = { m_pCommandList.Get() };
        m_pCommandQueue->ExecuteCommandLists(1, commandLists);
    }

    flushCommandQueue();
}

void ShapesDemo::update(float dt)
{
    m_curFrameResourcesIndex = (m_curFrameResourcesIndex + 1u) % m_frameResourcesCount;
    FrameResources& curFrameResources = m_frameResources[m_curFrameResourcesIndex];
    if (curFrameResources.m_fenceValue != 0 && curFrameResources.m_fenceValue > m_pFence->GetCompletedValue())
    {
        HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_pFence->SetEventOnCompletion(curFrameResources.m_fenceValue, hEvent));
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    {
        PassConstants passConstants;
        m_camera.update();
        passConstants.view = m_camera.m_matrix;
        DirectX::XMStoreFloat4x4(&passConstants.projection, DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(60.0f), static_cast<float>(m_windowWidth) / m_windowHeight, 0.1f, 100.0f));
        passConstants.time = m_timer.getElapsedTime();
        passConstants.deltaTime = dt;
        curFrameResources.m_pCbPass->copyData(static_cast<void*>(&passConstants), sizeof(PassConstants));
    }

    size_t elementSize = curFrameResources.m_pCbObjects->getElementSize();
    for (const Renderable& renderable : m_renderables)
    {
        ObjectConstants objectConstants;
        objectConstants.model = renderable.m_model;
        curFrameResources.m_pCbObjects->copyData(static_cast<void*>(&objectConstants), elementSize, renderable.m_cbIndex * elementSize);
    }
}
void ShapesDemo::render()
{
    FrameResources& curFrameResources = m_frameResources[m_curFrameResourcesIndex];
    curFrameResources.m_fenceValue = ++m_curFrameFenceValue;

    ThrowIfFailed(curFrameResources.m_pCommandAllocator->Reset());
    ThrowIfFailed(m_pCommandList->Reset(curFrameResources.m_pCommandAllocator.Get(), m_pPipelineState.Get()));

    {
        D3D12_RESOURCE_BARRIER presetToRenderTargetTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_pCommandList->ResourceBarrier(1, &presetToRenderTargetTransition);
    }

    float clearColor[] = { 0.2f, 0.4f, 0.6f, 1.0f };
    m_pCommandList->ClearRenderTargetView(getCurrentBackBufferView(), clearColor, 0, nullptr);
    m_pCommandList->ClearDepthStencilView(getCurrentDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

    ID3D12DescriptorHeap* const descriptorHeaps[] = { m_pDescriptorHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(sizeof(descriptorHeaps) / sizeof(ID3D12DescriptorHeap*), descriptorHeaps);

    const D3D12_INDEX_BUFFER_VIEW indexBufferView = m_pMesh->getIndexBufferView();
    m_pCommandList->IASetIndexBuffer(&indexBufferView);

    const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = m_pMesh->getVertexBufferView();
    m_pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandlePassCb = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandlePassCb.ptr += m_frameResourcesCount * m_renderables.size() * m_cbvSrvUavDescriptorSize;
    gpuHandlePassCb.ptr += m_curFrameResourcesIndex * m_cbvSrvUavDescriptorSize;;

    m_pCommandList->SetGraphicsRootDescriptorTable(1, gpuHandlePassCb);

    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = getCurrentBackBufferView();
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = getCurrentDepthStencilView();
        m_pCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
    }

    {
        D3D12_VIEWPORT viewport = {};
        viewport.Height = static_cast<float>(m_windowHeight);
        viewport.Width = static_cast<float>(m_windowWidth);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_pCommandList->RSSetViewports(1, &viewport);
    }

    {
        D3D12_RECT rect = {};
        rect.bottom = m_windowHeight;
        rect.right = m_windowWidth;
        m_pCommandList->RSSetScissorRects(1, &rect);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleObjectsCb = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandleObjectsCb.ptr += m_curFrameResourcesIndex * m_renderables.size() * m_cbvSrvUavDescriptorSize;
    for (const Renderable& renderable : m_renderables)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = gpuHandleObjectsCb;
        gpuHandle.ptr += renderable.m_cbIndex * m_cbvSrvUavDescriptorSize;
        m_pCommandList->SetGraphicsRootDescriptorTable(0, gpuHandle);

        m_pCommandList->IASetPrimitiveTopology(renderable.m_topology);

        m_pCommandList->DrawIndexedInstanced(renderable.m_indexCount, 1, renderable.m_startIndex, renderable.m_baseVertex, 0);
    }

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

    m_currenBackBufferId = (m_currenBackBufferId + 1) % m_swapChainBufferCount;

    ThrowIfFailed(m_pCommandQueue->Signal(m_pFence.Get(), curFrameResources.m_fenceValue));
}

void ShapesDemo::onMouseDown(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    SetCapture(m_hWnd);
}

void ShapesDemo::onMouseUp(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    ReleaseCapture();
}

void ShapesDemo::onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons)
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
