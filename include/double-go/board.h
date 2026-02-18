#pragma once

#include "types.h"

#include <optional>
#include <vector>

namespace double_go {

class Board {
  public:
    explicit Board(int size = 19);

    int size() const { return size_; }
    Color at(Point p) const { return grid_[index(p)]; }
    Color to_play() const { return to_play_; }
    std::optional<Point> ko_point() const { return ko_point_; }
    int captures(Color c) const;
    Phase phase() const { return phase_; }
    bool must_pass() const;

    bool is_on_board(Point p) const;
    bool is_legal(Point p) const;
    std::vector<Point> legal_moves() const;
    std::vector<Action> legal_actions() const;

    bool apply(Action a);
    bool play(Point p);
    void pass();

  private:
    int index(Point p) const { return p.row * size_ + p.col; }
    Point point(int idx) const { return {idx / size_, idx % size_}; }

    void neighbors(Point p, Point out[], int& count) const;
    int group_liberties(Point p) const;
    int group_size(Point p) const;
    void remove_group(Point p);
    void flood(Point p, Color color, std::vector<bool>& visited,
               int& lib_count) const;
    void apply_move(Point p);

    int size_;
    std::vector<Color> grid_;
    Color to_play_ = Color::Black;
    std::optional<Point> ko_point_;
    int black_captures_ = 0;
    int white_captures_ = 0;
    Phase phase_ = Phase::Normal;
    bool black_must_pass_ = false;
    bool white_must_pass_ = false;
};

} // namespace double_go
