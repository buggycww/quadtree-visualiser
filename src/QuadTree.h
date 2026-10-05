#pragma once
#include "AABB.h"
#include "Point.h"
#include <vector>

/*
every node is either a leaf (has points, no children) or an
internal node (has children, no points)
all points must be pushed down to leaf nodes

quadtree does not own points
its focus is on spatial indexing, not memory management
points should belong to the visualiser that creates it

subdivde is ignored when called on a node with maxdepth (prevents infinite recursions on clustered nodes)
disable copy constructer to prevent shallow copy, since it uses pointers
when clear is called, delete root and create new one. more consistent and cleaner
*/

class QuadTree {
    public:
        struct QuadNode {
            AABB bounds;
            std::vector<Point*> points;
            QuadNode* children[4];
            int depth;

            QuadNode(const AABB& _bounds, int _depth);
            bool isLeaf() const { return children[0] == nullptr; }
        };

        QuadTree(const AABB& _bounds, int _capacity, int _maxDepth, float _minW = 1, float _minH = 1);
        ~QuadTree();

        QuadTree(const QuadTree&) = delete;
        QuadTree& operator= (const QuadTree&) = delete;

        bool insert(Point* p);
        void query(const AABB& bounds, std::vector<Point*>& out) const;
        bool remove(Point *p);
        void clear();
        const QuadNode* getRoot() const { return root; }
        int getCapacity() const { return capacity; }
        int getMaxDepth() const { return maxDepth; }

    private:
        QuadNode* root;
        int capacity; // max points a node can hold before it subdivides
        int maxDepth; // max depth of a node
        float minW;
        float minH; // minumims to guard against dividing an excessively small point

        bool insertHelper(QuadNode* node, Point* p);
        void queryHelper(const QuadNode* node, const AABB& bounds, std::vector<Point*>& out) const;
        bool removeHelper(QuadNode* node, Point* p);
        void subdivide(QuadNode* node);
        void destroySubtree(QuadNode* node);
        int getQuadrant(const AABB& bounds, const Point* p) const;
};