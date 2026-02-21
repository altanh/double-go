#include "double-go/model.h"

namespace double_go {

torch::Tensor Model::encode(const std::deque<Board> &boards) {
  //   HISTORY_LEN board states (black, white stones)
  // + current player
  // + is bonus phase
  // + is first phase
  // + is second phase
  auto encoding = torch::zeros({NUM_PLANES, board_size, board_size});
  auto a = encoding.accessor<float, 3>();

  // boards.back() is the current board
  int board_index = HISTORY_LEN - 1;
  for (auto board = boards.rbegin(); board != boards.rend(); ++board) {
    if (board_index < 0) {
      break;
    }
    // Fill board planes
    for (int row = 0; row < board_size; ++row) {
      for (int col = 0; col < board_size; ++col) {
        Color c = board->at_index(row * board_size + col);
        if (c == Color::Black) {
          a[2 * board_index][row][col] = 1.0f;
        } else if (c == Color::White) {
          a[2 * board_index + 1][row][col] = 1.0f;
        }
      }
    }
    --board_index;
  }

  // Set player plane if white to play
  if (boards.back().to_play() == Color::White) {
    encoding[HISTORY_LEN * 2].fill_(1.0f);
  }

  // Set appropriate phase plane
  int phase_index = static_cast<int>(boards.back().phase());
  encoding[HISTORY_LEN * 2 + 1 + phase_index].fill_(1.0f);

  return encoding;
}

} // namespace double_go