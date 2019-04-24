#pragma once

#include "AppBase.h"
#include "Mesh.h"

#include <memory>
#include <vector>

#include "DirectXMath.h"
#include "D3D12Util.h"

class BoxDemo : public AppBase
{
public:
    using AppBase::AppBase;

    virtual void initialize() override;
    virtual void update(float dt) override;
    virtual void render() override;
    virtual void onMouseDown(int16_t xPos, int16_t yPos, uint8_t buttons) override;
    virtual void onMouseUp(int16_t xPos, int16_t yPos, uint8_t buttons) override;
    virtual void onMouseMove(int16_t xPos, int16_t yPos, uint8_t buttons) override;

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
        float time;
    };

    PerObjectConstants m_perObjectConstants = {};

    std::unique_ptr<Mesh> m_pMesh;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pCbvHeap;
    std::unique_ptr<D3D12Util::ConstantBuffer> m_pConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

    Microsoft::WRL::ComPtr<ID3DBlob> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> m_pPixelShader;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

    int16_t m_lastMouseX = 0;
    int16_t m_lastMouseY = 0;
    int16_t m_curMouseX = 0;
    int16_t m_curMouseY = 0;
    float m_camAnglePhi = 0.0f;
    float m_camAngleTheta = 0.0f;
    float m_camDistance = 3.0f;
};
