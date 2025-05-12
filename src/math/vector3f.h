#pragma once

#include <cmath>
#include <iostream>
#include <vector>

#define PI acos(-1)
#define epsilon 1e-09

class vector3f
{
private:
    float inverse_rsqrt(float number) const;

public:
    double x, y, z, w;
    vector3f(double x = 0, double y = 0, double z = 0);
    vector3f(const std::vector<double> &v);
    vector3f(const vector3f &v);
    double length() const;
    double length_squared() const;
    vector3f normalize() const;
    vector3f cross(const vector3f &v) const;
    double dot(const vector3f &v) const;
    vector3f rotate(double angle, const vector3f &axis);
    vector3f scale(double sx, double sy, double sz);
    vector3f scale(double s);
    bool near_zero() const;
    vector3f operator+(const vector3f &v) const;
    vector3f operator-(const vector3f &v) const;
    vector3f operator*(double scalar) const;
    vector3f operator/(double scalar) const;
    vector3f operator/(const vector3f &v) const;
    vector3f operator=(const vector3f &v);
    vector3f operator+=(const vector3f &v);
    vector3f operator-=(const vector3f &v);

    friend std::ostream &operator<<(std::ostream &os, vector3f v);
};
