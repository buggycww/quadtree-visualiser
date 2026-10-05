#include "Visualizer.h"
#include <iostream>
#include <cmath>
#include <optional>
#include <algorithm>
#include <sstream>

Visualizer::Visualizer(QuadTree& t, const AABB& wb, unsigned w, unsigned h)
    : window(sf::VideoMode({w, h}), "Quadtree Visualizer")
    , tree(t), worldBounds(wb)
{
    window.setFramerateLimit(60);

    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf") &&
        !font.openFromFile("C:/Windows/Fonts/consola.ttf")) {
        std::cerr << "Failed to load font.\n";
    }

    // Default query rect: right half of the world
    queryRect = { worldBounds.x + worldBounds.w * 0.5f, worldBounds.y,
                  worldBounds.w * 0.5f, worldBounds.h };

    computeTransform();
}

void Visualizer::computeTransform() {
    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);

    const float MARGIN = 40.f;
    float availW = winW - MARGIN * 2.f;
    float availH = winH - MARGIN * 2.f - 50.f;   // reserve bottom for HUD

    scaleX = availW / worldBounds.w;
    scaleY = availH / worldBounds.h;
    offsetX = MARGIN;
    offsetY = MARGIN;
}

sf::Vector2f Visualizer::toScreen(float wx, float wy) const {
    return { offsetX + (wx - worldBounds.x) * scaleX,
             offsetY + (wy - worldBounds.y) * scaleY };
}

sf::Vector2f Visualizer::scaleSize(float w, float h) const {
    return {w * scaleX, h * scaleY};
}

void Visualizer::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

void Visualizer::processEvents() {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        else if (const auto* m = event->getIf<sf::Event::MouseButtonPressed>()) {
            float px = static_cast<float>(m->position.x);
            float py = static_cast<float>(m->position.y);

            if (m->button == sf::Mouse::Button::Left) {
                dragStart   = { px, py };
                dragCurrent = dragStart;
                dragging = true;
            }
            else if (m->button == sf::Mouse::Button::Right) {
                float wx = (px - offsetX) / scaleX + worldBounds.x;
                float wy = (py - offsetY) / scaleY + worldBounds.y;
                removeNearestPoint(wx, wy);
            }
        }
        else if (const auto* m = event->getIf<sf::Event::MouseMoved>()) {
            if (dragging) {
                dragCurrent = { static_cast<float>(m->position.x),
                                static_cast<float>(m->position.y) };
            }
        }
        else if (const auto* m = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (m->button == sf::Mouse::Button::Left && dragging) {
                dragCurrent = { static_cast<float>(m->position.x),
                                static_cast<float>(m->position.y) };
                finalizeDrag();
                dragging = false;
            }
        }
        else if (const auto* k = event->getIf<sf::Event::KeyPressed>()) {
            if (k->code == sf::Keyboard::Key::Space) {
                clearAll();
            }
            else if (k->code == sf::Keyboard::Key::Q ||
                     k->code == sf::Keyboard::Key::Escape) {
                queryActive = false;
                queryResults.clear();
            }
        }
    }
}

void Visualizer::update(float dt) {
    queryPulse += dt * 2.f;
    if (queryPulse > 6.28318f) queryPulse -= 6.28318f;
}

void Visualizer::addPoint(float wx, float wy) {
    Point* p = new Point(wx, wy);
    if (tree.insert(p)) {
        ownedPoints.push_back(p);
    } else {
        delete p;   // insert failed; we still own it, so clean up
    }

    if (queryActive) {
        queryResults.clear();
        tree.query(queryRect, queryResults);
    }
}

void Visualizer::removeNearestPoint(float wx, float wy) {
    Point* nearest = nullptr;
    float bestDistSq = 1e18f;
    for (Point* p : ownedPoints) {
        float dx = p->x - wx;
        float dy = p->y - wy;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestDistSq) { bestDistSq = d2; nearest = p; }
    }
    if (!nearest) return;

    tree.remove(nearest);
    ownedPoints.erase(std::remove(ownedPoints.begin(), ownedPoints.end(), nearest),
                      ownedPoints.end());
    delete nearest;

    if (queryActive) {
        queryResults.clear();
        tree.query(queryRect, queryResults);
    }
}

void Visualizer::clearAll() {
    for (Point* p : ownedPoints) delete p;
    ownedPoints.clear();
    tree.clear();
    queryResults.clear();
}

void Visualizer::render() {
    window.clear(sf::Color(20, 22, 28));
    drawGrid(tree.getRoot());
    drawPoints();
    drawQuery();
    drawDragPreview();   // ← shows the rect while you're dragging
    drawHUD();
    window.display();
}

void Visualizer::drawGrid(const QuadTree::QuadNode* node) {
    if (!node) return;

    if (!node->isLeaf()) {
        for (auto child : node->children) drawGrid(child);
        return;
    }

    sf::Vector2f topLeft = toScreen(node->bounds.x, node->bounds.y);
    sf::Vector2f size(scaleSize(node->bounds.w, node->bounds.h));

    sf::RectangleShape rect(size);
    rect.setPosition(topLeft);
    rect.setFillColor(sf::Color::Transparent);

    int d = node->depth;

    // Thickness falls off with depth: 3.0 at depth 0, ~0.5 at depth 5
    float thickness = std::max(0.5f, 3.0f - d * 0.5f);

    // Brightness falls off with depth: bright at shallow, dim at deep
    int alpha = std::max(60, 220 - d * 30);

    rect.setOutlineColor(sf::Color(120, 160, 220, alpha));
    rect.setOutlineThickness(thickness);
    window.draw(rect);
}

void Visualizer::drawPoints() {
    for (Point* p : ownedPoints) {
        bool inQuery = false;
        for (Point* q : queryResults) {
            if (q == p) { inQuery = true; break; }
        }
        sf::CircleShape circle(4.f);
        circle.setOrigin({4.f, 4.f});
        circle.setPosition(toScreen(p->x, p->y));
        circle.setFillColor(inQuery ? sf::Color(255, 200, 60)
                                    : sf::Color(180, 200, 240));
        window.draw(circle);
    }
}

void Visualizer::drawQuery() {
    if (!queryActive) return;

    sf::Vector2f tl = toScreen(queryRect.x, queryRect.y);
    sf::Vector2f size(scaleSize(queryRect.w, queryRect.h));

    float pulse = (std::sin(queryPulse) + 1.f) * 0.5f;   // 0..1
    int alpha = static_cast<int>(50 + pulse * 60);

    sf::RectangleShape rect(size);
    rect.setPosition(tl);
    rect.setFillColor(sf::Color(255, 200, 60, alpha));
    rect.setOutlineColor(sf::Color(255, 200, 60, 220));
    rect.setOutlineThickness(2.f);
    window.draw(rect);
}

AABB Visualizer::computeDragRect() const {
    float x1 = std::min(dragStart.x, dragCurrent.x);
    float y1 = std::min(dragStart.y, dragCurrent.y);
    float x2 = std::max(dragStart.x, dragCurrent.x);
    float y2 = std::max(dragStart.y, dragCurrent.y);
    return { x1, y1, x2 - x1, y2 - y1 };
}

void Visualizer::finalizeDrag() {
    float dx = dragCurrent.x - dragStart.x;
    float dy = dragCurrent.y - dragStart.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    constexpr float CLICK_THRESHOLD = 5.f;

    if (dist < CLICK_THRESHOLD) {
        // It's a click: add a point
        float wx = (dragStart.x - offsetX) / scaleX + worldBounds.x;
        float wy = (dragStart.y - offsetY) / scaleY + worldBounds.y;
        if (worldBounds.contains(wx, wy)) addPoint(wx, wy);
    }
    else {
        // It's a drag: define a query rect (convert screen -> world)
        AABB screen = computeDragRect();
        float wx1 = (screen.x - offsetX) / scaleX + worldBounds.x;
        float wy1 = (screen.y - offsetY) / scaleY + worldBounds.y;
        float wx2 = (screen.x + screen.w - offsetX) / scaleX + worldBounds.x;
        float wy2 = (screen.y + screen.h - offsetY) / scaleY + worldBounds.y;

        queryRect = { wx1, wy1, wx2 - wx1, wy2 - wy1 };
        queryActive = true;
        queryResults.clear();
        tree.query(queryRect, queryResults);
    }
}

void Visualizer::drawDragPreview() {
    if (!dragging) return;

    AABB r = computeDragRect();
    if (r.w < 2.f && r.h < 2.f) return;  // too small to matter

    sf::RectangleShape rect({ r.w, r.h });
    rect.setPosition({ r.x, r.y });
    rect.setFillColor(sf::Color(100, 180, 255, 40));
    rect.setOutlineColor(sf::Color(100, 180, 255, 200));
    rect.setOutlineThickness(1.f);
    window.draw(rect);
}

void Visualizer::drawHUD() {
    std::ostringstream ss;
    ss << "Points: " << ownedPoints.size();
    if (queryActive)
        ss << "   |   In query: " << queryResults.size() << "   (Q to clear)";
    ss << "   |   LMB: add   LMB-drag: query   RMB: remove   Space: clear";

    sf::Text txt(font, ss.str(), 15);
    txt.setPosition({20.f, static_cast<float>(window.getSize().y) - 32.f});
    txt.setFillColor(sf::Color(200, 200, 210));
    window.draw(txt);
}