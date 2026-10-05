#include "QuadTree.h"
#include <iostream>
#include <algorithm>

QuadTree::QuadNode::QuadNode(const AABB& _bounds, int _depth) 
    :bounds(_bounds), depth(_depth)
{
    for (int i = 0; i < 4; i++) {
        children[i] = nullptr;
    }
}

QuadTree::QuadTree(const AABB& bounds, int cap, int maxD, float _minW, float _minH)
    : capacity(cap), maxDepth(maxD), minW(_minW), minH(_minH)
{
    root = new QuadNode(bounds, 0);
}

QuadTree::~QuadTree() {
    destroySubtree(root);
}

bool QuadTree::insert(Point* p) { 
    if (p == nullptr) return false;
    if (!root->bounds.contains(p->x, p->y)) return false;

    return insertHelper(root, p);
}

// out is appended to to avoid any surprises
void QuadTree::query(const AABB& bounds, std::vector<Point*>& out) const {
    queryHelper(root, bounds, out);
}

// skip merge when points count drops to avoid added complexity
// "eventually consistent" is fine for now
bool QuadTree::remove(Point* p) { 
    if (p == nullptr) return false;

    return removeHelper(root, p); 
}

void QuadTree::clear() {
    AABB bounds = root->bounds;
    destroySubtree(root);
    root = new QuadNode(bounds, 0);
}

bool QuadTree::insertHelper(QuadNode* node, Point* p) {
    if (node->isLeaf()) {
        node->points.push_back(p);
        if ((int)node->points.size() > capacity) subdivide(node);
        return true;
    }
    int q = getQuadrant(node->bounds, p);
    return insertHelper(node->children[q], p);
}

void QuadTree::queryHelper(const QuadNode* node, const AABB& bounds, std::vector<Point*>& out) const {
    if (node == nullptr) return;
    if (!node->bounds.intersects(bounds)) return;
    if (node->isLeaf()) {
        for (auto point : node->points) {
            if (bounds.contains(point->x, point->y)) out.push_back(point);
        }
    }
    else {
        for (auto child : node->children) queryHelper(child, bounds, out);
    }
}

bool QuadTree::removeHelper(QuadNode* node, Point* p) {
    if (node == nullptr) return false;
    if (!node->bounds.contains(p->x, p->y)) return false;

    if (node->isLeaf()) {
        auto it = std::find(node->points.begin(), node->points.end(), p);
        if (it == node->points.end()) return false;
        node->points.erase(it);
        return true;
    }
    else {
        int q = getQuadrant(node->bounds, p);
        return removeHelper(node->children[q], p);
    }
}

void QuadTree::subdivide(QuadNode* node) {
    if (node->bounds.w  < minW || node->bounds.h < minH) return;
    if (node->depth >= maxDepth) return;

    AABB childrenBounds[4];
    float xPos = node->bounds.x;
    float middleXPos = node->bounds.x + (node->bounds.w / 2);
    float yPos = node->bounds.y;
    float middleYPos = node->bounds.y + (node->bounds.h / 2);

    // in SFML, y grows downwards
    childrenBounds[0] = { xPos, yPos,node->bounds.w / 2, node->bounds.h / 2};
    childrenBounds[1] = { middleXPos, yPos, node->bounds.w / 2, node->bounds.h / 2};
    childrenBounds[2] = { xPos, middleYPos, node->bounds.w / 2, node->bounds.h / 2};
    childrenBounds[3] = { middleXPos, middleYPos, node->bounds.w / 2, node->bounds.h / 2};

    for(int i = 0; i < 4; i++) {
        node->children[i] = new QuadNode(childrenBounds[i], node->depth + 1);
    }

    for (Point* p : node->points) {
        int q = getQuadrant(node->bounds, p);
        node->children[q]->points.push_back(p);
    }
    node->points.clear();
}

// post-order traversal: every subtree is visited before node is visited
void QuadTree::destroySubtree(QuadTree::QuadNode* node) {
    if (node == nullptr) return;
    for (auto child : node->children) destroySubtree(child);
    delete node;
}

int QuadTree::getQuadrant(const AABB& bounds, const Point* p) const {
    bool left  = p->x < bounds.x + bounds.w / 2;
    bool above = p->y < bounds.y + bounds.h / 2;   // SFML: smaller y = higher up
    if (left  && above)  return 0;  // NW
    if (!left && above)  return 1;  // NE
    if (left  && !above) return 2;  // SW
    return 3;                       // SE
}