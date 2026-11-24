#ifndef AABB_H
#define AABB_H

#include <algorithm>
#include <limits>
#include <type_traits>

#include <SFML/System/Vector2.hpp>

template <typename T>
struct AABB {
    static_assert(std::is_floating_point_v<T>);

    constexpr AABB& operator&=(const AABB& other) {
        minPoint = {std::max(minPoint.x, other.minPoint.x), std::max(minPoint.y, other.minPoint.y)};
        maxPoint = {std::min(maxPoint.x, other.maxPoint.x), std::min(maxPoint.y, other.maxPoint.y)};
        return *this;
    }

    constexpr AABB operator&(const AABB& other) const {
        auto ret = *this;
        return ret &= other;
    }

    constexpr AABB& operator|=(const AABB& other) {
        minPoint = {std::min(minPoint.x, other.minPoint.x), std::min(minPoint.y, other.minPoint.y)};
        maxPoint = {std::max(maxPoint.x, other.maxPoint.x), std::max(maxPoint.y, other.maxPoint.y)};
        return *this;
    }

    constexpr AABB operator|(const AABB& other) const {
        auto ret = *this;
        return ret |= other;
    }

    constexpr AABB& operator|=(const sf::Vector2<T>& p) {
        minPoint = {std::min(minPoint.x, p.x), std::min(minPoint.y, p.y)};
        maxPoint = {std::max(maxPoint.x, p.x), std::max(maxPoint.y, p.y)};
        return *this;
    }

    constexpr AABB operator|(const sf::Vector2<T>& p) const {
        auto ret = *this;
        return ret |= p;
    }

    constexpr bool operator==(const AABB& other) const {
        return minPoint == other.minPoint && maxPoint == other.maxPoint;
    }

    constexpr bool empty() const { return minPoint.x > maxPoint.x || minPoint.y > maxPoint.y; }

    constexpr explicit operator bool() const { return !empty(); }

    constexpr bool contains(const sf::Vector2<T>& p) const {
        return minPoint.x <= p.x && maxPoint.x >= p.x && minPoint.y <= p.y && maxPoint.y >= p.y;
    }

    constexpr bool contains(const AABB& other) const { return (*this & other) == other; }

    constexpr bool intersects(const AABB& other) const { return !(*this & other).empty(); }

    constexpr bool intersects(const sf::Vector2<T>& p) const { return contains(p); }

    constexpr sf::Vector2<T> middle() const {
        return minPoint + (maxPoint - minPoint) / static_cast<T>(2);
    }

    constexpr sf::Vector2<T> centroid() const { return middle(); }

    constexpr T perimeterLength() const {
        if (empty()) {
            return 0;
        }
        const auto delta = maxPoint - minPoint;
        return delta.x * 2 + delta.y * 2;
    }

    sf::Vector2<T> minPoint = {std::numeric_limits<T>::infinity(),
                               std::numeric_limits<T>::infinity()};
    sf::Vector2<T> maxPoint = {-std::numeric_limits<T>::infinity(),
                               -std::numeric_limits<T>::infinity()};
};

#endif  // AABB_H
