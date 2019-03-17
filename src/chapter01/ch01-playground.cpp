#include "DirectXMath.h"
#include "DirectXPackedVector.h"

#include <iostream>

std::ostream& XM_CALLCONV operator<<(std::ostream& o, const DirectX::FXMVECTOR v)
{
    DirectX::XMFLOAT4 vec;
    DirectX::XMStoreFloat4(&vec, v);
    return o << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
}

int main()
{
    DirectX::XMVECTORF32 x{ 1.0f, 0.0f, 0.0f, 0.0f };
    DirectX::XMVECTORF32 y{ 0.0f, 1.0f, 0.0f, 0.0f };

    DirectX::XMVECTOR x_cross_y = DirectX::XMVector3Cross(x, y);
    DirectX::XMVECTOR y_cross_x = DirectX::XMVector3Cross(y, x);

    std::cout << "x cross y: " << x_cross_y << std::endl;
    std::cout << "y cross x: " << y_cross_x << std::endl;

    return 0;
}