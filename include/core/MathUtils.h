#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>

namespace MathUtils {
    constexpr double PI = 3.14159265358979323846;
    constexpr double EPS = 1e-9;

    inline double tuDoSangRadian(double deg) {
        return deg * (PI / 180.0);
    }

    inline double radianSangDo(double rad) {
        return rad * (180.0 / PI);
    }

    inline bool bangNhau(double a, double b, double eps = EPS) {
        return std::abs(a - b) < eps;
    }

    inline bool nhoHon(double a, double b, double eps = EPS) {
        return (b - a) > eps;
    }

    inline bool lonHon(double a, double b, double eps = EPS) {
        return (a - b) > eps;
    }
}

#endif // MATH_UTILS_H