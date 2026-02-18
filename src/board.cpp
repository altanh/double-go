#include "double-go/board.h"

#include <array>

namespace double_go {

Board::Board(int size) : size_(size), grid_(size * size, Color::Empty) {}

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
  Color color = grid_[index(p)];
  if (color == Color::Empty)
    return;

  std::vector<Point> stack;
  stack.push_back(p);
  grid_[index(p)] = Color::Empty;
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
        stack.push_back(nbrs[i]);
      }
    }
  }
}

bool Board::is_legal(Point p) const {
  if (!is_on_board(p) || grid_[index(p)] != Color::Empty)
    return false;

  if (ko_point_ && *ko_point_ == p && phase() == Phase::Normal)
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

bool Board::must_pass() const {
  return to_play_ == Color::Black ? black_must_pass_ : white_must_pass_;
}

void Board::apply_move(Point p) {
  grid_[index(p)] = to_play_;

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

  ko_point_.reset();
  if (total_captured == 1 && group_size(p) == 1 && group_liberties(p) == 1) {
    ko_point_ = last_captured;
  }
}

bool Board::game_over() const {
  return consecutive_passes_ >= 2 && phase_ == Phase::Normal;
}

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
  switch (a.type) {
  case ActionType::Pass:
    if (phase_ == Phase::DoubleMove)
      return false;
    ko_point_.reset();
    if (to_play_ == Color::Black)
      black_must_pass_ = false;
    else
      white_must_pass_ = false;
    to_play_ = opponent(to_play_);
    consecutive_passes_++;
    return true;

  case ActionType::Move:
    if (phase_ != Phase::Normal || must_pass())
      return false;
    if (!is_legal(a.point))
      return false;
    apply_move(a.point);
    to_play_ = opponent(to_play_);
    consecutive_passes_ = 0;
    return true;

  case ActionType::DoubleFirst:
    if (phase_ != Phase::Normal || must_pass())
      return false;
    if (!is_legal(a.point))
      return false;
    apply_move(a.point);
    phase_ = Phase::DoubleMove;
    consecutive_passes_ = 0;
    return true;

  case ActionType::DoubleSecond:
    if (phase_ != Phase::DoubleMove)
      return false;
    if (!is_legal(a.point))
      return false;
    apply_move(a.point);
    if (to_play_ == Color::Black)
      black_must_pass_ = true;
    else
      white_must_pass_ = true;
    phase_ = Phase::Normal;
    to_play_ = opponent(to_play_);
    consecutive_passes_ = 0;
    return true;
  }
  return false;
}

std::vector<Action> Board::legal_actions() const {
  std::vector<Action> actions;
  if (phase_ == Phase::DoubleMove) {
    for (int r = 0; r < size_; r++)
      for (int c = 0; c < size_; c++)
        if (is_legal({r, c}))
          actions.push_back(Action::double_second({r, c}));
    return actions;
  }
  if (must_pass()) {
    actions.push_back(Action::pass());
    return actions;
  }
  actions.push_back(Action::pass());
  for (int r = 0; r < size_; r++)
    for (int c = 0; c < size_; c++)
      if (is_legal({r, c})) {
        actions.push_back(Action::move({r, c}));
        actions.push_back(Action::double_first({r, c}));
      }
  return actions;
}

bool Board::play(Point p) { return apply(Action::move(p)); }

void Board::pass() { apply(Action::pass()); }

} // namespace double_go
