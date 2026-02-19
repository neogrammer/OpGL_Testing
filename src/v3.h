// IN v3.h
#pragma once
#include <concepts>
#include <type_traits>
#include <cmath>
#include <limits>
#include <iostream>

template<typename T>
struct v3
{
    T x{ (T)0 };
    T y{ (T)0 };
    T z{ (T)0 };

    using value_type = T;

    v3() = default;
    v3(T xin, T yin, T zin) : x{ xin }, y{ yin }, z{ zin } {}

    void print() const
    {
        std::cout
            << "\n__________________________________________"
            << "\nx: " << x
            << "\ny: " << y
            << "\nz: " << z
            << "\n__________________________________________"
            << std::endl;
    }

    template<typename T2>
    using common_t = std::common_type_t<T, T2>;

    // -----------------------------
    // Non-mutating vector ops
    // -----------------------------
    template<typename T2>
    v3 operator+(const v3<T2>& v) const { return v3{ (T)(x + (T)v.x), (T)(y + (T)v.y), (T)(z + (T)v.z) }; }

    template<typename T2>
    v3 operator-(const v3<T2>& v) const { return v3{ (T)(x - (T)v.x), (T)(y - (T)v.y), (T)(z - (T)v.z) }; }

    template<typename T2>
    v3 operator*(const v3<T2>& v) const { return v3{ (T)(x * (T)v.x), (T)(y * (T)v.y), (T)(z * (T)v.z) }; }

    template<typename T2>
    v3 operator/(const v3<T2>& v) const { return v3{ (T)(x / (T)v.x), (T)(y / (T)v.y), (T)(z / (T)v.z) }; }

    v3 operator+() const { return *this; }
    v3 operator-() const { return v3{ (T)-x, (T)-y, (T)-z }; }

    // -----------------------------
    // Scalar ops
    // -----------------------------
    template<typename S> requires std::is_arithmetic_v<S>
    v3 operator*(S s) const { return v3{ (T)(x * (T)s), (T)(y * (T)s), (T)(z * (T)s) }; }

    template<typename S> requires std::is_arithmetic_v<S>
    v3 operator/(S s) const { return v3{ (T)(x / (T)s), (T)(y / (T)s), (T)(z / (T)s) }; }

    template<typename S> requires std::is_arithmetic_v<S>
    friend v3 operator*(S s, const v3& v) { return v * s; }

    // -----------------------------
    // Comparisons
    // -----------------------------
    template<typename T2>
    bool operator==(const v3<T2>& v) const { return x == (T)v.x && y == (T)v.y && z == (T)v.z; }

    template<typename T2>
    bool operator!=(const v3<T2>& v) const { return !(*this == v); }

    // -----------------------------
    // Compound assignment
    // -----------------------------
    template<typename T2>
    v3& operator+=(const v3<T2>& v) { x = (T)(x + (T)v.x); y = (T)(y + (T)v.y); z = (T)(z + (T)v.z); return *this; }

    template<typename T2>
    v3& operator-=(const v3<T2>& v) { x = (T)(x - (T)v.x); y = (T)(y - (T)v.y); z = (T)(z - (T)v.z); return *this; }

    template<typename T2>
    v3& operator*=(const v3<T2>& v) { x = (T)(x * (T)v.x); y = (T)(y * (T)v.y); z = (T)(z * (T)v.z); return *this; }

    template<typename T2>
    v3& operator/=(const v3<T2>& v) { x = (T)(x / (T)v.x); y = (T)(y / (T)v.y); z = (T)(z / (T)v.z); return *this; }

    template<typename S> requires std::is_arithmetic_v<S>
    v3& operator*=(S s) { x = (T)(x * (T)s); y = (T)(y * (T)s); z = (T)(z * (T)s); return *this; }

    template<typename S> requires std::is_arithmetic_v<S>
    v3& operator/=(S s) { x = (T)(x / (T)s); y = (T)(y / (T)s); z = (T)(z / (T)s); return *this; }

    // -----------------------------
    // Named mutating methods
    // -----------------------------
    template<typename T2> v3& add(const v3<T2>& v) { return (*this += v); }
    template<typename T2> v3& sub(const v3<T2>& v) { return (*this -= v); }
    template<typename T2> v3& mul(const v3<T2>& v) { return (*this *= v); }
    template<typename T2> v3& div(const v3<T2>& v) { return (*this /= v); }

    template<typename S> requires std::is_arithmetic_v<S>
    v3& mul(S s) { return (*this *= s); }

    template<typename S> requires std::is_arithmetic_v<S>
    v3& div(S s) { return (*this /= s); }

    // -----------------------------
    // Math
    // -----------------------------
    double magnitude() const
    {
        return std::sqrt((double)x * (double)x + (double)y * (double)y + (double)z * (double)z);
    }

    template<typename T2>
    common_t<T2> dot(const v3<T2>& v) const
    {
        using R = common_t<T2>;
        return (R)x * (R)v.x + (R)y * (R)v.y + (R)z * (R)v.z;
    }

    // Standard 3D cross product (returns common type)
    template<typename T2>
    v3<common_t<T2>> cross(const v3<T2>& v) const
    {
        using R = common_t<T2>;
        return v3<R>{
            (R)y* (R)v.z - (R)z * (R)v.y,
                (R)z* (R)v.x - (R)x * (R)v.z,
                (R)x* (R)v.y - (R)y * (R)v.x
        };
    }

    // Normalize only for floating point vectors
    v3& normalize() requires std::floating_point<T>
    {
        double mag = magnitude();
        if (mag <= std::numeric_limits<double>::epsilon())
        {
            x = (T)0; y = (T)0; z = (T)0;
            return *this;
        }

        x = (T)((double)x / mag);
        y = (T)((double)y / mag);
        z = (T)((double)z / mag);
        return *this;
    }

    using normalized_type = std::conditional_t<std::floating_point<T>, T, double>;

    v3<normalized_type> normalized() const
    {
        v3<normalized_type> out{ (normalized_type)x, (normalized_type)y, (normalized_type)z };
        out.normalize();
        return out;
    }
};

using v3f = v3<float>;
using v3d = v3<double>;
using v3i = v3<std::int32_t>;
using v3u = v3<std::uint32_t>;