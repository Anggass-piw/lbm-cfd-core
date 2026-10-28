#pragma once

#include <array>
#include <cstddef>

namespace lbm {

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

// Standard D3Q19 ordering: rest, six axis directions, then twelve face diagonals.
inline constexpr std::size_t Q = 19;
inline constexpr std::array<Vec3, Q> c{{
    {0, 0, 0},
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
    {1, 1, 0}, {-1, -1, 0}, {1, -1, 0}, {-1, 1, 0},
    {1, 0, 1}, {-1, 0, -1}, {1, 0, -1}, {-1, 0, 1},
    {0, 1, 1}, {0, -1, -1}, {0, 1, -1}, {0, -1, 1}
}};
inline constexpr std::array<double, Q> w{{
    1.0 / 3.0,
    1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0,
    1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
    1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
    1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0
}};
inline constexpr std::array<std::size_t, Q> opposite{{
    0, 2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13, 16, 15, 18, 17
}};
inline constexpr double cs2 = 1.0 / 3.0;

inline double equilibrium(std::size_t direction, double rho, const Vec3& velocity) {
    const double cu = c[direction].x * velocity.x +
                      c[direction].y * velocity.y +
                      c[direction].z * velocity.z;
    const double u2 = velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z;
    return w[direction] * rho * (1.0 + cu / cs2 +
           (cu * cu) / (2.0 * cs2 * cs2) - u2 / (2.0 * cs2));
}

}  // namespace lbm
