#pragma once

#include "AppBase.h"

#include <memory>
#include <vector>

#include "DirectXMath.h"
#include "D3D12Util.h"

class BoxDemo : public AppBase
{
public:
    using AppBase::AppBase;

    virtual void initialize();
    virtual void update(float dt);
    virtual void render();

protected:
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
    };

    struct PerObjectConstants
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    std::vector<Vertex> m_vertices;
    std::vector<std::uint16_t> m_indices;

    Microsoft::WRL::ComPtr<ID3D12Resource1> m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource1> m_pVertexBufferUpload;

    Microsoft::WRL::ComPtr<ID3D12Resource1> m_pIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource1> m_pIndexBufferUpload;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pCbvHeap;
    std::unique_ptr<D3D12Util::ConstantBuffer> m_pConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

    Microsoft::WRL::ComPtr<ID3DBlob> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> m_pPixelShader;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
};
