#include "vector3f.h"
#include <cmath>
#include <iostream>

float vector3f::inverse_rsqrt(float number) const
{
    //! don't ask. watch https://www.youtube.com/watch?v=p8u_k2LIZyo
    const float threehalfs = 1.5F;
    float x2 = number * 0.5F;
    float y = number;
    long i = *(long *)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *)&i;
    y = y * (threehalfs - (x2 * y * y));
    y = y * (threehalfs - (x2 * y * y));
    return y;
}

vector3f::vector3f(double x, double y, double z)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = 1;
}
vector3f::vector3f(const std::vector<double> &v)
{
    x = v[0];
    y = v[1];
    z = v[2];
    w = 1;
}

vector3f::vector3f(const vector3f &v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
}

double vector3f::length() const
{
    return sqrt(x * x + y * y + z * z);
}

double vector3f::length_squared() const
{
    return x * x + y * y + z * z;
}

vector3f vector3f::normalize() const
{
    // double l = length();
    // return vector3f(x / l, y / l, z / l);
    double l = inverse_rsqrt(x * x + y * y + z * z);
    return vector3f(x * l, y * l, z * l);
}

vector3f vector3f::cross(const vector3f &v) const
{
    return vector3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}

double vector3f::dot(const vector3f &v) const
{
    return x * v.x + y * v.y + z * v.z;
}

vector3f vector3f::rotate(double angle, const vector3f &axis)
{
    // rotate vector about axis by angle
    // Rodrigues' rotation formula
    vector3f v = *this;
    vector3f a = axis.normalize();
    double cosa = cos(angle) > epsilon ? cos(angle) : 0;
    vector3f v_rot = v * cosa + a.cross(v) * sin(angle) + a * (a.dot(v)) * (1 - cosa);
    return v_rot;
}

bool vector3f::near_zero() const
{
    const double s = 1e-8;
    return (fabs(x) < s) && (fabs(y) < s) && (fabs(z) < s);
}

vector3f vector3f::operator+(const vector3f &v) const
{
    return vector3f(x + v.x, y + v.y, z + v.z);
}

vector3f vector3f::operator-(const vector3f &v) const
{
    return vector3f(x - v.x, y - v.y, z - v.z);
}

vector3f vector3f::operator*(double scalar) const
{
    return vector3f(x * scalar, y * scalar, z * scalar);
}

vector3f vector3f::operator/(double scalar) const
{
    return vector3f(x / scalar, y / scalar, z / scalar);
}

vector3f vector3f::operator/(const vector3f &v) const
{
    return vector3f(x / v.x, y / v.y, z / v.z);
}

vector3f vector3f::operator=(const vector3f &v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
}

vector3f vector3f::operator+=(const vector3f &v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

vector3f vector3f::operator-=(const vector3f &v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

vector3f vector3f::scale(double sx, double sy, double sz)
{
    x *= sx;
    y *= sy;
    z *= sz;
    return *this;
}
vector3f vector3f::scale(double s)
{
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

std::ostream &operator<<(std::ostream &os, vector3f v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
