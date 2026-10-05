#include <iostream>
#include <cassert>

#include "AABB.h"
#include "Point.h"
#include "QuadTree.h"

void TestAABB() {
    AABB box{ 0, 0, 100, 100 };

    // contains
    assert( box.contains(50, 50));      // dead center
    assert( box.contains(0, 0));        // top-left (inside, per half-open)
    assert( box.contains(99.9f, 99.9f));// just inside bottom-right
    assert(!box.contains(100, 100));    // exactly on right/bottom edge -> outside
    assert(!box.contains(-1, 50));      // outside left
    assert(!box.contains(50, 200));     // outside bottom

    // intersects
    AABB a{0, 0, 100, 100};
    AABB b{50, 50, 100, 100};           // overlaps
    AABB c{200, 200, 50, 50};           // far away
    AABB d{100, 0, 50, 100};            // shares an edge, no overlap (half-open)
    AABB e{99, 0, 50, 100};             // overlaps by 1 unit

    assert( a.intersects(b));
    assert( b.intersects(a));           // symmetric
    assert(!a.intersects(c));
    assert(!c.intersects(a));
    assert(!a.intersects(d));           // touching at x=100 is not overlap
    assert( a.intersects(e));

    std::cout << "Phase 1: all tests passed.\n";
}

// Count nodes at a given depth
int countAtDepth(const QuadTree::QuadNode* n, int target, int current = 0) {
    if (!n) return 0;
    int total = (current == target) ? 1 : 0;
    for (int i = 0; i < 4; i++)
        total += countAtDepth(n->children[i], target, current + 1);
    return total;
}

// Count points in a subtree
int countPointsInSubtree(const QuadTree::QuadNode* n) {
    if (!n) return 0;
    int total = (int)n->points.size();
    for (int i = 0; i < 4; i++)
        total += countPointsInSubtree(n->children[i]);
    return total;
}

void testQuadTreeInsertAndSubdiv() {
    AABB world{0, 0, 1000, 800};
    QuadTree tree(world, 4, 6);

    // Insert 5 points all in the top-left (should trigger one subdivide)
    Point p1{100, 100}, p2{150, 120}, p3{200, 180}, p4{50, 60}, p5{80, 90};
    assert(tree.insert(&p1));
    assert(tree.insert(&p2));
    assert(tree.insert(&p3));
    assert(tree.insert(&p4));
    assert(tree.insert(&p5));

    const auto* root = tree.getRoot();

    // After 5 inserts into one quadrant, root should be internal
    assert(!root->isLeaf());
    assert(root->points.empty());

    // All 5 points should still be reachable in the subtree
    assert(countPointsInSubtree(root) == 5);

    // Only one child (NW) should have any points at depth 1
    const auto* nw = root->children[0];
    const auto* ne = root->children[1];
    const auto* sw = root->children[2];
    const auto* se = root->children[3];
    assert(countPointsInSubtree(nw) == 5);
    assert(countPointsInSubtree(ne) == 0);
    assert(countPointsInSubtree(sw) == 0);
    assert(countPointsInSubtree(se) == 0);

    // All 5 points are at (100,100), (150,120), (200,180), (50,60), (80,90)
    // — every one of them has x < 500 and y < 400, so they all land in NW

    std::cout << "Phase 3: insert + subdivide tests passed.\n";
}

void testQuadTreeQuery() {
    AABB world{0, 0, 1000, 800};
    QuadTree tree(world, 4, 6);

    Point pNW{100, 100}, pNE{600, 100}, pSW{100, 600}, pSE{600, 600};
    tree.insert(&pNW); tree.insert(&pNE);
    tree.insert(&pSW); tree.insert(&pSE);

    std::vector<Point*> results;
    tree.query({0, 0, 500, 400}, results);   // NW quadrant only

    assert(results.size() == 1);
    assert(results[0] == &pNW);

    QuadTree tree2(world, 4, 6);
    Point p{100, 50};
    tree2.insert(&p);

    results.clear();
    tree2.query({0, 0, 100, 100}, results);   // right edge excluded
    assert(results.empty());

    results.clear();
    tree2.query({0, 0, 101, 100}, results);   // right edge included
    assert(results.size() == 1);

    QuadTree tree3(world, 4, 6);
    Point a{400, 300}, b{600, 300}, c{400, 500}, d{600, 500};
    tree3.insert(&a); tree3.insert(&b); tree3.insert(&c); tree3.insert(&d);

    results.clear();
    tree3.query({350, 250, 300, 300}, results);   // spans all four quadrants

    assert(results.size() == 4);

    std::cout << "Phase 4: query tests passed.\n";
}

void testQuadTreeRemove() {
    AABB world{0, 0, 1000, 800};
    QuadTree tree(world, 4, 6);

    // Insert 5 points
    Point p1{900, 100}, p2{150, 120}, p3{200, 180}, p4{550, 60}, p5{180, 90}, p6{700, 90};
    assert(tree.insert(&p1));
    assert(tree.insert(&p2));
    assert(tree.insert(&p3));
    assert(tree.insert(&p4));
    assert(tree.insert(&p5));
    
    // remove 1 point
    assert(tree.remove(&p1));
    // remove twice
    assert(tree.remove(&p1) == false);
    // remove non-existent point
    assert(tree.remove(&p6) == false);

    auto* root = tree.getRoot();

    // Only 4 points left
    assert(countPointsInSubtree(root) == 4);

    tree.clear();
    root = tree.getRoot();
    assert(root->isLeaf());
    assert(countPointsInSubtree(root) == 0);

    std::cout << "Phase 5: remove + clear tests passed.\n";
}

int main() {
    // TestAABB();
    // testQuadTreeInsertAndSubdiv();
    // testQuadTreeQuery();
    testQuadTreeRemove();

    return 0;
}