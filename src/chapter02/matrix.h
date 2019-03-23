#include <array>
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