#include <cassert>
#include <array>
#include <vector>
#include <ostream>

template <typename T, size_t N>
using Matrix = std::array<std::array<T, N>, N>;

template <typename T>
using Matrix4 = Matrix<T, 4>;

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