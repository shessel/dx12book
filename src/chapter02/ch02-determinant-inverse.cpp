#include <iostream>

#include "matrix.h"

int main()
{
    Matrix<float, 2> m2 = { {
        {21, -4},
        {10, 7},
    } };

    std::cout << determinant(m2) << "\n";

    Matrix<float, 3> m3 = { {
        {2, 0, 0},
        {0, 3, 0},
        {0, 0, 7},
    } };

    std::cout << determinant(m3) << "\n";

    Matrix4<float> m4 = {{
        {1.0f, 4.0f ,0.0f,-5.0f},
        {5.0f,9.2f,-2.7f,0.0f},
        {0.0f,-1.3f,0.0f,-4.2f},
        {2.0f, 3.1f, -9.8f, 0.0f},
    } };

    std::cout << determinant(m4) << "\n";

    Matrix4<float> m4_adj = adjugate(m4);
    std::cout << m4_adj << "\n";;

    Matrix4<float> m4_inv = inverse(m4);
    std::cout << m4_inv << "\n";

    Matrix4<float> identity = m4 * m4_inv;
    std::cout << identity << "\n";

    identity = m4_inv * m4;
    std::cout << identity << "\n";

    return 0;
}