#pragma once
#include "Maths.h"
#include <cmath>

struct AABB {
    Vec3 position; 
    Vec3 size;      

    static bool check(AABB a, AABB b)
    {
        bool collisionX = (a.position.x - b.position.x) < (a.size.x / 2 + b.size.x / 2) &&
            (a.position.x - b.position.x) > -(a.size.x / 2 + b.size.x / 2);

        bool collisionY = (a.position.y - b.position.y) < (a.size.y / 2 + b.size.y / 2) &&
            (a.position.y - b.position.y) > -(a.size.y / 2 + b.size.y / 2);

        bool collisionZ = (a.position.z - b.position.z) < (a.size.z / 2 + b.size.z / 2) &&
            (a.position.z - b.position.z) > -(a.size.z / 2 + b.size.z / 2);

        return collisionX && collisionY && collisionZ;
    }

};