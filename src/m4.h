#pragma once
#include <array>
#include <type_traits>
#include <concepts>
#include <cmath>
#include <limits>
#include <cassert>

#include "v4.h"

template<typename T>
    requires std::is_arithmetic_v<T>
struct m4
{

    // Column-major: a[col][row]
    std::array<std::array<T, 4>, 4> a{ {
        {{ (T)1, (T)0, (T)0, (T)0 }},
        {{ (T)0, (T)1, (T)0, (T)0 }},
        {{ (T)0, (T)0, (T)1, (T)0 }},
        {{ (T)0, (T)0, (T)0, (T)1 }},
    } };

    using value_type = T;

    constexpr m4() = default;

    // Construct from 16 values (column-major)
    constexpr m4(
        T c0r0, T c0r1, T c0r2, T c0r3,
        T c1r0, T c1r1, T c1r2, T c1r3,
        T c2r0, T c2r1, T c2r2, T c2r3,
        T c3r0, T c3r1, T c3r2, T c3r3
    ) : a{ {
        {{ c0r0, c0r1, c0r2, c0r3 }},
        {{ c1r0, c1r1, c1r2, c1r3 }},
        {{ c2r0, c2r1, c2r2, c2r3 }},
        {{ c3r0, c3r1, c3r2, c3r3 }},
    } } {
    }

    static constexpr m4 identity() { return m4{}; }

    // Access in math order: (row, col)
    constexpr T& at(int row, int col) { return a[(size_t)col][(size_t)row]; }
    constexpr const T& at(int row, int col) const { return a[(size_t)col][(size_t)row]; }

    //  Gangsta functions
    static constexpr m4 identity() { return m4{}; }

    // Translation (adds to last column in column-major, for column-vector math)
    static constexpr m4 translation(const v3<T>& t)
    {
        m4 M = identity();
        M.at(0, 3) = t.x;
        M.at(1, 3) = t.y;
        M.at(2, 3) = t.z;
        return M;
    }

    static constexpr m4 scale(const v3<T>& s)
    {
        m4 M{};
        M.at(0, 0) = s.x;  M.at(0, 1) = (T)0; M.at(0, 2) = (T)0; M.at(0, 3) = (T)0;
        M.at(1, 0) = (T)0; M.at(1, 1) = s.y;  M.at(1, 2) = (T)0; M.at(1, 3) = (T)0;
        M.at(2, 0) = (T)0; M.at(2, 1) = (T)0; M.at(2, 2) = s.z;  M.at(2, 3) = (T)0;
        M.at(3, 0) = (T)0; M.at(3, 1) = (T)0; M.at(3, 2) = (T)0; M.at(3, 3) = (T)1;
        return M;
    }

    // Angles are radians.
    static m4 rotationX(T radians)
    {
        const double c = std::cos((double)radians);
        const double s = std::sin((double)radians);

        m4 M = identity();
        M.at(1, 1) = (T)c;  M.at(1, 2) = (T)-s;
        M.at(2, 1) = (T)s;  M.at(2, 2) = (T)c;
        return M;
    }

    static m4 rotationY(T radians)
    {
        const double c = std::cos((double)radians);
        const double s = std::sin((double)radians);

        m4 M = identity();
        M.at(0, 0) = (T)c;  M.at(0, 2) = (T)s;
        M.at(2, 0) = (T)-s; M.at(2, 2) = (T)c;
        return M;
    }

    static m4 rotationZ(T radians)
    {
        const double c = std::cos((double)radians);
        const double s = std::sin((double)radians);

        m4 M = identity();
        M.at(0, 0) = (T)c;  M.at(0, 1) = (T)-s;
        M.at(1, 0) = (T)s;  M.at(1, 1) = (T)c;
        return M;
    }

    // Standard right-handed perspective (OpenGL style NDC: z in [-1, 1]).
    // fovyRadians: vertical FOV in radians
    static m4 perspectiveRH_OpenGL(T fovyRadians, T aspect, T zNear, T zFar)
    {
        // requires floating point for sanity
        static_assert(std::is_floating_point_v<T>, "perspectiveRH_OpenGL requires floating point T");

        const T f = (T)(1.0 / std::tan((double)fovyRadians * 0.5));

        m4 M{};
        M.at(0, 0) = f / aspect;  M.at(0, 1) = (T)0; M.at(0, 2) = (T)0;                         M.at(0, 3) = (T)0;
        M.at(1, 0) = (T)0;        M.at(1, 1) = f;     M.at(1, 2) = (T)0;                         M.at(1, 3) = (T)0;
        M.at(2, 0) = (T)0;        M.at(2, 1) = (T)0;  M.at(2, 2) = (zFar + zNear) / (zNear - zFar); M.at(2, 3) = (T)((2 * (double)zFar * (double)zNear) / ((double)zNear - (double)zFar));
        M.at(3, 0) = (T)0;        M.at(3, 1) = (T)0;  M.at(3, 2) = (T)-1;                        M.at(3, 3) = (T)0;
        return M;
    }

    // Right-handed orthographic (OpenGL NDC z in [-1, 1])
    static m4 orthoRH_OpenGL(T left, T right, T bottom, T top, T zNear, T zFar)
    {
        static_assert(std::is_floating_point_v<T>, "orthoRH_OpenGL requires floating point T");

        m4 M = identity();
        M.at(0, 0) = (T)(2 / (double)(right - left));
        M.at(1, 1) = (T)(2 / (double)(top - bottom));
        M.at(2, 2) = (T)(2 / (double)(zNear - zFar));

        M.at(0, 3) = (T)(-(double)(right + left) / (double)(right - left));
        M.at(1, 3) = (T)(-(double)(top + bottom) / (double)(top - bottom));
        M.at(2, 3) = (T)(-(double)(zFar + zNear) / (double)(zFar - zNear));
        return M;
    }

    // Right-handed LookAt (OpenGL style)
    // eye: camera position
    // center: target point camera looks at
    // up: world up (doesn't need to be perfectly orthogonal)
    static m4 lookAtRH(const v3<T>& eye, const v3<T>& center, const v3<T>& up)
    {
        static_assert(std::is_floating_point_v<T>, "lookAtRH requires floating point T");

        // Forward (camera -Z points toward target in RH OpenGL convention)
        v3<T> f = (center - eye).normalized();          // forward
        v3<T> s = f.cross(up).normalized();             // right
        v3<T> u = s.cross(f);                           // corrected up

        m4 M = identity();

        // Basis into the rotation part (columns are basis vectors)
        M.at(0, 0) = s.x; M.at(1, 0) = s.y; M.at(2, 0) = s.z;
        M.at(0, 1) = u.x; M.at(1, 1) = u.y; M.at(2, 1) = u.z;
        M.at(0, 2) = (T)-f.x; M.at(1, 2) = (T)-f.y; M.at(2, 2) = (T)-f.z;

        // Translation
        M.at(0, 3) = (T)-s.dot(eye);
        M.at(1, 3) = (T)-u.dot(eye);
        M.at(2, 3) = (T)f.dot(eye);   // note sign due to -f in matrix

        return M;
    }

    // Convenience: degrees->radians
    static constexpr T radians(T degrees)
    {
        return (T)((double)degrees * (3.14159265358979323846 / 180.0));
    }


    // -----------------------------
    // Basic ops
    // -----------------------------
    constexpr m4 operator+(const m4& b) const
    {
        m4 r{};
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            r.a[c][r0] = a[c][r0] + b.a[c][r0];
        return r;
    }

    constexpr m4 operator-(const m4& b) const
    {
        m4 r{};
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            r.a[c][r0] = a[c][r0] - b.a[c][r0];
        return r;
    }

    constexpr m4& operator+=(const m4& b)
    {
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            a[c][r0] += b.a[c][r0];
        return *this;
    }

    constexpr m4& operator-=(const m4& b)
    {
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            a[c][r0] -= b.a[c][r0];
        return *this;
    }

    constexpr m4 operator*(T s) const
    {
        m4 r{};
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            r.a[c][r0] = a[c][r0] * s;
        return r;
    }

    constexpr m4 operator/(T s) const
    {
        m4 r{};
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            r.a[c][r0] = a[c][r0] / s;
        return r;
    }

    constexpr m4& operator*=(T s)
    {
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            a[c][r0] *= s;
        return *this;
    }

    constexpr m4& operator/=(T s)
    {
        for (int c = 0; c < 4; ++c) for (int r0 = 0; r0 < 4; ++r0)
            a[c][r0] /= s;
        return *this;
    }

    friend constexpr m4 operator*(T s, const m4& M) { return M * s; }

    // -----------------------------
    // Matrix multiply
    // -----------------------------
    constexpr m4 operator*(const m4& b) const
    {
        m4 r{};
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                T sum = (T)0;
                for (int k = 0; k < 4; ++k)
                    sum += at(row, k) * b.at(k, col);
                r.at(row, col) = sum;
            }
        }
        return r;
    }

    constexpr m4& operator*=(const m4& b)
    {
        *this = (*this) * b;
        return *this;
    }

    // -----------------------------
    // Matrix * vector
    // -----------------------------
    constexpr v4<T> operator*(const v4<T>& v) const
    {
        return v4<T>{
            at(0, 0)* v.x + at(0, 1) * v.y + at(0, 2) * v.z + at(0, 3) * v.w,
                at(1, 0)* v.x + at(1, 1) * v.y + at(1, 2) * v.z + at(1, 3) * v.w,
                at(2, 0)* v.x + at(2, 1) * v.y + at(2, 2) * v.z + at(2, 3) * v.w,
                at(3, 0)* v.x + at(3, 1) * v.y + at(3, 2) * v.z + at(3, 3) * v.w
        };
    }


    friend constexpr v4<T> operator*(const v4<T>& v, const m4& M)
    {
        return v4<T>{
            v.x* M.at(0, 0) + v.y * M.at(1, 0) + v.z * M.at(2, 0) + v.w * M.at(3, 0),
                v.x* M.at(0, 1) + v.y * M.at(1, 1) + v.z * M.at(2, 1) + v.w * M.at(3, 1),
                v.x* M.at(0, 2) + v.y * M.at(1, 2) + v.z * M.at(2, 2) + v.w * M.at(3, 2),
                v.x* M.at(0, 3) + v.y * M.at(1, 3) + v.z * M.at(2, 3) + v.w * M.at(3, 3)
        };
    }

    // m4 * v3 as a POINT (w=1)
    constexpr v3<T> transformPoint(const v3<T>& p) const
    {
        v4<T> r = (*this) * v4<T>{ p.x, p.y, p.z, (T)1 };

        if constexpr (std::floating_point<T>)
        {
            if (std::fabs((double)r.w) > std::numeric_limits<double>::epsilon())
                return v3<T>{ (T)(r.x / r.w), (T)(r.y / r.w), (T)(r.z / r.w) };
        }

        return v3<T>{ r.x, r.y, r.z };
    }

    // m4 * v3 as a DIRECTION/VECTOR (w=0)
    constexpr v3<T> transformVector(const v3<T>& d) const
    {
        v4<T> r = (*this) * v4<T>{ d.x, d.y, d.z, (T)0 };
        return v3<T>{ r.x, r.y, r.z };
    }

    // -----------------------------
    // Transpose
    // -----------------------------
    constexpr m4 transpose() const
    {
        m4 r{};
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                r.at(row, col) = at(col, row);
        return r;
    }

    // -----------------------------
    // Determinant + inverse
    // (General 4x4 inverse; robust enough for engine use)
    // -----------------------------
    double determinant() const
    {
        // Convert to row/col readable doubles
        const double m00 = (double)at(0, 0), m01 = (double)at(0, 1), m02 = (double)at(0, 2), m03 = (double)at(0, 3);
        const double m10 = (double)at(1, 0), m11 = (double)at(1, 1), m12 = (double)at(1, 2), m13 = (double)at(1, 3);
        const double m20 = (double)at(2, 0), m21 = (double)at(2, 1), m22 = (double)at(2, 2), m23 = (double)at(2, 3);
        const double m30 = (double)at(3, 0), m31 = (double)at(3, 1), m32 = (double)at(3, 2), m33 = (double)at(3, 3);

        const double sub00 = m22 * m33 - m23 * m32;
        const double sub01 = m21 * m33 - m23 * m31;
        const double sub02 = m21 * m32 - m22 * m31;
        const double sub03 = m20 * m33 - m23 * m30;
        const double sub04 = m20 * m32 - m22 * m30;
        const double sub05 = m20 * m31 - m21 * m30;

        return  m00 * (m11 * sub00 - m12 * sub01 + m13 * sub02)
            - m01 * (m10 * sub00 - m12 * sub03 + m13 * sub04)
            + m02 * (m10 * sub01 - m11 * sub03 + m13 * sub05)
            - m03 * (m10 * sub02 - m11 * sub04 + m12 * sub05);
    }

    m4 inverse() const
    {
        // Standard adjugate/cofactor method (expanded); uses double for stability
        double inv[16]{};

        const double m00 = (double)at(0, 0), m01 = (double)at(0, 1), m02 = (double)at(0, 2), m03 = (double)at(0, 3);
        const double m10 = (double)at(1, 0), m11 = (double)at(1, 1), m12 = (double)at(1, 2), m13 = (double)at(1, 3);
        const double m20 = (double)at(2, 0), m21 = (double)at(2, 1), m22 = (double)at(2, 2), m23 = (double)at(2, 3);
        const double m30 = (double)at(3, 0), m31 = (double)at(3, 1), m32 = (double)at(3, 2), m33 = (double)at(3, 3);

        inv[0] = m11 * (m22 * m33 - m23 * m32) - m12 * (m21 * m33 - m23 * m31) + m13 * (m21 * m32 - m22 * m31);
        inv[4] = -m10 * (m22 * m33 - m23 * m32) + m12 * (m20 * m33 - m23 * m30) - m13 * (m20 * m32 - m22 * m30);
        inv[8] = m10 * (m21 * m33 - m23 * m31) - m11 * (m20 * m33 - m23 * m30) + m13 * (m20 * m31 - m21 * m30);
        inv[12] = -m10 * (m21 * m32 - m22 * m31) + m11 * (m20 * m32 - m22 * m30) - m12 * (m20 * m31 - m21 * m30);

        inv[1] = -m01 * (m22 * m33 - m23 * m32) + m02 * (m21 * m33 - m23 * m31) - m03 * (m21 * m32 - m22 * m31);
        inv[5] = m00 * (m22 * m33 - m23 * m32) - m02 * (m20 * m33 - m23 * m30) + m03 * (m20 * m32 - m22 * m30);
        inv[9] = -m00 * (m21 * m33 - m23 * m31) + m01 * (m20 * m33 - m23 * m30) - m03 * (m20 * m31 - m21 * m30);
        inv[13] = m00 * (m21 * m32 - m22 * m31) - m01 * (m20 * m32 - m22 * m30) + m02 * (m20 * m31 - m21 * m30);

        inv[2] = m01 * (m12 * m33 - m13 * m32) - m02 * (m11 * m33 - m13 * m31) + m03 * (m11 * m32 - m12 * m31);
        inv[6] = -m00 * (m12 * m33 - m13 * m32) + m02 * (m10 * m33 - m13 * m30) - m03 * (m10 * m32 - m12 * m30);
        inv[10] = m00 * (m11 * m33 - m13 * m31) - m01 * (m10 * m33 - m13 * m30) + m03 * (m10 * m31 - m11 * m30);
        inv[14] = -m00 * (m11 * m32 - m12 * m31) + m01 * (m10 * m32 - m12 * m30) - m02 * (m10 * m31 - m11 * m30);

        inv[3] = -m01 * (m12 * m23 - m13 * m22) + m02 * (m11 * m23 - m13 * m21) - m03 * (m11 * m22 - m12 * m21);
        inv[7] = m00 * (m12 * m23 - m13 * m22) - m02 * (m10 * m23 - m13 * m20) + m03 * (m10 * m22 - m12 * m20);
        inv[11] = -m00 * (m11 * m23 - m13 * m21) + m01 * (m10 * m23 - m13 * m20) - m03 * (m10 * m21 - m11 * m20);
        inv[15] = m00 * (m11 * m22 - m12 * m21) - m01 * (m10 * m22 - m12 * m20) + m02 * (m10 * m21 - m11 * m20);

        const double det = m00 * inv[0] + m01 * inv[4] + m02 * inv[8] + m03 * inv[12];
        assert(std::fabs(det) > std::numeric_limits<double>::epsilon());

        const double invDet = 1.0 / det;

        m4 r{};
        // remember: at(row,col) maps to a[col][row]
        r.at(0, 0) = (T)(inv[0] * invDet);
        r.at(1, 0) = (T)(inv[1] * invDet);
        r.at(2, 0) = (T)(inv[2] * invDet);
        r.at(3, 0) = (T)(inv[3] * invDet);

        r.at(0, 1) = (T)(inv[4] * invDet);
        r.at(1, 1) = (T)(inv[5] * invDet);
        r.at(2, 1) = (T)(inv[6] * invDet);
        r.at(3, 1) = (T)(inv[7] * invDet);

        r.at(0, 2) = (T)(inv[8] * invDet);
        r.at(1, 2) = (T)(inv[9] * invDet);
        r.at(2, 2) = (T)(inv[10] * invDet);
        r.at(3, 2) = (T)(inv[11] * invDet);

        r.at(0, 3) = (T)(inv[12] * invDet);
        r.at(1, 3) = (T)(inv[13] * invDet);
        r.at(2, 3) = (T)(inv[14] * invDet);
        r.at(3, 3) = (T)(inv[15] * invDet);

        return r;
    }
};

// aliases
using m4f = m4<float>;
using m4d = m4<double>;
using m4i = m4<std::int32_t>;
using m4u = m4<std::uint32_t>;

//  HOW TO USE
//m4f M = m4f::translation({ 1,2,3 }) * m4f::rotationY(m4f::radians(45.f)) * m4f::scale({ 2,2,2 });
//
//v3f pWorld = M.transformPoint(pLocal);
//v3f dWorld = M.transformVector(dLocal);
//
//m4f V = m4f::lookAtRH(eye, target, { 0,1,0 });
//m4f P = m4f::perspectiveRH_OpenGL(m4f::radians(60.f), aspect, 0.1f, 1000.f);
//
//m4f MVP = P * V * M;