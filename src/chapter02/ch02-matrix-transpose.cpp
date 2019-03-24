#include <iostream>

#include "matrix.h"

int main()
{
    Matrix4<float> m = {{
        {1, 2, 0, 0},
        {0, 2, 5, 0},
        {0, 2, 0, 3},
        {0, 2, 0, 1},
    }};

    std::cout << m << "\n";
    
    Matrix4<float> m_transpose = m;
    transpose(m_transpose);
    std::cout << m_transpose;

    return 0;
}