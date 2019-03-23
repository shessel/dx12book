#include <array>
#include <iostream>

#include "matrix.h"

template <typename T, size_t N>
void transpose(Matrix<T, N>& matrix)
{
    for (size_t row = 0; row < N - 1; ++row)
    {
        // swapping is symmetrical so we only need to process half of the matrix => col = row
        // main diagonal stays untouched => col = row + 1
        for (size_t col = row + 1; col < N; ++col)
        {
            std::swap(matrix[row][col], matrix[col][row]);
        }
    }
}

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