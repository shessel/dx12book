#include <cassert>
#include <vector>
#include <iostream>

#include "matrix.h"

template <typename T, size_t N>
T sub_determinant(Matrix<T, N> &matrix, std::vector<size_t>& row_exclude, std::vector<size_t>& col_exclude)
{
    assert(row_exclude.size() == col_exclude.size());
    if (row_exclude.size() < N - 2)
    {
        T determinant = static_cast<T>(0);
        T sign = static_cast<T>(1);

        for (size_t row = 0; row < N; ++row)
        {
            if (std::find(row_exclude.begin(), row_exclude.end(), row) == row_exclude.end())
            {
                row_exclude.push_back(row);
                break;
            }
        }

        for (size_t col = 0; col < N; ++col)
        {
            if (std::find(col_exclude.begin(), col_exclude.end(), col) != col_exclude.end())
            {
                continue;
            }

            col_exclude.push_back(col);

            determinant += sign * matrix[row_exclude.back()][col] * sub_determinant(matrix, row_exclude, col_exclude);
            sign *= static_cast<T>(-1);

            col_exclude.pop_back();
        }
        row_exclude.pop_back();
        return determinant;
    }
    else
    {
        size_t rows[2] = {};
        size_t cols[2] = {};

        for (size_t col = 0, i = 0; col < N; ++col)
        {
            if (std::find(col_exclude.begin(), col_exclude.end(), col) != col_exclude.end())
            {
                continue;
            }

            cols[i++] = col;
        }

        for (size_t row = 0, i = 0; row < N; ++row)
        {
            if (std::find(row_exclude.begin(), row_exclude.end(), row) != row_exclude.end())
            {
                continue;
            }

            rows[i++] = row;
        }

        return matrix[rows[0]][cols[0]] * matrix[rows[1]][cols[1]] - matrix[rows[0]][cols[1]] * matrix[rows[1]][cols[0]];
    }
}

template <typename T, size_t N>
T determinant(Matrix<T, N> &matrix)
{
    std::vector<size_t> col_exclude;
    std::vector<size_t> row_exclude;
    return sub_determinant(matrix, row_exclude, col_exclude);
}

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

    Matrix4<float> m4 = {{
        {1.0f, 4.0f ,0.0f,-5.0f},
        {5.0f,9.2f,-2.7f,0.0f},
        {0.0f,-1.3f,0.0f,-4.2f},
        {2.0f, 3.1f, -9.8f, 0.0f},
    } };

    std::cout << determinant(m4) << "\n";

    return 0;
}