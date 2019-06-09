#include "Mirror.h"

#include <iostream>

#include "DebugUtil.h"
#include "GeometryUtil.h"

Mirror::Mirror(HINSTANCE hInst) : AppBase(hInst, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
{
}

Mirror::~Mirror()
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

void Mirror::onMouseDown(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    SetCapture(m_hWnd);
}

void Mirror::onMouseUp(int16_t xPos, int16_t yPos, uint8_t /*buttons*/)
{
    m_curMouseX = m_lastMouseX = xPos;
    m_curMouseY = m_lastMouseY = yPos;
    ReleaseCapture();
}

void Mirror::onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons)
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

void Mirror::initialize()
{
    ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr));

    ThrowIfFailed(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFrameFence)));

    UINT currentRenderableCbIndex = 0u;
    UINT currentMaterialCbIndex = 0u;

    {
        size_t wallVertexCount;
        size_t wallIndexCount;
        GeometryUtil::calculateVertexIndexCountsSquare(VERTICES_PER_SIDE, wallVertexCount, wallIndexCount);

        std::unique_ptr<Vertex[]> pLandVertices = std::make_unique<Vertex[]>(wallVertexCount);
        std::unique_ptr<uint16_t[]> pLandIndices = std::make_unique<uint16_t[]>(wallIndexCount);
        GeometryUtil::createSquare(m_gridWidth, VERTICES_PER_SIDE, pLandVertices.get(), pLandIndices.get());

        // not thread safe
        size_t meshIndex = m_meshes.size();
        Mesh squareMesh;
        squareMesh.createVertexBuffer(pLandVertices.get(), wallVertexCount, sizeof(Vertex), m_pCommandList.Get());
        squareMesh.createIndexBuffer(pLandIndices.get(), wallIndexCount, sizeof(uint16_t), m_pCommandList.Get());
        m_meshes.emplace_back(squareMesh);

        // not thread safe
        size_t textureIndex = m_textures.size();
        m_textures.emplace_back().createFromFileAndUpload(m_pCommandList.Get(), L"data/textures/brown_mud_leaves_01_diff_1k_bc1.dds");

        {
            // not thread safe
            Material wallMaterial;
            size_t materialIndex = m_materials.size();
            wallMaterial.m_framesDirtyCount = FRAME_RESOURCES_COUNT;
            wallMaterial.m_cbIndex = currentMaterialCbIndex++;
            wallMaterial.m_albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            wallMaterial.m_roughness = 0.9f;
            wallMaterial.texCoordTransformColumn0 = { 10.0f, 0.0f };
            wallMaterial.texCoordTransformColumn1 = { 0.0f, 10.0f };
            wallMaterial.m_diffuseTextureIndex = textureIndex;
            m_materials.emplace_back(wallMaterial);

            Renderable wallRenderable0;
            wallRenderable0.m_meshIndex = meshIndex;
            wallRenderable0.m_materialIndex = materialIndex;
            wallRenderable0.m_cbIndex = currentRenderableCbIndex++;
            wallRenderable0.m_startIndex = 0;
            wallRenderable0.m_baseVertex = 0;
            wallRenderable0.m_indexCount = static_cast<UINT>(wallIndexCount);
            wallRenderable0.m_model._42 = -m_gridWidth / 2.0f;
            m_sceneRenderables.emplace_back(wallRenderable0);

            Renderable wallRenderable1 = wallRenderable0;
            wallRenderable1.m_cbIndex = currentRenderableCbIndex++;
            wallRenderable1.m_model = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
            wallRenderable1.m_model._43 = -m_gridWidth / 2.0f;
            m_sceneRenderables.emplace_back(wallRenderable1);

            Renderable wallRenderable2 = wallRenderable1;
            wallRenderable2.m_cbIndex = currentRenderableCbIndex++;
            wallRenderable2.m_model = {
                0.0f, 0.0f, -1.0f, 0.0f,
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
            wallRenderable2.m_model._41 = -m_gridWidth / 2.0f;
            m_sceneRenderables.emplace_back(wallRenderable2);

            Renderable wallRenderable3 = wallRenderable1;
            wallRenderable3.m_cbIndex = currentRenderableCbIndex++;
            wallRenderable3.m_model = {
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
            wallRenderable3.m_model._43 = m_gridWidth / 2.0f;
            m_sceneRenderables.emplace_back(wallRenderable3);

            Renderable wallRenderable4 = wallRenderable1;
            wallRenderable4.m_cbIndex = currentRenderableCbIndex++;
            wallRenderable4.m_model = {
                0.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
            wallRenderable4.m_model._41 = m_gridWidth / 2.0f;
            wallRenderable4.m_model._42 = -m_gridWidth / 2.0f;
            m_sceneRenderables.emplace_back(wallRenderable4);

            Renderable wallRenderable5 = wallRenderable0;
            wallRenderable5.m_cbIndex = currentRenderableCbIndex++;
            wallRenderable5.m_model._22 = -1.0f;
            wallRenderable5.m_model._33 = -1.0f;
            wallRenderable5.m_model._42 = m_gridWidth / 2.0f;
            m_sceneRenderables.emplace_back(wallRenderable5);
        }

        {
            // not thread safe
            Material mirrorMaterial;
            size_t materialIndex = m_materials.size();
            mirrorMaterial.m_framesDirtyCount = FRAME_RESOURCES_COUNT;
            mirrorMaterial.m_cbIndex = currentMaterialCbIndex++;
            mirrorMaterial.m_albedoColor = { 0.8f, 0.8f, 1.0f, 0.3f };
            mirrorMaterial.m_roughness = 0.9f;
            mirrorMaterial.texCoordTransformColumn0 = { 1.0f, 0.0f };
            mirrorMaterial.texCoordTransformColumn1 = { 0.0f, 1.0f };
            mirrorMaterial.m_diffuseTextureIndex = textureIndex;
            m_materials.emplace_back(mirrorMaterial);

            Renderable mirrorRenderable;
            mirrorRenderable.m_meshIndex = meshIndex;
            mirrorRenderable.m_materialIndex = materialIndex;
            mirrorRenderable.m_cbIndex = currentRenderableCbIndex++;
            mirrorRenderable.m_startIndex = 0;
            mirrorRenderable.m_baseVertex = 0;
            mirrorRenderable.m_indexCount = static_cast<UINT>(wallIndexCount);
            mirrorRenderable.m_model = {
                0.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
            };
            mirrorRenderable.m_model._41 = m_gridWidth / 2.0f;
            mirrorRenderable.m_model._42 = m_gridWidth / 2.0f;
            m_mirrorRenderables.emplace_back(mirrorRenderable);

            // mirror normal is y-axis of mirrors coordinate frame
            m_mirrorPlane = { mirrorRenderable.m_model._21, mirrorRenderable.m_model._22, mirrorRenderable.m_model._23, mirrorRenderable.m_model._42 };
        }
    }

    {
        size_t sphereVertexCount;
        size_t sphereIndexCount;
        const uint8_t sphereSubdivisions = 3u;
        GeometryUtil::calculateVertexIndexCountsGeoSphere(sphereSubdivisions, sphereVertexCount, sphereIndexCount);

        std::unique_ptr<uint16_t[]> pIndices = std::make_unique<uint16_t[]>(sphereIndexCount);
        std::unique_ptr<Vertex[]> pVertices = std::make_unique<Vertex[]>(sphereVertexCount);
        GeometryUtil::createGeoSphere(2.0f, sphereSubdivisions, pVertices.get(), pIndices.get());

        // not thread safe
        size_t meshIndex = m_meshes.size();
        Mesh sphereMesh;
        sphereMesh.createVertexBuffer(pVertices.get(), sphereVertexCount, sizeof(Vertex), m_pCommandList.Get());
        sphereMesh.createIndexBuffer(pIndices.get(), sphereIndexCount, sizeof(uint16_t), m_pCommandList.Get());
        m_meshes.emplace_back(sphereMesh);

        // not thread safe
        size_t textureIndex = m_textures.size();
        m_textures.emplace_back().createFromFileAndUpload(m_pCommandList.Get(), L"data/textures/MetalWalkway04_col_bc3.dds");

        // not thread safe
        size_t materialIndex = m_materials.size();
        Material metalGridMaterial;
        metalGridMaterial.m_framesDirtyCount = FRAME_RESOURCES_COUNT;
        metalGridMaterial.m_cbIndex = currentMaterialCbIndex++;
        metalGridMaterial.m_fresnelR0 = { 0.1f, 0.1f, 0.1f };
        metalGridMaterial.m_albedoColor = { 1.0f, 1.0f, 1.0f, 0.5f };
        metalGridMaterial.m_roughness = 0.0f;
        metalGridMaterial.texCoordTransformColumn0 = { 3.0f, 0.0f };
        metalGridMaterial.texCoordTransformColumn1 = { 0.0f, 3.0f };
        metalGridMaterial.m_diffuseTextureIndex = textureIndex;
        m_materials.emplace_back(metalGridMaterial);

        Renderable metalGridSphereRenderable;
        metalGridSphereRenderable.m_meshIndex = meshIndex;
        metalGridSphereRenderable.m_materialIndex = materialIndex;
        metalGridSphereRenderable.m_cbIndex = currentRenderableCbIndex++;
        metalGridSphereRenderable.m_startIndex = 0;
        metalGridSphereRenderable.m_baseVertex = 0;
        metalGridSphereRenderable.m_indexCount = static_cast<UINT>(sphereIndexCount);
        m_sceneRenderables.emplace_back(metalGridSphereRenderable);

        Renderable mirroredMetalGridSphereRenderable;
        mirroredMetalGridSphereRenderable.m_meshIndex = meshIndex;
        mirroredMetalGridSphereRenderable.m_materialIndex = materialIndex;
        mirroredMetalGridSphereRenderable.m_cbIndex = currentRenderableCbIndex++;
        mirroredMetalGridSphereRenderable.m_startIndex = 0;
        mirroredMetalGridSphereRenderable.m_baseVertex = 0;
        DirectX::XMVECTOR mirrorPlane = DirectX::XMLoadFloat4(&m_mirrorPlane);
        DirectX::XMStoreFloat4x4(&mirroredMetalGridSphereRenderable.m_model, DirectX::XMMatrixReflect(mirrorPlane));
        mirroredMetalGridSphereRenderable.m_indexCount = static_cast<UINT>(sphereIndexCount);
        m_mirroredSceneRenderables.emplace_back(mirroredMetalGridSphereRenderable);
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
        size_t totalRenderableCount = m_sceneRenderables.size() + m_mirrorRenderables.size() + m_mirroredSceneRenderables.size();
        ThrowIfFailed(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameResources.m_pCommandAllocator)));
        frameResources.m_pCbPass = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), 1, sizeof(PassConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pCbLights = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), 2, sizeof(LightConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pCbObjects = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), totalRenderableCount, sizeof(ObjectConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
        frameResources.m_pCbMaterials = std::make_unique<D3D12Util::MappedGPUBuffer>(m_pDevice.Get(), totalRenderableCount, sizeof(MaterialConstants), D3D12Util::MappedGPUBuffer::Flags::ConstantBuffer);
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

        D3D12_ROOT_PARAMETER1 lightCbParameter = {};
        lightCbParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        lightCbParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        lightCbParameter.Descriptor.ShaderRegister = 2;

        D3D12_ROOT_PARAMETER1 passCbParameter = {};
        passCbParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        passCbParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
        passCbParameter.Descriptor.ShaderRegister = 3;

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
            lightCbParameter,
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

        desc.DSVFormat = m_depthStencilFormat;

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

        {
            std::vector<D3D_SHADER_MACRO> defines;
            if (m_useFog) {
                defines.emplace_back(D3D_SHADER_MACRO{"USE_FOG", ""});
            }
            defines.emplace_back(D3D_SHADER_MACRO{ nullptr, nullptr });

            Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader = D3D12Util::compileShader(
                L"data/shaders/chapter11/mirror.hlsl", "vs", "vs_5_1", defines.data());
            desc.VS.pShaderBytecode = pVertexShader->GetBufferPointer();
            desc.VS.BytecodeLength = pVertexShader->GetBufferSize();

            Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader = D3D12Util::compileShader(
                L"data/shaders/chapter11/mirror.hlsl", "ps", "ps_5_1", defines.data());
            desc.PS.pShaderBytecode = pPixelShader->GetBufferPointer();
            desc.PS.BytecodeLength = pPixelShader->GetBufferSize();

            ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pPipelineStateOpaque)));

            D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaBlendDesc = desc;
            alphaBlendDesc.BlendState.RenderTarget[0].BlendEnable = true;
            alphaBlendDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            alphaBlendDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            alphaBlendDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            alphaBlendDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            alphaBlendDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            alphaBlendDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

            alphaBlendDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

            ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&alphaBlendDesc, IID_PPV_ARGS(&m_pPipelineStateAlphaBlend)));

            D3D12_GRAPHICS_PIPELINE_STATE_DESC stencilWriteDesc = desc;
            stencilWriteDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
            stencilWriteDesc.DepthStencilState.StencilEnable = true;
            stencilWriteDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            stencilWriteDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            stencilWriteDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
            stencilWriteDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
            stencilWriteDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;

            stencilWriteDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
            stencilWriteDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;

            ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&stencilWriteDesc, IID_PPV_ARGS(&m_pPipelineStateStencilWrite)));

            D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueMirroredDesc = desc;
            opaqueMirroredDesc.RasterizerState.FrontCounterClockwise = true;
            opaqueMirroredDesc.DepthStencilState.StencilEnable = true;
            opaqueMirroredDesc.DepthStencilState.StencilWriteMask = 0;
            opaqueMirroredDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            opaqueMirroredDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            opaqueMirroredDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
            opaqueMirroredDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;

            ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&opaqueMirroredDesc, IID_PPV_ARGS(&m_pPipelineStateOpaqueMirrored)));
        }
    }

    ThrowIfFailed(m_pCommandList->Close());

    ID3D12CommandList* pCommandList = m_pCommandList.Get();
    m_pCommandQueue->ExecuteCommandLists(1, &pCommandList);
    flushCommandQueue();
    ThrowIfFailed(m_pCommandAllocator->Reset());
};

void Mirror::update(float dt)
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
        passConstants.ambientLight = { 0.05f, 0.05f, 0.05f };
        passConstants.cameraPositionW = m_camera.m_position;
        passConstants.alphaClipThreshold = 0.02f;

        passConstants.directionalLightCount = 0u;
        passConstants.pointLightCount = 3u;

        passConstants.fogColor = { m_clearColor[0], m_clearColor[1], m_clearColor[2] };
        passConstants.fogBegin = 2.0f;
        passConstants.fogEnd = 20.0f;

        curFrameResources.m_pCbPass->copyData(&passConstants, sizeof(PassConstants));
    }

    {
        LightConstants lightConstants = {};
        lightConstants.lightData[0].color = { 1.0f, 1.0f, 1.0f };
        lightConstants.lightData[0].position = { -5.0f, -10.0f, -10.0f };
        lightConstants.lightData[0].falloffBegin = 1.0f;
        lightConstants.lightData[0].falloffEnd = 20.0f;

        lightConstants.lightData[1].color = { 1.0f, 1.0f, 1.0f };
        lightConstants.lightData[1].position = { 0.0f, 5.0f, 0.0f };
        lightConstants.lightData[1].falloffBegin = 1.0f;
        lightConstants.lightData[1].falloffEnd = 20.0f;

        lightConstants.lightData[2].color = { 1.0f, 1.0f, 1.0f };
        lightConstants.lightData[2].position = { 2.0f, 0.0f, 10.0f };
        lightConstants.lightData[2].falloffBegin = 1.0f;
        lightConstants.lightData[2].falloffEnd = 20.0f;

        LightConstants mirroredLightConstants = lightConstants;

        DirectX::XMMATRIX mirrorMatrix = DirectX::XMMatrixReflect(DirectX::XMLoadFloat4(&m_mirrorPlane));

        DirectX::XMVECTOR mirroredPosistion = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&mirroredLightConstants.lightData[0].position), mirrorMatrix);
        DirectX::XMStoreFloat3(&mirroredLightConstants.lightData[0].position, mirroredPosistion);

        mirroredPosistion = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&mirroredLightConstants.lightData[1].position), mirrorMatrix);
        DirectX::XMStoreFloat3(&mirroredLightConstants.lightData[1].position, mirroredPosistion);

        mirroredPosistion = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&mirroredLightConstants.lightData[2].position), mirrorMatrix);
        DirectX::XMStoreFloat3(&mirroredLightConstants.lightData[2].position, mirroredPosistion);

        curFrameResources.m_pCbLights->copyData(&lightConstants, sizeof(LightConstants));
        curFrameResources.m_pCbLights->copyData(&mirroredLightConstants, sizeof(LightConstants), curFrameResources.m_pCbLights->getElementSize());
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

    for (const auto& renderable : m_sceneRenderables)
    {
        updateObjectCbContents(renderable);
    }

    for (const auto& renderable : m_mirrorRenderables)
    {
        updateObjectCbContents(renderable);
    }

    for (const auto& renderable : m_mirroredSceneRenderables)
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

void Mirror::render()
{
    FrameResources& curFrameResources = m_frameResources[m_curFrameResourcesIndex];
    curFrameResources.m_fenceValue = ++m_curFrameFenceValue;

    ThrowIfFailed(curFrameResources.m_pCommandAllocator->Reset());
    ThrowIfFailed(m_pCommandList->Reset(curFrameResources.m_pCommandAllocator.Get(), m_pPipelineStateOpaque.Get()));

    {
        D3D12_RESOURCE_BARRIER presentToRenderTargetTransition = D3D12Util::TransitionBarrier(getCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_pCommandList->ResourceBarrier(1, &presentToRenderTargetTransition);
    }

    m_pCommandList->ClearDepthStencilView(getCurrentDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    m_pCommandList->ClearRenderTargetView(getCurrentBackBufferView(), m_clearColor, 0, nullptr);

    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
    m_pCommandList->SetGraphicsRootConstantBufferView(3, curFrameResources.m_pCbPass->getResource()->GetGPUVirtualAddress());

    D3D12_GPU_VIRTUAL_ADDRESS lightCbGpuAddress = curFrameResources.m_pCbLights->getResource()->GetGPUVirtualAddress();
    m_pCommandList->SetGraphicsRootConstantBufferView(2, lightCbGpuAddress);
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
        m_pCommandList->SetGraphicsRootDescriptorTable(4, textureSrvGpuHandle);

        const D3D12_INDEX_BUFFER_VIEW indexBufferView = m_meshes[renderable.m_meshIndex].getIndexBufferView();
        m_pCommandList->IASetIndexBuffer(&indexBufferView);

        const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = m_meshes[renderable.m_meshIndex].getVertexBufferView();
        m_pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        m_pCommandList->IASetPrimitiveTopology(renderable.m_topology);

        m_pCommandList->DrawIndexedInstanced(renderable.m_indexCount, 1u, renderable.m_startIndex, renderable.m_baseVertex, 0u);
    };

    for (const auto& renderable : m_sceneRenderables)
    {
        renderRenderable(renderable);
    }

    m_pCommandList->SetPipelineState(m_pPipelineStateStencilWrite.Get());
    for (const auto& renderable : m_mirrorRenderables)
    {
        m_pCommandList->OMSetStencilRef(1u);
        renderRenderable(renderable);
    }

    D3D12_GPU_VIRTUAL_ADDRESS mirroredLightCbGpuAddress = lightCbGpuAddress;
    mirroredLightCbGpuAddress += curFrameResources.m_pCbLights->getElementSize();
    m_pCommandList->SetGraphicsRootConstantBufferView(2, mirroredLightCbGpuAddress);
    m_pCommandList->SetPipelineState(m_pPipelineStateOpaqueMirrored.Get());
    for (const auto& renderable : m_mirroredSceneRenderables)
    {
        renderRenderable(renderable);
    }

    m_pCommandList->SetGraphicsRootConstantBufferView(2, lightCbGpuAddress);
    m_pCommandList->SetPipelineState(m_pPipelineStateAlphaBlend.Get());
    for (const auto& renderable : m_mirrorRenderables)
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
