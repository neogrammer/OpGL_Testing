// IN v2.h
#pragma once
#include <concepts>
#include <cmath>
#include <limits>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <type_traits>

template<typename T>
struct v2
{
    T x{ (T)0 };
    T y{ (T)0 };

    using value_type = T;

    v2() = default;
    v2(T xin, T yin) : x{ xin }, y{ yin } {}

    // -----------------------------
    // Printing
    // -----------------------------
    void print() const
    {
        std::cout
            << "\n__________________________________________"
            << "\nx: " << x
            << "\ny: " << y
            << "\n__________________________________________"
            << std::endl;
    }

    // -----------------------------
    // Helper: common numeric type
    // -----------------------------
    template<typename T2>
    using common_t = std::common_type_t<T, T2>;

    // -----------------------------
    // Non-mutating vector ops (component-wise)
    // -----------------------------
    template<typename T2>
    v2 operator+(const v2<T2>& v) const
    {
        return v2{ (T)(x + (T)v.x), (T)(y + (T)v.y) };
    }

    template<typename T2>
    v2 operator-(const v2<T2>& v) const
    {
        return v2{ (T)(x - (T)v.x), (T)(y - (T)v.y) };
    }

    template<typename T2>
    v2 operator*(const v2<T2>& v) const
    {
        return v2{ (T)(x * (T)v.x), (T)(y * (T)v.y) };
    }

    template<typename T2>
    v2 operator/(const v2<T2>& v) const
    {
        return v2{ (T)(x / (T)v.x), (T)(y / (T)v.y) };
    }

    // Unary
    v2 operator+() const { return *this; }
    v2 operator-() const { return v2{ (T)-x, (T)-y }; }

    // -----------------------------
    // Scalar ops (vector * scalar, etc.)
    // -----------------------------
    template<typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    v2 operator*(S s) const
    {
        return v2{ (T)(x * (T)s), (T)(y * (T)s) };
    }

    template<typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    v2 operator/(S s) const
    {
        return v2{ (T)(x / (T)s), (T)(y / (T)s) };
    }

    // Allow scalar * vector
    template<typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    friend v2 operator*(S s, const v2& v)
    {
        return v * s;
    }

    // -----------------------------
    // Comparisons
    // -----------------------------
    template<typename T2>
    bool operator==(const v2<T2>& v) const
    {
        return x == (T)v.x && y == (T)v.y;
    }

    template<typename T2>
    bool operator!=(const v2<T2>& v) const
    {
        return !(*this == v);
    }

    // -----------------------------
    // Compound assignment operators
    // -----------------------------
    template<typename T2>
    v2& operator+=(const v2<T2>& v) { x = (T)(x + (T)v.x); y = (T)(y + (T)v.y); return *this; }

    template<typename T2>
    v2& operator-=(const v2<T2>& v) { x = (T)(x - (T)v.x); y = (T)(y - (T)v.y); return *this; }

    template<typename T2>
    v2& operator*=(const v2<T2>& v) { x = (T)(x * (T)v.x); y = (T)(y * (T)v.y); return *this; }

    template<typename T2>
    v2& operator/=(const v2<T2>& v) { x = (T)(x / (T)v.x); y = (T)(y / (T)v.y); return *this; }

    template<typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    v2& operator*=(S s) { x = (T)(x * (T)s); y = (T)(y * (T)s); return *this; }

    template<typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    v2& operator/=(S s) { x = (T)(x / (T)s); y = (T)(y / (T)s); return *this; }

    // -----------------------------
    // Named mutating methods (chainable)
    // -----------------------------
    template<typename T2>
    v2& add(const v2<T2>& v) { return (*this += v); }

    template<typename T2>
    v2& sub(const v2<T2>& v) { return (*this -= v); }

    template<typename T2>
    v2& mul(const v2<T2>& v) { return (*this *= v); }

    template<typename T2>
    v2& div(const v2<T2>& v) { return (*this /= v); }

    template<typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    v2& mul(S s) { return (*this *= s); }

    template<typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    v2& div(S s) { return (*this /= s); }

    // -----------------------------
    // Math (non-mutating)
    // -----------------------------
    // magnitude as double for stability (works for ints too)
    double magnitude() const
    {
        return std::sqrt((double)x * (double)x + (double)y * (double)y);
    }

    // direction in degrees: 0° at +X axis, CCW positive
    double direction() const
    {
        double angRad = std::atan2((double)y, (double)x); // [-pi, pi]
        double angDeg = angRad * (180.0 / 3.14159265358979323846);
        if (angDeg < 0.0) angDeg += 360.0;
        return angDeg;
    }

    template<typename T2>
    common_t<T2> dot(const v2<T2>& v) const
    {
        using R = common_t<T2>;
        return (R)x * (R)v.x + (R)y * (R)v.y;
    }

    // -----------------------------
    // Normalize (mutating)
    // -----------------------------
    v2& normalize() requires std::floating_point<T>
    {
        double mag = magnitude();
        if (mag <= std::numeric_limits<double>::epsilon())
        {
            x = (T)0; y = (T)0;
            return *this;
        }

        // For integral T, you probably *don't* want normalize().
        // This keeps behavior defined, but will truncate.
        x = (T)((double)x / mag);
        y = (T)((double)y / mag);
        return *this;
    }
    // Return same type if already floating, otherwise return double
    using normalized_type = std::conditional_t<std::floating_point<T>, T, double>;

    v2<normalized_type> normalized() const
    {
        v2<normalized_type> out{ (normalized_type)x, (normalized_type)y };
        out.normalize(); // ok because normalized_type is floating
        return out;
    }
};

// Aliases
using v2f = v2<float>;
using v2d = v2<double>;
using v2u = v2<std::uint32_t>;
using v2i = v2<std::int32_t>;
using v2i8 = v2<std::int8_t>;
using v2u8 = v2<std::uint8_t>;
using v2s = v2<std::size_t>;