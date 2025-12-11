#pragma once
#include "Maths.h"
#include <cmath>
#include <cfloat>
#include <algorithm>

inline Vec3 MaxVec(const Vec3& a, const Vec3& b) {
    return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline Vec3 MinVec(const Vec3& a, const Vec3& b) {
    return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

struct Ray
{
    Vec3 o;
    Vec3 dir;
    Vec3 invdir;

    Ray() {}
    Ray(const Vec3 _o, const Vec3 _dir) {
        init(_o, _dir);
    }
    void init(const Vec3 _o, const Vec3 _dir) {
        o = _o;
        dir = _dir;
        invdir = Vec3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
    }
    Vec3 at(const float t) const {
        return (o + (dir * t));
    }
};

class AABB
{
public:
    Vec3 max;
    Vec3 min;

    AABB() { reset(); }

    AABB(const Vec3& _min, const Vec3& _max) {
        min = _min;
        max = _max;
    }

    void reset()
    {
        max = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        min = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    }

    void extend(const Vec3& p)
    {
        max = MaxVec(max, p);
        min = MinVec(min, p);
    }

    static bool check(const AABB& a, const AABB& b)
    {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
            (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
            (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }

    bool rayAABB(const Ray& r, float& t) const
    {
        float t1 = (min.x - r.o.x) * r.invdir.x;
        float t2 = (max.x - r.o.x) * r.invdir.x;
        float t3 = (min.y - r.o.y) * r.invdir.y;
        float t4 = (max.y - r.o.y) * r.invdir.y;
        float t5 = (min.z - r.o.z) * r.invdir.z;
        float t6 = (max.z - r.o.z) * r.invdir.z;

        float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
        float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

        if (tmax < 0 || tmin > tmax) {
            t = tmax;
            return false;
        }

        t = tmin;
        return true;
    }
};