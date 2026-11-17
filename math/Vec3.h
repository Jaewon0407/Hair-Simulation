#pragma once

#include <cmath>

struct Vec3 {

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 operator*(const Vec3& other) const {
        return Vec3(x * other.x, y * other.y, z * other.z);
    }

    Vec3 operator/(const Vec3& other) const {
        return Vec3(x / other.x, y / other.y, z / other.z);
    }

    Vec3& operator+=(const Vec3& other) {
        this->x += other.x;
        this->y += other.y;
        this->z += other.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& other) {
        this->x -= other.x;
        this->y -= other.y;
        this->z -= other.z;
        return *this;
    }

    Vec3& operator*=(const Vec3& other) {
        this->x *= other.x;
        this->y *= other.y;
        this->z *= other.z;
        return *this;
    }

    Vec3& operator/=(const Vec3& other) {
        this->x /= other.x;
        this->y /= other.y;
        this->z /= other.z;
        return *this;
    }

    Vec3 operator+(float scalar) const {
        return Vec3(x + scalar, y + scalar, z + scalar);
    }

    Vec3 operator-(float scalar) const {
        return Vec3(x - scalar, y - scalar, z - scalar);
    }

    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    Vec3 operator/(float scalar) const {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }

    Vec3& operator+=(float scalar) {
        this->x += scalar;
        this->y += scalar;
        this->z += scalar;
        return *this;
    }

    Vec3& operator-=(float scalar) {
        this->x -= scalar;
        this->y -= scalar;
        this->z -= scalar;
        return *this;
    }

    Vec3& operator*=(float scalar) {
        this->x *= scalar;
        this->y *= scalar;
        this->z *= scalar;
        return *this;
    }

    Vec3& operator/=(float scalar) {
        this->x /= scalar;
        this->y /= scalar;
        this->z /= scalar;
        return *this;
    }

};

inline Vec3 operator*(float scalar, const Vec3& v) {
    return v * scalar;
}

inline Vec3 operator-(const Vec3& v) {
    return Vec3(-v.x, -v.y, -v.z);
}

inline float dot(const Vec3& v1, const Vec3& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vec3 cross(const Vec3& v1, const Vec3& v2) {
    return Vec3(v1.y * v2.z - v1.z * v2.y,
                v1.z * v2.x - v1.x * v2.z,
                v1.x * v2.y - v1.y * v2.x);
}

inline float lengthSquared(const Vec3& v) {
    return dot(v, v);
}

inline float length(const Vec3& v) {
    return std::sqrt(lengthSquared(v));
}

inline Vec3 normalized(const Vec3& v) {
    const float len = length(v);
    return len > 0.0f ? v / len : Vec3();
}