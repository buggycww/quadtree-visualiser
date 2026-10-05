#pragma once

// Axis-aligned bounding box. Non-rotated rectangle.
// Boundary convention: half-open intervals [x, x+w) × [y, y+h).
//   - Left and top edges are inside.
//   - Right and bottom edges are outside.
// This makes sub-quadrants tile their parent perfectly, so every
// point in a quadtree descends to exactly one leaf.

struct AABB { // mostly data & simple helpers, so make struct
    float x, y, w, h;

    bool contains(float px, float py) const {
        return px >= x && px < x + w &&
                py >= y && py <= y + h;
    }

    bool intersects(const AABB& other) const {
        return x < other.x + other.w && x + w > other.x &&
                y < other.y + other.h && y + h > other.y;
    }
};
