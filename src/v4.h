// IN v4.h
#pragma once
#include <concepts>
#include <type_traits>
#include <cmath>
#include <limits>
#include <iostream>
#include "v3.h" // for returning v3 if you want later; not strictly required

template<typename T>
struct v4
{
    T x{ (T)0 };
    T y{ (T)0 };
    T z{ (T)0 };
    T w{ (T)0 };

    using value_type = T;

    v4() = default;
    v4(T xin, T yin, T zin, T win) : x{ xin }, y{ yin }, z{ zin }, w{ win } {}

    void print() const
    {
        std::cout
            << "\n__________________________________________"
            << "\nx: " << x
            << "\ny: " << y
            << "\nz: " << z
            << "\nw: " << w
            << "\n__________________________________________"
            << std::endl;
    }

    template<typename T2>
    using common_t = std::common_type_t<T, T2>;

    // -----------------------------
    // Non-mutating vector ops
    // -----------------------------
    template<typename T2>
    v4 operator+(const v4<T2>& v) const { return v4{ (T)(x + (T)v.x), (T)(y + (T)v.y), (T)(z + (T)v.z), (T)(w + (T)v.w) }; }

    template<typename T2>
    v4 operator-(const v4<T2>& v) const { return v4{ (T)(x - (T)v.x), (T)(y - (T)v.y), (T)(z - (T)v.z), (T)(w - (T)v.w) }; }

    template<typename T2>
    v4 operator*(const v4<T2>& v) const { return v4{ (T)(x * (T)v.x), (T)(y * (T)v.y), (T)(z * (T)v.z), (T)(w * (T)v.w) }; }

    template<typename T2>
    v4 operator/(const v4<T2>& v) const { return v4{ (T)(x / (T)v.x), (T)(y / (T)v.y), (T)(z / (T)v.z), (T)(w / (T)v.w) }; }

    v4 operator+() const { return *this; }
    v4 operator-() const { return v4{ (T)-x, (T)-y, (T)-z, (T)-w }; }

    // -----------------------------
    // Scalar ops
    // -----------------------------
    template<typename S> requires std::is_arithmetic_v<S>
    v4 operator*(S s) const { return v4{ (T)(x * (T)s), (T)(y * (T)s), (T)(z * (T)s), (T)(w * (T)s) }; }

    template<typename S> requires std::is_arithmetic_v<S>
    v4 operator/(S s) const { return v4{ (T)(x / (T)s), (T)(y / (T)s), (T)(z / (T)s), (T)(w / (T)s) }; }

    template<typename S> requires std::is_arithmetic_v<S>
    friend v4 operator*(S s, const v4& v) { return v * s; }

    // -----------------------------
    // Comparisons
    // -----------------------------
    template<typename T2>
    bool operator==(const v4<T2>& v) const { return x == (T)v.x && y == (T)v.y && z == (T)v.z && w == (T)v.w; }

    template<typename T2>
    bool operator!=(const v4<T2>& v) const { return !(*this == v); }

    // -----------------------------
    // Compound assignment
    // -----------------------------
    template<typename T2>
    v4& operator+=(const v4<T2>& v) { x = (T)(x + (T)v.x); y = (T)(y + (T)v.y); z = (T)(z + (T)v.z); w = (T)(w + (T)v.w); return *this; }

    template<typename T2>
    v4& operator-=(const v4<T2>& v) { x = (T)(x - (T)v.x); y = (T)(y - (T)v.y); z = (T)(z - (T)v.z); w = (T)(w - (T)v.w); return *this; }

    template<typename T2>
    v4& operator*=(const v4<T2>& v) { x = (T)(x * (T)v.x); y = (T)(y * (T)v.y); z = (T)(z * (T)v.z); w = (T)(w * (T)v.w); return *this; }

    template<typename T2>
    v4& operator/=(const v4<T2>& v) { x = (T)(x / (T)v.x); y = (T)(y / (T)v.y); z = (T)(z / (T)v.z); w = (T)(w / (T)v.w); return *this; }

    template<typename S> requires std::is_arithmetic_v<S>
    v4& operator*=(S s) { x = (T)(x * (T)s); y = (T)(y * (T)s); z = (T)(z * (T)s); w = (T)(w * (T)s); return *this; }

    template<typename S> requires std::is_arithmetic_v<S>
    v4& operator/=(S s) { x = (T)(x / (T)s); y = (T)(y / (T)s); z = (T)(z / (T)s); w = (T)(w / (T)s); return *this; }

    // -----------------------------
    // Named mutating methods
    // -----------------------------
    template<typename T2> v4& add(const v4<T2>& v) { return (*this += v); }
    template<typename T2> v4& sub(const v4<T2>& v) { return (*this -= v); }
    template<typename T2> v4& mul(const v4<T2>& v) { return (*this *= v); }
    template<typename T2> v4& div(const v4<T2>& v) { return (*this /= v); }

    template<typename S> requires std::is_arithmetic_v<S>
    v4& mul(S s) { return (*this *= s); }

    template<typename S> requires std::is_arithmetic_v<S>
    v4& div(S s) { return (*this /= s); }

    // -----------------------------
    // Math
    // -----------------------------
    double magnitude() const
    {
        return std::sqrt(
            (double)x * (double)x +
            (double)y * (double)y +
            (double)z * (double)z +
            (double)w * (double)w
        );
    }

    template<typename T2>
    common_t<T2> dot(const v4<T2>& v) const
    {
        using R = common_t<T2>;
        return (R)x * (R)v.x + (R)y * (R)v.y + (R)z * (R)v.z + (R)w * (R)v.w;
    }

    // "Cross" for v4 in the common graphics sense:
    // treat vectors as 3D directions in xyz, ignore w, set w = 0.
    template<typename T2>
    v4<common_t<T2>> cross(const v4<T2>& v) const
    {
        using R = common_t<T2>;
        return v4<R>{
            (R)y* (R)v.z - (R)z * (R)v.y,
                (R)z* (R)v.x - (R)x * (R)v.z,
                (R)x* (R)v.y - (R)y * (R)v.x,
                (R)0
        };
    }

    v4& normalize() requires std::floating_point<T>
    {
        double mag = magnitude();
        if (mag <= std::numeric_limits<double>::epsilon())
        {
            x = (T)0; y = (T)0; z = (T)0; w = (T)0;
            return *this;
        }

        x = (T)((double)x / mag);
        y = (T)((double)y / mag);
        z = (T)((double)z / mag);
        w = (T)((double)w / mag);
        return *this;
    }

    using normalized_type = std::conditional_t<std::floating_point<T>, T, double>;

    v4<normalized_type> normalized() const
    {
        v4<normalized_type> out{ (normalized_type)x, (normalized_type)y, (normalized_type)z, (normalized_type)w };
        out.normalize();
        return out;
    }
};

using v4f = v4<float>;
using v4d = v4<double>;
using v4i = v4<std::int32_t>;
using v4u = v4<std::uint32_t>;