#pragma once

#include "DirectXMath.h"

struct Material
{
    size_t m_framesDirtyCount = 0u;
    UINT m_cbIndex = UINT_MAX;
    DirectX::XMFLOAT4 m_albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 m_fresnelR0 = { 0.01f, 0.01f, 0.01f };
    float m_roughness = 1.0f;
};