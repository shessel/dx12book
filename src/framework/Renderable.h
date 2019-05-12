#pragma once

#include "d3d12.h"
#include "DirectXMath.h"

struct Renderable
{
    DirectX::XMFLOAT4X4 m_model = {
        1.0f,0.0f,0.0f,0.0f,
        0.0f,1.0f,0.0f,0.0f,
        0.0f,0.0f,1.0f,0.0f,
        0.0f,0.0f,0.0f,1.0f,
    };

    size_t m_meshIndex;
    size_t m_materialIndex;

    D3D_PRIMITIVE_TOPOLOGY m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT m_cbIndex = UINT_MAX;
    UINT m_startIndex = UINT_MAX;
    UINT m_baseVertex = UINT_MAX;
    UINT m_indexCount = UINT_MAX;
};