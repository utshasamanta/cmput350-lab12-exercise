// Lab 12: 2D ray tracing with a BVH. Work through the TODOs below and answer the questions in
// written.txt.
// TODO Bonus 2: some circles are drawn with their sides cut off; fix them.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <vector>

#include "aabb.h"
#include "vector4.h"
#include "vector_util.h"
#include <SFML/Graphics.hpp>

constexpr int gScreenWidth = 1024;
constexpr int gScreenHeight = 768;
constexpr float EPSILON = 0.001f;

struct Circle {
    constexpr bool intersects(sf::Vector2f point) const {
        return (point - position).lengthSquared() < radius * radius;
    }

    constexpr AABB<float> bounds() const { return {position - radius, position + radius}; }

    sf::Vector2f position;
    float radius;
    Vector4f color;
};

struct BVHNode {
    AABB<float> bounds;
    int childAOrIdx = -1;  // if leaf node, first index into index list, else child bvh node ptr
    int childBOrLen = -1;  // if leaf node, length into index list, else child bvh node ptr
    int isLeaf = 0;
    // TODO Bonus 1: store each node's height in the tree and color its box in Draw by height.
    int padding = 0;  // to get to POT size of 32 bytes.
};
static_assert(sizeof(BVHNode) == 32, "keep BVHNode a power-of-two size");

struct BVH {
    static constexpr float T_ISECT = 1;
    static constexpr float T_TRAV = 1.f / 8;
    static constexpr int BINS = 32;

    explicit BVH(const std::vector<Circle>& prims) : primitives(&prims), indices(prims.size()) {
        std::iota(indices.begin(), indices.end(), 0);
        nodes.reserve(2 * prims.size());
        nodes.push_back({});
        constructNode(0, static_cast<int>(indices.size()), 0);
    }

    // TODO Q4: write a reset() function that rebuilds the tree instead of refitting it.

    void constructNode(int startIdx, int numIndices, int bvhOffset) {
        const std::vector<Circle>& prims = *primitives;
        const auto first = indices.begin() + startIdx;
        const auto last = first + numIndices;
        const auto centroid = [&](int idx) { return prims[idx].bounds().centroid(); };

        // determine total bound of centroids (to determine bins)
        AABB<float> centroidBound{};
        for (auto it = first; it != last; ++it) {
            centroidBound |= centroid(*it) + EPSILON;
            centroidBound |= centroid(*it) - EPSILON;
        }
        const sf::Vector2f binWidth =
            (centroidBound.maxPoint - centroidBound.minPoint) / static_cast<float>(BINS);
        assert(binWidth.x != 0);
        assert(binWidth.y != 0);
        const sf::Vector2f invBinWidth = {1.f / binWidth.x, 1.f / binWidth.y};

        // find bin each primitive belongs to, per dimension (0 = x, 1 = y)
        std::array<std::array<AABB<float>, BINS>, 2> binBounds{};
        std::array<std::array<int, BINS>, 2> binCounts{};
        for (auto it = first; it != last; ++it) {
            const sf::Vector2i bin((centroid(*it) - centroidBound.minPoint) * invBinWidth);
            for (int dim : {0, 1}) {
                binBounds[dim][axis(bin, dim)] |= prims[*it].bounds();
                ++binCounts[dim][axis(bin, dim)];
            }
        }
        // prefix and suffix sums
        std::array<std::array<AABB<float>, BINS + 1>, 2> binBoundsPrefix{};
        std::array<std::array<int, BINS + 1>, 2> binCountsPrefix{};
        std::array<std::array<AABB<float>, BINS + 1>, 2> binBoundsSuffix{};
        std::array<std::array<int, BINS + 1>, 2> binCountsSuffix{};
        for (int dim : {0, 1}) {
            for (int i = 0; i < BINS; ++i) {
                binBoundsPrefix[dim][i + 1] = binBoundsPrefix[dim][i] | binBounds[dim][i];
                binCountsPrefix[dim][i + 1] = binCountsPrefix[dim][i] + binCounts[dim][i];
            }
            for (int i = BINS - 1; i >= 0; --i) {
                binBoundsSuffix[dim][i] = binBoundsSuffix[dim][i + 1] | binBounds[dim][i];
                binCountsSuffix[dim][i] = binCountsSuffix[dim][i + 1] + binCounts[dim][i];
            }
        }
        const AABB<float> totalBound = binBoundsPrefix[0].back();
        const float totalPerimLength = totalBound.perimeterLength();

        // Compute best split. Split BINS - 1 (everything on the left) is skipped since it is
        // always worse than just making a leaf node.
        float minCost = std::numeric_limits<float>::infinity();
        int bestDim = -1;
        int bestSplit = -1;
        for (int split = 0; split < BINS - 1; ++split) {
            for (int dim : {0, 1}) {
                const float cost =
                    T_TRAV +
                    T_ISECT *
                        ((binBoundsPrefix[dim][split + 1].perimeterLength() / totalPerimLength) *
                             binCountsPrefix[dim][split + 1] +
                         (binBoundsSuffix[dim][split + 1].perimeterLength() / totalPerimLength) *
                             binCountsSuffix[dim][split + 1]);
                if (cost < minCost) {
                    minCost = cost;
                    bestDim = dim;
                    bestSplit = split;
                }
            }
        }

        const auto makeLeaf = [&] {
            nodes[bvhOffset] = BVHNode{totalBound, startIdx, numIndices, /*isLeaf=*/true};
        };
        // see if we should be leaf node
        if (numIndices * T_ISECT < minCost) {
            makeLeaf();
            return;
        }
        // repartition indices based on chosen split
        const float pivotPos =
            axis(centroidBound.minPoint, bestDim) + (bestSplit + 1) * axis(binWidth, bestDim);
        const auto mid = std::partition(
            first, last, [&](int idx) { return axis(centroid(idx), bestDim) < pivotPos; });
        const int smallerCount = static_cast<int>(mid - first);
        if (smallerCount == numIndices) {
            makeLeaf();
            return;
        }
        // internal node; children are appended after their parent
        const int childA = static_cast<int>(nodes.size());
        const int childB = childA + 1;
        nodes[bvhOffset] = BVHNode{totalBound, childA, childB, /*isLeaf=*/false};
        nodes.resize(nodes.size() + 2);  // filled in by the recursive calls
        constructNode(startIdx, smallerCount, childA);
        constructNode(startIdx + smallerCount, numIndices - smallerCount, childB);
    }

    // Refit every box bottom-up without changing the tree structure. Children are always stored
    // after their parent, so a reverse sweep visits them first.
    void fullRefit() {
        for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i) {
            BVHNode& node = nodes[i];
            if (node.isLeaf) {
                node.bounds = {};
                for (int j = node.childAOrIdx; j < node.childAOrIdx + node.childBOrLen; ++j)
                    node.bounds |= (*primitives)[indices[j]].bounds();
            } else {
                node.bounds = nodes[node.childAOrIdx].bounds | nodes[node.childBOrLen].bounds;
            }
        }
    }

    const std::vector<Circle>* primitives;
    std::vector<int> indices;
    std::vector<BVHNode> nodes;
};

// On a hit, color is set to the color of the last circle hit.
bool intersects(sf::Vector2f point, const BVHNode& node, const BVH& bvh, Vector4f& color) {
    if (!node.bounds.intersects(point)) return false;
    if (node.isLeaf) {
        bool hit = false;
        for (int i = node.childAOrIdx; i < node.childAOrIdx + node.childBOrLen; ++i) {
            const Circle& circle = (*bvh.primitives)[bvh.indices[i]];
            if (circle.intersects(point)) {
                color = circle.color;
                hit = true;
            }
        }
        return hit;
    }
    // evaluate both children (no short-circuit) so the overlap color stays consistent
    const bool hitA = intersects(point, bvh.nodes[node.childAOrIdx], bvh, color);
    const bool hitB = intersects(point, bvh.nodes[node.childBOrLen], bvh, color);
    return hitA || hitB;
}

// SOA layout for easy buffering
// TODO Q3: store a 2D movement direction for each circle, and write an update() function that
// moves every circle by its movement.
struct Scene {
    std::size_t createCircle(sf::Vector2f position, float radius, const Vector4f& color) {
        circles.push_back({position, radius, color});
        return circles.size() - 1;
    }

    Circle& circle(std::size_t i) {
        assert(valid(i));
        return circles[i];
    }

    bool valid(std::size_t i) const { return i < circles.size(); }

    std::vector<Circle> circles;
};

void Draw(sf::RenderWindow& window, const AABB<float>& aabb) {
    // draw frame of rectangle
    static sf::RectangleShape shape;
    shape.setSize(aabb.maxPoint - aabb.minPoint);
    shape.setPosition(aabb.minPoint);
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(1.0f);
    window.draw(shape);
}

int main() {
    const sf::Vector2u screenSize(gScreenWidth, gScreenHeight);
    sf::RenderWindow window(sf::VideoMode(screenSize), "BVH", sf::State::Windowed);
    window.setFramerateLimit(30);

    std::mt19937 rng;  // default seed: the same scene every run
    const auto uniform = [&rng](float lo, float hi) {
        return std::uniform_real_distribution<float>(lo, hi)(rng);
    };
    Scene scene;
    for (int n = 0; n < 100; ++n) {
        scene.createCircle({uniform(0, gScreenWidth), uniform(0, gScreenHeight)}, uniform(10, 20),
                           {uniform(0, 1), uniform(0, 1), uniform(0, 1), 1});
    }
    BVH bvh(scene.circles);

    std::vector<std::uint8_t> pixels(4 * gScreenWidth * gScreenHeight);  // RGBA, starts black
    sf::Texture texture(screenSize);
    const sf::Sprite sprite(texture);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::Escape) window.close();
        }

        // TODO Q3: every frame, call update() on the scene and fullRefit() on the BVH.

        for (int y = 0; y < gScreenHeight; ++y) {
            for (int x = 0; x < gScreenWidth; ++x) {
                const sf::Vector2f pixel(static_cast<float>(x), static_cast<float>(y));
                // TODO Q2: antialiasing. Take four samples at offsets of +-0.3333 from the pixel
                // and average their colors.
                Vector4f color{};  // stays transparent black on a miss
                intersects(pixel, bvh.nodes[0], bvh, color);
                const std::size_t i = 4 * (static_cast<std::size_t>(y) * gScreenWidth + x);
                pixels[i + 0] = static_cast<std::uint8_t>(255 * color.x);  // R
                pixels[i + 1] = static_cast<std::uint8_t>(255 * color.y);  // G
                pixels[i + 2] = static_cast<std::uint8_t>(255 * color.z);  // B
                pixels[i + 3] = static_cast<std::uint8_t>(255 * color.w);  // A
            }
        }        
        // TODO Q4: rebuild the BVH with reset() instead of fullRefit().

        texture.update(pixels.data());

        window.clear();
        window.draw(sprite);
        // TODO Q1: only draw non-leaf nodes so the BVH structure is easier to see.
        for (const BVHNode& node : bvh.nodes) {
            Draw(window, node.bounds);
        }
        window.display();
    }
    return 0;
}
