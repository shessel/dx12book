#pragma once

#include "DirectXMath.h"

struct ArcBallCamera
{
    ArcBallCamera(const float distance = 1.0f, const float phi = 0.0f, const float theta = 0.0f);
    void update();
    void updatePhi(const float dPhi);
    void updateTheta(const float dTheta);
    void updateDistance(const float dDistance);
    void move(const DirectX::XMFLOAT3& offset);

    float m_phi = 0.0f;
    float m_theta = 0.0f;
    float m_distance = 1.0f;
    DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 m_focus = { 0.0f, 0.0f, 0.0f };

    DirectX::XMFLOAT4X4 m_matrix;
};
