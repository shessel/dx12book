#include "ArcBallCamera.h"

#include <cmath>


ArcBallCamera::ArcBallCamera(const float distance, const float phi, const float theta)
    : m_distance(distance), m_phi(phi), m_theta(theta)
{}

void ArcBallCamera::update()
{
    float sinPhi, cosPhi, sinTheta, cosTheta;
    DirectX::XMScalarSinCos(&sinPhi, &cosPhi, m_phi);
    DirectX::XMScalarSinCos(&sinTheta, &cosTheta, m_theta);

    float x = m_distance * sinPhi * cosTheta;
    float y = m_distance * sinTheta;
    float z = m_distance * cosPhi * cosTheta;;

    DirectX::XMVECTOR position = { x, y, z };
    DirectX::XMVECTOR focus = DirectX::XMLoadFloat3(&m_focus);
    position = DirectX::XMVectorAdd(focus, position);
    DirectX::XMStoreFloat3(&m_position, position);
    DirectX::XMVECTOR up = { 0.0f, 1.0f, 0.0f };

    DirectX::XMStoreFloat4x4(&m_matrix, DirectX::XMMatrixLookAtRH(position, focus, up));
}

void ArcBallCamera::updatePhi(const float dPhi)
{
    m_phi += dPhi;
}

void ArcBallCamera::updateTheta(const float dTheta)
{
    m_theta += dTheta;
    m_theta = std::fmaxf(m_theta, -DirectX::XM_PIDIV2 + 0.001f);
    m_theta = std::fminf(m_theta, DirectX::XM_PIDIV2 - 0.001f);
}

void ArcBallCamera::updateDistance(const float dDistance)
{
    m_distance += dDistance;
    m_distance = std::fmaxf(m_distance, 0.1f);
}

void ArcBallCamera::move(const DirectX::XMFLOAT3& offset)
{
    DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&offset);
    DirectX::XMMATRIX m = DirectX::XMLoadFloat4x4(&m_matrix);
    m = DirectX::XMMatrixTranspose(m);
    v = DirectX::XMVector3Transform(v, m);
    DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&m_focus);
    v = DirectX::XMVectorAdd(v, c);
    DirectX::XMStoreFloat3(&m_focus, v);
}