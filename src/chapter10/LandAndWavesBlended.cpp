#include "LandAndWavesBlended.h"

#include <iostream>

#include "DebugUtil.h"
#include "GeometryUtil.h"

LandAndWavesBlended::~LandAndWavesBlended()
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

void LandAndWavesBlended::onMouseDown(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    SetCapture(m_hWnd);
}

void LandAndWavesBlended::onMouseUp(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    ReleaseCapture();
}

void LandAndWavesBlended::onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons)
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

void LandAndWavesBlended::initialize()
{
    ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

    ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFrameFence)));

    size_t landVertexCount;
    {
        size_t landIndexCount;
        GeometryUtil::calculateVertexIndexCountsSquare(VERTICES_PER_SIDE, landVertexCount, landIndexCount);

        std::unique_ptr<Vertex[]> p_landVertices = std::make_unique<Vertex[]>(landVertexCount);
        std::unique_ptr<uint16_t[]> p_landIndices = std::make_unique<uint16_t[]>(landIndexCount);
        GeometryUtil::createSquare(m_gridWidth, VERTICES_PER_SIDE, p_landVertices.get(), p_landIndices.get());

        for (uint16_t y = 0; y < VERTICES_PER_SIDE; ++y)
        {
            for (uint16_t x = 0; x < VERTICES_PER_SIDE; ++x)
            {
                Vertex& vertex = p_landVertices[y * VERTICES_PER_SIDE + x];
                float sinX = DirectX::XMScalarSinEst(vertex.pos.x * 16.0f / (m_gridWidth));
                float cosZ = DirectX::XMScalarCosEst(vertex.pos.z * 16.0f / (m_gridWidth));
                vertex.pos.y = (6.0f*(vertex.pos.z* sinX + vertex.pos.x*cosZ) + 0.5f) / m_gridWidth;

                float sinXdx = (16.0f / (m_gridWidth)) * DirectX::XMScalarCosEst(vertex.pos.x * 16.0f / (m_gridWidth));
                float cosZdz = -(16.0f / (m_gridWidth)) * DirectX::XMScalarSinEst(vertex.pos.z * 16.0f / (m_gridWidth));
                DirectX::XMVECTOR normal = {
                    -(6.0f * vertex.pos.z * sinXdx + cosZ) / m_gridWidth,
                    1.0f,
                    -(6.0f * vertex.pos.x * cosZdz + sinX) / m_gridWidth,
                };
                DirectX::XMStoreFloat3(&vertex.normal, DirectX::XMVector3Normalize(normal));
            }
        }

        // not thread safe
        size_t meshIndex = m_meshes.size();
        Mesh landMesh;
        landMesh.createVertexBuffer(p_landVertices.get(), landVertexCount, sizeof(Vertex), m_pCommandList.Get());
        landMesh.createIndexBuffer(p_landIndices.get(), landIndexCount, sizeof(uint16_t), m_pCommandList.Get());
        m_meshes.emplace_back(landMesh);

        // not thread safe
        size_t textureIndex = m_textures.size();
        m_textures.emplace_back().createFromFileAndUpload(m_pCommandList.Get(), L"data/textures/brown_mud_leaves_01_diff_1k_bc1.dds");

        // not thread safe
        Material landMaterial;
        size_t materialIndex = m_materials.size();
        landMaterial.m_framesDirtyCount = FRAME_RESOURCES_COUNT;
        landMaterial.m_cbIndex = 0u;
        landMaterial.m_albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        landMaterial.m_roughness = 0.9f;
        landMaterial.texCoordTransformColumn0 = { 10.0f, 0.0f };
        landMaterial.texCoordTransformColumn1 = { 0.0f, 10.0f };
        landMaterial.m_diffuseTextureIndex = textureIndex;
        m_materials.emplace_back(landMaterial);

        Renderable landRenderable;
        landRenderable.m_meshIndex = meshIndex;
        landRenderable.m_materialIndex = materialIndex;
        landRenderable.m_cbIndex = 0;
        landRenderable.m_startIndex = 0;
        landRenderable.m_baseVertex = 0;
        landRenderable.m_indexCount = static_cast<UINT>(landIndexCount);
        m_opaqueRenderables.emplace_back(landRenderable);
    }

    {
        size_t wavesVertexCount;
        size_t wavesIndexCount;
        GeometryUtil::calculateVertexIndexCountsSquare(VERTICES_PER_SIDE, wavesVertexCount, wavesIndexCount);

        std::unique_ptr<uint16_t[]> pIndices = std::make_unique<uint16_t[]>(wavesIndexCount);
        GeometryUtil::createSquare(m_gridWidth, VERTICES_PER_SIDE, m_wavesVertices, pIndices.get());

        // not thread safe
        size_t meshIndex = m_meshes.size();
        Mesh wavesMesh;
        wavesMesh.m_indexCount = wavesIndexCount;
        wavesMesh.m_indexSize = sizeof(uint16_t);
        wavesMesh.m_vertexCount = wavesVertexCount;
        wavesMesh.m_vertexSize[0] = sizeof(Vertex);

        D3D12Util::createAndUploadBuffer(pIndices.get(), wavesMesh.m_indexCount * wavesMesh.m_indexSize, m_pCommandList.Get(), &wavesMesh.m_pIndexBuffer, &wavesMesh.m_pIndexBufferUpload);
        m_meshes.emplace_back(wavesMesh);

        // not thread safe
        size_t textureIndex = m_textures.size();
        m_textures.emplace_back().createFromFileAndUpload(m_pCommandList.Get(), L"data/textures/Water_001_COLOR_bc1.dds");

        // not thread safe
        size_t materialIndex = m_materials.size();
        Material waterMaterial;
        waterMaterial.m_framesDirtyCount = FRAME_RESOURCES_COUNT;
        waterMaterial.m_cbIndex = 1u;
        waterMaterial.m_fresnelR0 = { 0.1f, 0.1f, 0.1f };
        waterMaterial.m_albedoColor = { 1.0f, 1.0f, 1.0f, 0.5f };
        waterMaterial.m_roughness = 0.0f;
        waterMaterial.texCoordTransformColumn0 = { 20.0f, 0.0f };
        waterMaterial.texCoordTransformColumn1 = { 0.0f, 20.0f };
        waterMaterial.m_diffuseTextureIndex = textureIndex;
        m_materials.emplace_back(waterMaterial);
        
        m_waveRenderableIndex = m_transparentRenderables.size();
        Renderable wavesRenderable;
        wavesRenderable.m_model._42 = -0.25f;
        wavesRenderable.m_meshIndex = meshIndex;
        wavesRenderable.m_materialIndex = materialIndex;
        wavesRenderable.m_cbIndex = 1;
        wavesRenderable.m_startIndex = 0;
        wavesRenderable.m_baseVertex = 0;
        wavesRenderable.m_indexCount = static_cast<UINT>(wavesIndexCount);
        m_transparentRenderables.emplace_back(wavesRenderable);
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC  desc = {};
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NumDescriptors = static_cast<UINT>(m_textures.size());
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pSrvHeap)));
    }

    for (size_t srvIndex = 0; srvIndex < m_textures.size(); ++srvIndex)
    {
        D3D12_RESOURCE_DESC resourceDesc = m_textures[srvIndex].m_pResource->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = resourceDesc.Format;
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = resourceDesc.MipLevels;
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.PlaneSlice = 0;
        desc.Texture2D.ResourceMinLODClamp = 0.0f;

        auto cpuHandle = m_pSrvHeap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += srvIndex * m_cbvSrvUavDescriptorSize;

        m_pDevice->CreateShaderResourceView(m_textures[srvIndex].m_pResource.Get(), &desc, cpuHandle);

        m_textures[srvIndex].m_srvHeapIndex = srvIndex;
    }

    for (FrameResources& frameResources : m_frameResources)
    {
        size_t totalRenderableCount = m_opaqueRenderables.size() + m_transparentRenderables.size();
        ThrowIfFailed(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameResources.m_pCommandAllocator)));
        frameResources.m_pCbPass = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), 1, sizeof(PassConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pCbObjects = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), totalRenderableCount, sizeof(ObjectConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pCbMaterials = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), totalRenderableCount, sizeof(MaterialConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pDynamicVertices = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), landVertexCount, sizeof(Vertex));
    }

    {
        D3D12_ROOT_PARAMETER1 materialCbParameter = {};
        materialCbParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        materialCbParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        materialCbParameter.Descriptor.ShaderRegister = 0;

        D3D12_ROOT_PARAMETER1 objectCbParameter = {};
        objectCbParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        objectCbParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        objectCbParameter.Descriptor.ShaderRegister = 1;

        D3D12_ROOT_PARAMETER1 passCbParameter = {};
        passCbParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        passCbParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        passCbParameter.Descriptor.ShaderRegister = 2;

        D3D12_DESCRIPTOR_RANGE1 textureDescriptorRange = {};
        textureDescriptorRange.BaseShaderRegister = 0;
        textureDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
        textureDescriptorRange.NumDescriptors = 1;
        textureDescriptorRange.OffsetInDescriptorsFromTableStart = 0;
        textureDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureDescriptorRange.RegisterSpace = 0u;

        D3D12_ROOT_PARAMETER1 textureSrvParameter = {};
        textureSrvParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        textureSrvParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        textureSrvParameter.DescriptorTable.NumDescriptorRanges = 1;
        textureSrvParameter.DescriptorTable.pDescriptorRanges = &textureDescriptorRange;

        D3D12_ROOT_PARAMETER1 rootParameters[] = {
            materialCbParameter,
            objectCbParameter,
            passCbParameter,
            textureSrvParameter,
        };

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rootSignatureDesc.Desc_1_1.NumParameters = sizeof(rootParameters) / sizeof(D3D12_ROOT_PARAMETER1);
        rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
        std::array<D3D12_STATIC_SAMPLER_DESC, m_staticSamplerCount> staticSamplers = createDefaultStaticSamplerDescs();
        rootSignatureDesc.Desc_1_1.NumStaticSamplers = m_staticSamplerCount;
        rootSignatureDesc.Desc_1_1.pStaticSamplers = staticSamplers.data();

        Microsoft::WRL::ComPtr<ID3DBlob> pRootSignatureBlob, pRootSignatureErrorBlob;
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &pRootSignatureBlob, &pRootSignatureErrorBlob));
        ThrowIfFailed(m_pDevice->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)));
    }

    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.BlendState.AlphaToCoverageEnable = false;
        desc.BlendState.IndependentBlendEnable = false;
        desc.BlendState.RenderTarget[0].BlendEnable = true;
        desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
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

        D3D12_INPUT_ELEMENT_DESC texCoordInputElementDesc = {};
        texCoordInputElementDesc.AlignedByteOffset = 12u;
        texCoordInputElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        texCoordInputElementDesc.InputSlot = 0u;
        texCoordInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        texCoordInputElementDesc.InstanceDataStepRate = 0u;
        texCoordInputElementDesc.SemanticIndex = 0u;
        texCoordInputElementDesc.SemanticName = "TEXCOORD";

        D3D12_INPUT_ELEMENT_DESC normalInputElementDesc = {};
        normalInputElementDesc.AlignedByteOffset = 20u;
        normalInputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        normalInputElementDesc.InputSlot = 0u;
        normalInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        normalInputElementDesc.InstanceDataStepRate = 0u;
        normalInputElementDesc.SemanticIndex = 0u;
        normalInputElementDesc.SemanticName = "NORMAL";

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            vertexInputElementDesc,
            texCoordInputElementDesc,
            normalInputElementDesc,
        };

        desc.InputLayout.NumElements = sizeof(inputElementDescs)/sizeof(D3D12_INPUT_ELEMENT_DESC);
        desc.InputLayout.pInputElementDescs = inputElementDescs;

        desc.pRootSignature = m_pRootSignature.Get();

        Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader = D3D12Util::compileShader(L"data/shaders/chapter10/landAndWavesBlended.hlsl", "vs", "vs_5_1");
        desc.VS.pShaderBytecode = pVertexShader->GetBufferPointer();
        desc.VS.BytecodeLength = pVertexShader->GetBufferSize();

        Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader = D3D12Util::compileShader(L"data/shaders/chapter10/landAndWavesBlended.hlsl", "ps", "ps_5_1");
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

void LandAndWavesBlended::update(float dt)
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
        constexpr float scale = 0.005f;
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
                m_wavesVertices[y][x].pos.y = scale * offset;

                const uint16_t xPos = x < VERTICES_PER_SIDE - 1 ? x + 1 : x;
                const uint16_t xNeg = x > 0 ? x - 1 : x;
                float xStep = m_gridWidth / VERTICES_PER_SIDE;
                if (x <= 0 || x >= VERTICES_PER_SIDE - 1)
                {
                    xStep *= 0.5f;
                }

                const uint16_t yPos = y < VERTICES_PER_SIDE - 1 ? y + 1 : y;
                const uint16_t yNeg = y > 0 ? y - 1 : y;
                float yStep = m_gridWidth / VERTICES_PER_SIDE;
                if (y <= 0 || y >= VERTICES_PER_SIDE - 1)
                {
                    yStep *= 0.5f;
                }

                DirectX::XMVECTOR normal = {
                    (m_wavesVertices[y][xPos].pos.y - m_wavesVertices[y][xNeg].pos.y) / xStep,
                    1.0f,
                    (m_wavesVertices[yPos][x].pos.y - m_wavesVertices[yNeg][x].pos.y) / yStep,
                };
                DirectX::XMStoreFloat3(&m_wavesVertices[y][x].normal, DirectX::XMVector3Normalize(normal));
            }
        }

        const auto& waveRenderable = m_transparentRenderables[m_waveRenderableIndex];
        auto& waveMesh = m_meshes[waveRenderable.m_meshIndex];
        curFrameResources.m_pDynamicVertices->copyData(m_wavesVertices, waveMesh.m_vertexCount * waveMesh.m_vertexSize[0]);
        waveMesh.m_pVertexBuffer[0] = curFrameResources.m_pDynamicVertices->getResourceComPtr();
        auto& waveMaterial = m_materials[waveRenderable.m_materialIndex];
        waveMaterial.texCoordOffset.x += dt * 0.02f;
        if (waveMaterial.texCoordOffset.x >= 1.0f) {
            waveMaterial.texCoordOffset.x -= 1.0f;
        }
        waveMaterial.texCoordOffset.y += dt * 0.05f;
        if (waveMaterial.texCoordOffset.y >= 1.0f) {
            waveMaterial.texCoordOffset.y -= 1.0f;
        }
        waveMaterial.m_framesDirtyCount = FRAME_RESOURCES_COUNT;
    }

    {
        PassConstants passConstants = {};
        passConstants.view = m_camera.m_matrix;
        DirectX::XMStoreFloat4x4(&passConstants.projection, DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(60.0f), static_cast<float>(m_windowWidth) / m_windowHeight, 0.1f, 100.0f));
        passConstants.time = m_timer.getElapsedTime();
        passConstants.dTime = dt;
        passConstants.ambientLight = { 0.05f, 0.05f, 0.05f };
        passConstants.cameraPositionW = m_camera.m_position;

        passConstants.lightData[0].color = { 1.0f, 1.0f, 1.0f };
        passConstants.lightData[0].direction = { 2.0f, -5.0f, 3.0f };
        passConstants.directionalLightCount = 1u;

        passConstants.lightData[1].color = { 1.0f, 1.0f, 1.0f };
        passConstants.lightData[1].position = { 0.0f, 5.0f, 0.0f };
        passConstants.lightData[1].falloffBegin = 1.0f;
        passConstants.lightData[1].falloffEnd = 10.0f;
        passConstants.pointLightCount = 1u;

        passConstants.lightData[2].color = { 1.0f, 1.0f, 1.0f };
        passConstants.lightData[2].position = { 0.0f, 3.0f, 0.0f };
        passConstants.lightData[2].direction = { -3.0f, -3.0f, -1.0f };
        passConstants.lightData[2].falloffBegin = 1.0f;
        passConstants.lightData[2].falloffEnd = 15.0f;
        passConstants.lightData[2].spotPower = 8.0f;
        passConstants.spotLightCount = 1u;

        curFrameResources.m_pCbPass->copyData(&passConstants, sizeof(PassConstants));
    }

    auto updateObjectCbContents = [&curFrameResources](const Renderable& renderable)
    {
        ObjectConstants objectConstants{};
        objectConstants.world = renderable.m_model;
        objectConstants.texCoordTransformColumn0 = { 1.0f, 0.0f };
        objectConstants.texCoordTransformColumn1 = { 0.0f, 1.0f };
        objectConstants.texCoordOffset = { 0.0f, 0.0f };
        curFrameResources.m_pCbObjects->copyData(&objectConstants, sizeof(ObjectConstants), renderable.m_cbIndex * curFrameResources.m_pCbObjects->getElementSize());
    };

    for (const auto& renderable : m_opaqueRenderables)
    {
        updateObjectCbContents(renderable);
    }

    for (const auto& renderable : m_transparentRenderables)
    {
        updateObjectCbContents(renderable);
    }

    for (auto& material : m_materials)
    {
        if (material.m_framesDirtyCount > 0)
        {
            MaterialConstants materialConstants{};
            materialConstants.albedoColor = material.m_albedoColor;
            materialConstants.fresnelR0 = material.m_fresnelR0;
            materialConstants.roughness = material.m_roughness;
            materialConstants.texCoordTransformColumn0 = material.texCoordTransformColumn0;
            materialConstants.texCoordTransformColumn1 = material.texCoordTransformColumn1;
            materialConstants.texCoordOffset = material.texCoordOffset;

            curFrameResources.m_pCbMaterials->copyData(&materialConstants, sizeof(MaterialConstants), material.m_cbIndex * curFrameResources.m_pCbMaterials->getElementSize());

            --material.m_framesDirtyCount;
        }
    }
};

void LandAndWavesBlended::render()
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
    m_pCommandList->SetGraphicsRootConstantBufferView(2, curFrameResources.m_pCbPass->getResource()->GetGPUVirtualAddress());
    ID3D12DescriptorHeap* const heaps[] = { m_pSrvHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(1u, heaps);

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

    auto renderRenderable = [&](const Renderable & renderable)
    {
        D3D12_GPU_VIRTUAL_ADDRESS materialCbGpuAddress = curFrameResources.m_pCbMaterials->getResource()->GetGPUVirtualAddress();
        materialCbGpuAddress += m_materials[renderable.m_materialIndex].m_cbIndex * curFrameResources.m_pCbMaterials->getElementSize();
        m_pCommandList->SetGraphicsRootConstantBufferView(0, materialCbGpuAddress);

        D3D12_GPU_VIRTUAL_ADDRESS objectCbGpuAddress = curFrameResources.m_pCbObjects->getResource()->GetGPUVirtualAddress();
        objectCbGpuAddress += renderable.m_cbIndex * curFrameResources.m_pCbObjects->getElementSize();
        m_pCommandList->SetGraphicsRootConstantBufferView(1, objectCbGpuAddress);

        D3D12_GPU_DESCRIPTOR_HANDLE textureSrvGpuHandle = m_pSrvHeap->GetGPUDescriptorHandleForHeapStart();
        textureSrvGpuHandle.ptr += m_textures[m_materials[renderable.m_materialIndex].m_diffuseTextureIndex].m_srvHeapIndex * m_cbvSrvUavDescriptorSize;
        m_pCommandList->SetGraphicsRootDescriptorTable(3, textureSrvGpuHandle);

        const D3D12_INDEX_BUFFER_VIEW indexBufferView = m_meshes[renderable.m_meshIndex].getIndexBufferView();
        m_pCommandList->IASetIndexBuffer(&indexBufferView);

        const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = m_meshes[renderable.m_meshIndex].getVertexBufferView();
        m_pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        m_pCommandList->IASetPrimitiveTopology(renderable.m_topology);

        m_pCommandList->DrawIndexedInstanced(static_cast<UINT>(m_meshes[renderable.m_meshIndex].m_indexCount), 1u, 0u, 0u, 0u);
    };

    for (const auto& renderable : m_opaqueRenderables)
    {
        renderRenderable(renderable);
    }

    for (const auto& renderable : m_transparentRenderables)
    {
        renderRenderable(renderable);
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
