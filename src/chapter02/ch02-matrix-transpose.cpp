#include <array>
#include <iostream>

template <typename T, size_t N>
using Matrix = std::array<std::array<T, N>, N>;

template <typename T>
using Matrix4 = Matrix<T, 4>;

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

template <typename T, size_t N>
std::ostream& operator<<(std::ostream &o, Matrix<T, N> &matrix)
{
    for (const auto &row : matrix)
    {
        bool first = true;
        for (const auto &element : row)
        {
            o << (first ? "" : ",\t") << element;
            first = false;
        }
        o << "\n";
    }
    return o;
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