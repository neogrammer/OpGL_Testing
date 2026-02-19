#pragma once
#include <array>
#include <type_traits>
#include <concepts>
#include <cmath>
#include <limits>
#include <cassert>

#include "v3.h"

template<typename T>
    requires std::is_arithmetic_v<T>
struct m3
{
    // Column-major: a[col][row]
    std::array<std::array<T, 3>, 3> a{ {
        {{ (T)1, (T)0, (T)0 }},
        {{ (T)0, (T)1, (T)0 }},
        {{ (T)0, (T)0, (T)1 }},
    } };

    using value_type = T;

    constexpr m3() = default;

    // Construct from 9 values (column-major)
    constexpr m3(
        T c0r0, T c0r1, T c0r2,
        T c1r0, T c1r1, T c1r2,
        T c2r0, T c2r1, T c2r2
    ) : a{ {
        {{ c0r0, c0r1, c0r2 }},
        {{ c1r0, c1r1, c1r2 }},
        {{ c2r0, c2r1, c2r2 }},
    } } {
    }

    // Construct from columns
    constexpr m3(const v3<T>& c0, const v3<T>& c1, const v3<T>& c2)
        : a{ { {{c0.x,c0.y,c0.z}}, {{c1.x,c1.y,c1.z}}, {{c2.x,c2.y,c2.z}} } } {
    }

    static constexpr m3 identity() { return m3{}; }

    // Access in math order: (row, col)
    constexpr T& at(int row, int col) { return a[(size_t)col][(size_t)row]; }
    constexpr const T& at(int row, int col) const { return a[(size_t)col][(size_t)row]; }

    // -----------------------------
    // Basic ops
    // -----------------------------
    constexpr m3 operator+(const m3& b) const
    {
        m3 r{};
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            r.a[c][r0] = a[c][r0] + b.a[c][r0];
        return r;
    }

    constexpr m3 operator-(const m3& b) const
    {
        m3 r{};
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            r.a[c][r0] = a[c][r0] - b.a[c][r0];
        return r;
    }

    constexpr m3& operator+=(const m3& b)
    {
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            a[c][r0] += b.a[c][r0];
        return *this;
    }

    constexpr m3& operator-=(const m3& b)
    {
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            a[c][r0] -= b.a[c][r0];
        return *this;
    }

    constexpr m3 operator*(T s) const
    {
        m3 r{};
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            r.a[c][r0] = a[c][r0] * s;
        return r;
    }

    constexpr m3 operator/(T s) const
    {
        m3 r{};
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            r.a[c][r0] = a[c][r0] / s;
        return r;
    }

    constexpr m3& operator*=(T s)
    {
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            a[c][r0] *= s;
        return *this;
    }

    constexpr m3& operator/=(T s)
    {
        for (int c = 0; c < 3; ++c) for (int r0 = 0; r0 < 3; ++r0)
            a[c][r0] /= s;
        return *this;
    }

    friend constexpr m3 operator*(T s, const m3& M) { return M * s; }

    // -----------------------------
    // Matrix multiply
    // -----------------------------
    constexpr m3 operator*(const m3& b) const
    {
        m3 r{};
        for (int col = 0; col < 3; ++col)
        {
            for (int row = 0; row < 3; ++row)
            {
                T sum = (T)0;
                for (int k = 0; k < 3; ++k)
                    sum += at(row, k) * b.at(k, col);
                r.at(row, col) = sum;
            }
        }
        return r;
    }

    constexpr m3& operator*=(const m3& b)
    {
        *this = (*this) * b;
        return *this;
    }

    // -----------------------------
    // Matrix * vector
    // -----------------------------
    constexpr v3<T> operator*(const v3<T>& v) const
    {
        return v3<T>{
            at(0, 0)* v.x + at(0, 1) * v.y + at(0, 2) * v.z,
                at(1, 0)* v.x + at(1, 1) * v.y + at(1, 2) * v.z,
                at(2, 0)* v.x + at(2, 1) * v.y + at(2, 2) * v.z
        };
    }

    // Row-vector multiply: v * M
    friend constexpr v3<T> operator*(const v3<T>& v, const m3& M)
    {
        // (1x3) * (3x3) = (1x3)
        return v3<T>{
            v.x* M.at(0, 0) + v.y * M.at(1, 0) + v.z * M.at(2, 0),
                v.x* M.at(0, 1) + v.y * M.at(1, 1) + v.z * M.at(2, 1),
                v.x* M.at(0, 2) + v.y * M.at(1, 2) + v.z * M.at(2, 2)
        };
    }

    // -----------------------------
    // Transpose / det / inverse
    // -----------------------------
    constexpr m3 transpose() const
    {
        m3 r{};
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
                r.at(row, col) = at(col, row);
        return r;
    }

    double determinant() const
    {
        const double m00 = (double)at(0, 0), m01 = (double)at(0, 1), m02 = (double)at(0, 2);
        const double m10 = (double)at(1, 0), m11 = (double)at(1, 1), m12 = (double)at(1, 2);
        const double m20 = (double)at(2, 0), m21 = (double)at(2, 1), m22 = (double)at(2, 2);

        return m00 * (m11 * m22 - m12 * m21)
            - m01 * (m10 * m22 - m12 * m20)
            + m02 * (m10 * m21 - m11 * m20);
    }

    m3 inverse() const
    {
        const double det = determinant();
        assert(std::fabs(det) > std::numeric_limits<double>::epsilon());

        const double invDet = 1.0 / det;

        const double m00 = (double)at(0, 0), m01 = (double)at(0, 1), m02 = (double)at(0, 2);
        const double m10 = (double)at(1, 0), m11 = (double)at(1, 1), m12 = (double)at(1, 2);
        const double m20 = (double)at(2, 0), m21 = (double)at(2, 1), m22 = (double)at(2, 2);

        // adjugate (cofactor matrix transposed)
        m3 r{};
        r.at(0, 0) = (T)((m11 * m22 - m12 * m21) * invDet);
        r.at(0, 1) = (T)((m02 * m21 - m01 * m22) * invDet);
        r.at(0, 2) = (T)((m01 * m12 - m02 * m11) * invDet);

        r.at(1, 0) = (T)((m12 * m20 - m10 * m22) * invDet);
        r.at(1, 1) = (T)((m00 * m22 - m02 * m20) * invDet);
        r.at(1, 2) = (T)((m02 * m10 - m00 * m12) * invDet);

        r.at(2, 0) = (T)((m10 * m21 - m11 * m20) * invDet);
        r.at(2, 1) = (T)((m01 * m20 - m00 * m21) * invDet);
        r.at(2, 2) = (T)((m00 * m11 - m01 * m10) * invDet);

        return r;
    }
};

// aliases
using m3f = m3<float>;
using m3d = m3<double>;
using m3i = m3<std::int32_t>;
using m3u = m3<std::uint32_t>;