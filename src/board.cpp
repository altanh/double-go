#include "double-go/board.h"

#include <array>
#include <cassert>

namespace double_go {

Board::Board(int size) : size_(size), grid_(size * size, Color::Empty) {
  ZobristHash &z = ZobristHash::get_instance();
  assert(size > 0);
  assert(size <= 19);
  hash_ = z.black_move();
  set_phase(Phase::First);
}

int Board::captures(Color c) const {
  return c == Color::Black ? black_captures_ : white_captures_;
}

bool Board::is_on_board(Point p) const {
  return p.row >= 0 && p.row < size_ && p.col >= 0 && p.col < size_;
}

void Board::neighbors(Point p, Point out[], int &count) const {
  count = 0;
  constexpr std::array<std::pair<int, int>, 4> dirs = {
      {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
  for (auto [dr, dc] : dirs) {
    Point n{p.row + dr, p.col + dc};
    if (is_on_board(n))
      out[count++] = n;
  }
}

void Board::flood(Point p, Color color, std::vector<bool> &visited,
                  int &lib_count) const {
  int idx = index(p);
  if (visited[idx])
    return;

  Color c = grid_[idx];
  if (c == Color::Empty) {
    lib_count++;
    visited[idx] = true;
    return;
  }
  if (c != color)
    return;

  visited[idx] = true;
  Point nbrs[4];
  int n;
  neighbors(p, nbrs, n);
  for (int i = 0; i < n; i++)
    flood(nbrs[i], color, visited, lib_count);
}

int Board::group_liberties(Point p) const {
  std::vector<bool> visited(size_ * size_, false);
  int libs = 0;
  flood(p, grid_[index(p)], visited, libs);
  return libs;
}

int Board::group_size(Point p) const {
  Color color = grid_[index(p)];
  if (color == Color::Empty)
    return 0;

  std::vector<bool> visited(size_ * size_, false);
  int count = 0;

  // BFS
  std::vector<Point> stack;
  stack.push_back(p);
  visited[index(p)] = true;
  while (!stack.empty()) {
    Point cur = stack.back();
    stack.pop_back();
    count++;

    Point nbrs[4];
    int n;
    neighbors(cur, nbrs, n);
    for (int i = 0; i < n; i++) {
      int ni = index(nbrs[i]);
      if (!visited[ni] && grid_[ni] == color) {
        visited[ni] = true;
        stack.push_back(nbrs[i]);
      }
    }
  }
  return count;
}

void Board::remove_group(Point p) {
  ZobristHash &z = ZobristHash::get_instance();
  Color color = grid_[index(p)];
  if (color == Color::Empty)
    return;

  std::vector<Point> stack;
  stack.push_back(p);
  grid_[index(p)] = Color::Empty;
  hash_ ^= z.stone(color, p);
  while (!stack.empty()) {
    Point cur = stack.back();
    stack.pop_back();

    Point nbrs[4];
    int n;
    neighbors(cur, nbrs, n);
    for (int i = 0; i < n; i++) {
      int ni = index(nbrs[i]);
      if (grid_[ni] == color) {
        grid_[ni] = Color::Empty;
        hash_ ^= z.stone(color, nbrs[i]);
        stack.push_back(nbrs[i]);
      }
    }
  }
}

bool Board::is_legal(Point p) const {
  if (!is_on_board(p) || grid_[index(p)] != Color::Empty)
    return false;

  // Fact: the ko point is cleared after the bonus move, since it can be
  // immediately filled, or the player can play away or pass in the next phase.
  // If a ko point is set in the first move, then it can be filled in the
  // second. Therefore, if a ko point is set, then it's only illegal to play it
  // if it's the players bonus or first move.

  if (ko_point_ && *ko_point_ == p && phase_ != Phase::Second)
    return false;

  Color me = to_play_;
  Color opp = opponent(me);

  Point nbrs[4];
  int n;
  neighbors(p, nbrs, n);

  for (int i = 0; i < n; i++) {
    Color nc = grid_[index(nbrs[i])];
    if (nc == Color::Empty)
      return true; // immediate liberty
  }

  for (int i = 0; i < n; i++) {
    Color nc = grid_[index(nbrs[i])];
    if (nc == opp && group_liberties(nbrs[i]) == 1)
      return true; // captures opponent group
  }

  for (int i = 0; i < n; i++) {
    Color nc = grid_[index(nbrs[i])];
    if (nc == me && group_liberties(nbrs[i]) >= 2)
      return true; // connects to friendly group that survives
  }

  return false; // suicide
}

std::vector<Point> Board::legal_moves() const {
  std::vector<Point> moves;
  for (int r = 0; r < size_; r++)
    for (int c = 0; c < size_; c++)
      if (is_legal({r, c}))
        moves.push_back({r, c});
  return moves;
}

void Board::apply_move(Point p) {
  ZobristHash &z = ZobristHash::get_instance();

  grid_[index(p)] = to_play_;
  hash_ ^= z.stone(to_play_, p);

  Color opp = opponent(to_play_);
  int total_captured = 0;
  Point last_captured{};

  Point nbrs[4];
  int n;
  neighbors(p, nbrs, n);

  for (int i = 0; i < n; i++) {
    if (grid_[index(nbrs[i])] == opp && group_liberties(nbrs[i]) == 0) {
      int sz = group_size(nbrs[i]);
      total_captured += sz;
      last_captured = nbrs[i];
      remove_group(nbrs[i]);
    }
  }

  if (to_play_ == Color::Black)
    black_captures_ += total_captured;
  else
    white_captures_ += total_captured;

  clear_ko();

  if (phase_ != Phase::Bonus && total_captured == 1 && group_size(p) == 1 &&
      group_liberties(p) == 1) {
    set_ko(last_captured);
  }
}

void Board::clear_ko() {
  ZobristHash &z = ZobristHash::get_instance();
  if (ko_point_) {
    hash_ ^= z.ko(*ko_point_);
  }
  ko_point_.reset();
}

void Board::set_ko(Point p) {
  ZobristHash &z = ZobristHash::get_instance();
  if (ko_point_) {
    hash_ ^= z.ko(*ko_point_);
  }
  hash_ ^= z.ko(p);
  ko_point_ = p;
}

void Board::flip_player() {
  ZobristHash &z = ZobristHash::get_instance();
  hash_ ^= z.black_move();
  to_play_ = opponent(to_play_);
}

void Board::set_phase(Phase phase) {
  ZobristHash &z = ZobristHash::get_instance();
  hash_ ^= z.phase(phase_);
  hash_ ^= z.phase(phase);
  phase_ = phase;
}

bool Board::game_over() const { return consecutive_passes_ >= 2; }

ScoreResult Board::score(double komi) const {
  int black_stones = 0, white_stones = 0;
  int black_territory = 0, white_territory = 0;
  int total = size_ * size_;

  for (int i = 0; i < total; i++) {
    if (grid_[i] == Color::Black)
      black_stones++;
    else if (grid_[i] == Color::White)
      white_stones++;
  }

  std::vector<bool> visited(total, false);
  for (int i = 0; i < total; i++) {
    if (visited[i] || grid_[i] != Color::Empty)
      continue;

    // Flood-fill connected empty region
    std::vector<int> region;
    std::vector<int> stack;
    stack.push_back(i);
    visited[i] = true;
    bool borders_black = false, borders_white = false;

    while (!stack.empty()) {
      int idx = stack.back();
      stack.pop_back();
      region.push_back(idx);

      Point p = point(idx);
      Point nbrs[4];
      int n;
      neighbors(p, nbrs, n);
      for (int j = 0; j < n; j++) {
        int ni = index(nbrs[j]);
        if (visited[ni])
          continue;
        if (grid_[ni] == Color::Empty) {
          visited[ni] = true;
          stack.push_back(ni);
        } else if (grid_[ni] == Color::Black) {
          borders_black = true;
        } else {
          borders_white = true;
        }
      }
    }

    if (borders_black && !borders_white)
      black_territory += static_cast<int>(region.size());
    else if (borders_white && !borders_black)
      white_territory += static_cast<int>(region.size());
  }

  ScoreResult result;
  result.black_territory = black_territory;
  result.white_territory = white_territory;
  result.black_stones = black_stones;
  result.white_stones = white_stones;
  result.black_score = black_stones + black_territory;
  result.white_score = white_stones + white_territory + komi;
  return result;
}

bool Board::apply(Action a) {
  if (game_over())
    return false;

  switch (a.type) {
  case ActionType::Pass:
    if (phase_ == Phase::Second) {
      consecutive_passes_ = 0;
      // Keep ko from Phase 1
    } else {
      ++consecutive_passes_;
      // Reset ko
      clear_ko();
    }
    set_phase(Phase::First);
    flip_player();
    return true;

  case ActionType::Place:
    if (!is_legal(a.point)) {
      return false;
    }
    apply_move(a.point);
    consecutive_passes_ = 0;
    if (phase_ == Phase::Bonus) {
      set_phase(Phase::First);
    } else if (phase_ == Phase::First) {
      set_phase(Phase::Second);
    } else {
      // phase_ == Phase::Second
      set_phase(Phase::Bonus); // Opponent gets bonus move
      flip_player();
    }
    return true;
  }
  return false;
}

std::vector<Action> Board::legal_actions() const {
  std::vector<Action> actions;
  actions.push_back(Action::pass());
  for (int r = 0; r < size_; r++)
    for (int c = 0; c < size_; c++)
      if (is_legal({r, c}))
        actions.push_back(Action::place({r, c}));
  return actions;
}

bool Board::play_single(Point p) {
  if (phase_ != Phase::First) {
    return false;
  }
  if (!is_legal(p)) {
    return false;
  }
  apply(Action::place(p));
  pass();
  return true;
}

void Board::pass() { apply(Action::pass()); }

} // namespace double_go
