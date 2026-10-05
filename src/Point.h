#pragma once

struct Point {
    float x, y;
    int id;
    inline static int numPoints = 0;

    Point(float _x = 0, float _y = 0) 
        : x(_x), y(_y)
    {
        id = numPoints;
        numPoints++;
    }
};