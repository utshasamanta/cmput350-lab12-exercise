#ifndef VECTOR_4_H
#define VECTOR_4_H

#include <cassert>
#include <ostream>

struct Vector4f {
    Vector4f& operator+=(const Vector4f& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    Vector4f operator+(const Vector4f& other) const {
        Vector4f ret(*this);
        return ret += other;
    }

    Vector4f& operator-=(const Vector4f& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    Vector4f operator-(const Vector4f& other) const {
        Vector4f ret(*this);
        return ret -= other;
    }

    Vector4f& operator*=(const Vector4f& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    Vector4f operator*(const Vector4f& other) const {
        Vector4f ret(*this);
        return ret *= other;
    }

    Vector4f& operator/=(const Vector4f& other) {
        assert(other.x != 0.f);
        assert(other.y != 0.f);
        assert(other.z != 0.f);
        assert(other.w != 0.f);
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }

    Vector4f operator/(const Vector4f& other) const {
        Vector4f ret(*this);
        return ret /= other;
    }

    Vector4f& operator+=(float scalar) {
        x += scalar;
        y += scalar;
        z += scalar;
        w += scalar;
        return *this;
    }

    Vector4f operator+(float scalar) const {
        Vector4f ret(*this);
        return ret += scalar;
    }

    Vector4f& operator-=(float scalar) {
        x -= scalar;
        y -= scalar;
        z -= scalar;
        w -= scalar;
        return *this;
    }

    Vector4f operator-(float scalar) const {
        Vector4f ret(*this);
        return ret -= scalar;
    }

    Vector4f& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    Vector4f operator*(float scalar) const {
        Vector4f ret(*this);
        return ret *= scalar;
    }

    Vector4f& operator/=(float scalar) {
        assert(scalar != 0.f);
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    Vector4f operator/(float scalar) const {
        Vector4f ret(*this);
        return ret /= scalar;
    }

    Vector4f operator-() const {
        Vector4f ret(*this);
        ret.x = -ret.x;
        ret.y = -ret.y;
        ret.z = -ret.z;
        ret.w = -ret.w;
        return ret;
    }

    bool operator==(const Vector4f& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    float x;
    float y;
    float z;
    float w;
};

inline Vector4f operator*(float scalar, const Vector4f& v) { return v * scalar; }

inline std::ostream& operator<<(std::ostream& os, const Vector4f& v) {
    return os << "Vector4{ " << v.x << ", " << v.y << ", " << v.z << ", " << v.w << " }";
}

#endif  // VECTOR_4_H
