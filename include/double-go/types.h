#pragma once

#include <cstdint>

namespace double_go {

enum class Color : uint8_t { Empty, Black, White };

inline Color opponent(Color c) {
    return c == Color::Black ? Color::White : Color::Black;
}

struct Point {
    int row;
    int col;

    bool operator==(const Point&) const = default;
};

enum class Phase : uint8_t { Normal, DoubleMove };

enum class ActionType : uint8_t { Pass, Move, DoubleFirst, DoubleSecond };

struct Action {
    ActionType type;
    Point point{};

    static Action pass() { return {ActionType::Pass}; }
    static Action move(Point p) { return {ActionType::Move, p}; }
    static Action double_first(Point p) { return {ActionType::DoubleFirst, p}; }
    static Action double_second(Point p) { return {ActionType::DoubleSecond, p}; }

    bool operator==(const Action&) const = default;
};

} // namespace double_go
