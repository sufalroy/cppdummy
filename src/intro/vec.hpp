#pragma once

#include <cmath>

struct vec2 
{
    float x, y;

    constexpr vec2() noexcept : x(0.0f), y(0.0f) {}
    constexpr vec2(float x_, float y_) noexcept : x(x_), y(y_) {}
    constexpr explicit vec2(float scalar) noexcept : x(scalar), y(scalar) {}

    constexpr vec2(const vec2&) noexcept = default;
    constexpr vec2& operator=(const vec2&) noexcept = default;

    constexpr float& operator[](int index) noexcept { 
        return index == 0 ? x : y; 
    }
    constexpr const float& operator[](int index) const noexcept { 
        return index == 0 ? x : y; 
    }

    constexpr vec2 operator+(const vec2& other) const noexcept {
        return vec2(x + other.x, y + other.y);
    }
    
    constexpr vec2 operator-(const vec2& other) const noexcept {
        return vec2(x - other.x, y - other.y);
    }
    
    constexpr vec2 operator*(float scalar) const noexcept {
        return vec2(x * scalar, y * scalar);
    }
    
    constexpr vec2 operator*(const vec2& other) const noexcept {
        return vec2(x * other.x, y * other.y);
    }
    
    constexpr vec2 operator/(float scalar) const noexcept {
        return vec2(x / scalar, y / scalar);
    }

    constexpr vec2& operator+=(const vec2& other) noexcept {
        x += other.x; y += other.y; return *this;
    }
    
    constexpr vec2& operator-=(const vec2& other) noexcept {
        x -= other.x; y -= other.y; return *this;
    }
    
    constexpr vec2& operator*=(float scalar) noexcept {
        x *= scalar; y *= scalar; return *this;
    }
    
    constexpr vec2& operator/=(float scalar) noexcept {
        x /= scalar; y /= scalar; return *this;
    }

    constexpr vec2 operator-() const noexcept {
        return vec2(-x, -y);
    }

    constexpr bool operator==(const vec2& other) const noexcept {
        return x == other.x && y == other.y;
    }
    
    constexpr bool operator!=(const vec2& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] constexpr float dot(const vec2& other) const noexcept {
        return x * other.x + y * other.y;
    }
    
    [[nodiscard]] float length() const noexcept {
        return std::sqrt(x * x + y * y);
    }
    
    [[nodiscard]] constexpr float length_squared() const noexcept {
        return x * x + y * y;
    }
    
    [[nodiscard]] vec2 normalized() const noexcept {
        const float len = length();
        return len > 0.0f ? *this / len : vec2(0.0f);
    }
    
    void normalize() noexcept {
        *this = normalized();
    }
};

constexpr vec2 operator*(float scalar, const vec2& v) noexcept {
    return v * scalar;
}

constexpr float dot(const vec2& a, const vec2& b) noexcept {
    return a.dot(b);
}

inline float length(const vec2& v) noexcept {
    return v.length();
}

inline vec2 normalize(const vec2& v) noexcept {
    return v.normalized();
}

struct vec3 
{
    float x, y, z;

    constexpr vec3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}
    constexpr explicit vec3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}
    constexpr vec3(const vec2& v, float z_) noexcept : x(v.x), y(v.y), z(z_) {}

    constexpr vec3(const vec3&) noexcept = default;
    constexpr vec3& operator=(const vec3&) noexcept = default;

    constexpr float& operator[](int index) noexcept { 
        return index == 0 ? x : (index == 1 ? y : z);
    }
    constexpr const float& operator[](int index) const noexcept { 
        return index == 0 ? x : (index == 1 ? y : z);
    }

    constexpr vec2 xy() const noexcept { return vec2(x, y); }
    constexpr vec2 xz() const noexcept { return vec2(x, z); }
    constexpr vec2 yz() const noexcept { return vec2(y, z); }

    constexpr vec3 operator+(const vec3& other) const noexcept {
        return vec3(x + other.x, y + other.y, z + other.z);
    }
    
    constexpr vec3 operator-(const vec3& other) const noexcept {
        return vec3(x - other.x, y - other.y, z - other.z);
    }
    
    constexpr vec3 operator*(float scalar) const noexcept {
        return vec3(x * scalar, y * scalar, z * scalar);
    }
    
    constexpr vec3 operator*(const vec3& other) const noexcept {
        return vec3(x * other.x, y * other.y, z * other.z);
    }
    
    constexpr vec3 operator/(float scalar) const noexcept {
        return vec3(x / scalar, y / scalar, z / scalar);
    }

    constexpr vec3& operator+=(const vec3& other) noexcept {
        x += other.x; y += other.y; z += other.z; return *this;
    }
    
    constexpr vec3& operator-=(const vec3& other) noexcept {
        x -= other.x; y -= other.y; z -= other.z; return *this;
    }
    
    constexpr vec3& operator*=(float scalar) noexcept {
        x *= scalar; y *= scalar; z *= scalar; return *this;
    }
    
    constexpr vec3& operator/=(float scalar) noexcept {
        x /= scalar; y /= scalar; z /= scalar; return *this;
    }

    constexpr vec3 operator-() const noexcept {
        return vec3(-x, -y, -z);
    }

    constexpr bool operator==(const vec3& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
    
    constexpr bool operator!=(const vec3& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] constexpr float dot(const vec3& other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }
    
    [[nodiscard]] constexpr vec3 cross(const vec3& other) const noexcept {
        return vec3(y * other.z - z * other.y,
                   z * other.x - x * other.z,
                   x * other.y - y * other.x);
    }
    
    [[nodiscard]] float length() const noexcept {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    [[nodiscard]] constexpr float length_squared() const noexcept {
        return x * x + y * y + z * z;
    }
    
    [[nodiscard]] vec3 normalized() const noexcept {
        const float len = length();
        return len > 0.0f ? *this / len : vec3(0.0f);
    }
    
    void normalize() noexcept {
        *this = normalized();
    }
};

constexpr vec3 operator*(float scalar, const vec3& v) noexcept {
    return v * scalar;
}

constexpr float dot(const vec3& a, const vec3& b) noexcept {
    return a.dot(b);
}

constexpr vec3 cross(const vec3& a, const vec3& b) noexcept {
    return a.cross(b);
}

inline float length(const vec3& v) noexcept {
    return v.length();
}

inline vec3 normalize(const vec3& v) noexcept {
    return v.normalized();
}