#pragma once

#include "types.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <random>
#include <vector>

namespace double_go {

struct ScoreResult {
  int black_territory;
  int white_territory;
  int black_stones;
  int white_stones;
  double black_score;
  double white_score;
};

class Board {
public:
  explicit Board(int size = 19);

  int size() const { return size_; }
  Color at(Point p) const { return grid_[index(p)]; }
  Color to_play() const { return to_play_; }
  std::optional<Point> ko_point() const { return ko_point_; }
  int captures(Color c) const;
  Phase phase() const { return phase_; }
  bool has_bonus_move() const { return phase_ == Phase::Bonus; }
  bool game_over() const;
  int consecutive_passes() const { return consecutive_passes_; }
  ScoreResult score(double komi = 6.5) const;

  bool is_on_board(Point p) const;
  bool is_legal(Point p) const;
  std::vector<Point> legal_moves() const;
  std::vector<Action> legal_actions() const;

  bool apply(Action a);
  // Plays a single move and ends turn, if currently in the First phase.
  // Otherwise, the move is not played. Returns true if a legal move was played.
  bool play_single(Point p);
  void pass();

  uint64_t hash() const { return hash_; }

private:
  int index(Point p) const { return p.row * size_ + p.col; }
  Point point(int idx) const { return {idx / size_, idx % size_}; }

  void neighbors(Point p, Point out[], int &count) const;
  int group_liberties(Point p) const;
  int group_size(Point p) const;
  void remove_group(Point p);
  void flood(Point p, Color color, std::vector<bool> &visited,
             int &lib_count) const;
  void apply_move(Point p);
  void clear_ko();
  void set_ko(Point p);
  void flip_player();
  void set_phase(Phase phase);
  int size_;
  std::vector<Color> grid_;
  Color to_play_ = Color::Black;
  std::optional<Point> ko_point_;
  int black_captures_ = 0;
  int white_captures_ = 0;
  Phase phase_ = Phase::First;
  int consecutive_passes_ = 0;

  uint64_t hash_;
};

class ZobristHash {
  // 0: black, 1: white, 2: ko point
  std::array<uint64_t, 19 * 19 * 3> stones_;
  std::array<uint64_t, 3> phases_;
  uint64_t black_move_;

public:
  static ZobristHash &get_instance() {
    static ZobristHash instance;
    return instance;
  }

  uint64_t stone(Color c, Point p) const {
    uint64_t offset = c == Color::Black ? 0 : 1;
    return stones_[offset * 19 * 19 + p.row * 19 + p.col];
  }

  uint64_t ko(Point p) const {
    return stones_[2 * 19 * 19 + p.row * 19 + p.col];
  }

  uint64_t black_move() const { return black_move_; }

  uint64_t phase(Phase phase) const {
    return phases_[static_cast<size_t>(phase)];
  }

private:
  static std::seed_seq seed(const char *str) {
    return std::seed_seq(str, str + std::strlen(str));
  }

  ZobristHash() {
    std::seed_seq s = seed("the quick brown fox jumps over the lazy dog");
    std::mt19937_64 rng(s);
    for (int i = 0; i < stones_.size(); ++i) {
      stones_[i] = rng();
    }
    for (int i = 0; i < phases_.size(); ++i) {
      phases_[i] = rng();
    }
    black_move_ = rng();
  }
};

} // namespace double_go
