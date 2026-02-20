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

  bool operator==(const Point &) const = default;
};

enum class Phase : uint8_t { Bonus, First, Second };

enum class ActionType : uint8_t { Pass, Place };

struct Action {
  ActionType type;
  Point point{};

  static Action pass() { return {ActionType::Pass}; }
  static Action place(Point p) { return {ActionType::Place, p}; }

  bool operator==(const Action &) const = default;
};

} // namespace double_go
