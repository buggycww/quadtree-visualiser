#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "QuadTree.h"
#include "Point.h"

class Visualizer {
public:
    Visualizer(QuadTree& tree, const AABB& worldBounds,
               unsigned width = 1000, unsigned height = 800);
    void run();

private:
    sf::RenderWindow window;
    sf::Font font;
    QuadTree& tree;
    AABB worldBounds;

    // World -> screen transform
    float scaleX, scaleY, offsetX, offsetY;
    void computeTransform();
    sf::Vector2f toScreen(float wx, float wy) const;
    sf::Vector2f scaleSize(float w, float h) const;

    // Owned points — the visualizer owns them, not the tree
    std::vector<Point*> ownedPoints;

    // Query state
    AABB queryRect;
    std::vector<Point*> queryResults;
    bool queryActive = false;
    float queryPulse = 0.f;    bool dragging = false;
    sf::Vector2f dragStart{0.f, 0.f};
    sf::Vector2f dragCurrent{0.f, 0.f};
    void finalizeDrag();
    AABB computeDragRect() const;
    void drawDragPreview();

    void processEvents();
    void update(float dt);
    void render();

    void drawGrid(const QuadTree::QuadNode* node);
    void drawPoints();
    void drawQuery();
    void drawHUD();

    void addPoint(float wx, float wy);
    void removeNearestPoint(float wx, float wy);
    void clearAll();
};