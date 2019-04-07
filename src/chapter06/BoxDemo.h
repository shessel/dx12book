#pragma once

#include "AppBase.h"

#include <vector>

#include "DirectXMath.h"

class BoxDemo : public AppBase
{
public:
    using AppBase::AppBase;

    virtual void initialize();
    virtual void update(float /*dt*/) {};
    virtual void render() {};

protected:
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
    };

    std::vector<Vertex> m_vertices;

    Microsoft::WRL::ComPtr<ID3D12Resource1> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource1> m_vertexBufferUpload;
};