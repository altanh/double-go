#include <gtest/gtest.h>

#include "double-go/model.h"

#include <deque>

using namespace double_go;

// ===== Basic Encoding Tests =====

// Encoding has correct shape
TEST(ModelEncode, CorrectShape) {
  Model model(9);
  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);

  // num_planes = HISTORY_LEN * 2 + 1 + 3 = 8*2 + 4 = 20
  EXPECT_EQ(encoding.dim(), 3);
  EXPECT_EQ(encoding.size(0), 20);
  EXPECT_EQ(encoding.size(1), 9);
  EXPECT_EQ(encoding.size(2), 9);
}

// Different board sizes produce correct shapes
TEST(ModelEncode, DifferentBoardSizes) {
  for (int size : {9, 13, 19}) {
    Model model(size);
    std::deque<Board> history;
    history.push_back(Board(size));

    auto encoding = model.encode(history);

    EXPECT_EQ(encoding.size(0), 20);
    EXPECT_EQ(encoding.size(1), size);
    EXPECT_EQ(encoding.size(2), size);
  }
}

// ===== Empty Board Encoding =====

// Empty board has all stone planes zero
TEST(ModelEncode, EmptyBoardStonePlanesZero) {
  Model model(9);
  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // First 16 planes (8 history * 2 colors) should all be zero
  for (int plane = 0; plane < Model::HISTORY_LEN * 2; ++plane) {
    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        EXPECT_EQ(a[plane][r][c], 0.0f)
            << "Non-zero at plane " << plane << ", (" << r << "," << c << ")";
      }
    }
  }
}

// Empty board with Black to play has player plane all zeros
TEST(ModelEncode, EmptyBoardBlackToPlayPlaneZero) {
  Model model(9);
  std::deque<Board> history;
  history.push_back(Board(9));

  EXPECT_EQ(history.back().to_play(), Color::Black);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // Player plane (index 16) should be all zeros for Black to play
  int player_plane = Model::HISTORY_LEN * 2;
  for (int r = 0; r < 9; ++r) {
    for (int c = 0; c < 9; ++c) {
      EXPECT_EQ(a[player_plane][r][c], 0.0f);
    }
  }
}

// Empty board in First phase has correct phase plane set
TEST(ModelEncode, EmptyBoardFirstPhasePlane) {
  Model model(9);
  std::deque<Board> history;
  history.push_back(Board(9));

  EXPECT_EQ(history.back().phase(), Phase::First);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // Phase planes start at index 17 (after player plane)
  // Phase::Bonus = 0, Phase::First = 1, Phase::Second = 2
  int bonus_plane = Model::HISTORY_LEN * 2 + 1 + 0;
  int first_plane = Model::HISTORY_LEN * 2 + 1 + 1;
  int second_plane = Model::HISTORY_LEN * 2 + 1 + 2;

  // First phase plane should be all ones
  for (int r = 0; r < 9; ++r) {
    for (int c = 0; c < 9; ++c) {
      EXPECT_EQ(a[bonus_plane][r][c], 0.0f);
      EXPECT_EQ(a[first_plane][r][c], 1.0f);
      EXPECT_EQ(a[second_plane][r][c], 0.0f);
    }
  }
}

// ===== Stone Placement Encoding =====

// Single black stone is encoded correctly
TEST(ModelEncode, SingleBlackStone) {
  Model model(9);
  Board b(9);
  b.apply(Action::place({3, 4}));

  std::deque<Board> history;
  history.push_back(b);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // Current board is at history index HISTORY_LEN-1
  // Black stones plane is 2 * (HISTORY_LEN - 1) = 14
  int black_plane = 2 * (Model::HISTORY_LEN - 1);
  int white_plane = black_plane + 1;

  // Check black stone at (3, 4)
  EXPECT_EQ(a[black_plane][3][4], 1.0f);
  // Check white plane is zero at (3, 4)
  EXPECT_EQ(a[white_plane][3][4], 0.0f);

  // Check no other black stones
  for (int r = 0; r < 9; ++r) {
    for (int c = 0; c < 9; ++c) {
      if (r != 3 || c != 4) {
        EXPECT_EQ(a[black_plane][r][c], 0.0f)
            << "Unexpected black at (" << r << "," << c << ")";
      }
    }
  }
}

// Single white stone is encoded correctly
TEST(ModelEncode, SingleWhiteStone) {
  Model model(9);
  Board b(9);
  b.play_single({3, 3}); // Black plays
  b.play_single({5, 5}); // White plays

  std::deque<Board> history;
  history.push_back(b);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int black_plane = 2 * (Model::HISTORY_LEN - 1);
  int white_plane = black_plane + 1;

  EXPECT_EQ(a[black_plane][3][3], 1.0f);
  EXPECT_EQ(a[white_plane][5][5], 1.0f);
  EXPECT_EQ(a[white_plane][3][3], 0.0f);
  EXPECT_EQ(a[black_plane][5][5], 0.0f);
}

// Multiple stones are encoded correctly
TEST(ModelEncode, MultipleStones) {
  Model model(9);
  Board b(9);
  b.play_single({0, 0}); // B
  b.play_single({0, 1}); // W
  b.play_single({1, 0}); // B
  b.play_single({1, 1}); // W

  std::deque<Board> history;
  history.push_back(b);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int black_plane = 2 * (Model::HISTORY_LEN - 1);
  int white_plane = black_plane + 1;

  EXPECT_EQ(a[black_plane][0][0], 1.0f);
  EXPECT_EQ(a[black_plane][1][0], 1.0f);
  EXPECT_EQ(a[white_plane][0][1], 1.0f);
  EXPECT_EQ(a[white_plane][1][1], 1.0f);
}

// ===== Player Plane Encoding =====

// White to play has player plane all ones
TEST(ModelEncode, WhiteToPlayPlaneOnes) {
  Model model(9);
  Board b(9);
  b.pass(); // Black passes, White to play

  std::deque<Board> history;
  history.push_back(b);

  EXPECT_EQ(history.back().to_play(), Color::White);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int player_plane = Model::HISTORY_LEN * 2;
  for (int r = 0; r < 9; ++r) {
    for (int c = 0; c < 9; ++c) {
      EXPECT_EQ(a[player_plane][r][c], 1.0f);
    }
  }
}

// ===== Phase Encoding =====

// Second phase is encoded correctly
TEST(ModelEncode, SecondPhaseEncoding) {
  Model model(9);
  Board b(9);
  b.apply(Action::place({3, 3})); // Enter Second phase

  std::deque<Board> history;
  history.push_back(b);

  EXPECT_EQ(history.back().phase(), Phase::Second);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int bonus_plane = Model::HISTORY_LEN * 2 + 1 + 0;
  int first_plane = Model::HISTORY_LEN * 2 + 1 + 1;
  int second_plane = Model::HISTORY_LEN * 2 + 1 + 2;

  // Only second phase plane should be ones
  EXPECT_EQ(a[bonus_plane][0][0], 0.0f);
  EXPECT_EQ(a[first_plane][0][0], 0.0f);
  EXPECT_EQ(a[second_plane][0][0], 1.0f);
}

// Bonus phase is encoded correctly
TEST(ModelEncode, BonusPhaseEncoding) {
  Model model(9);
  Board b(9);
  b.apply(Action::place({3, 3})); // First phase -> Second phase
  b.apply(Action::place({4, 4})); // Second phase -> Bonus phase

  std::deque<Board> history;
  history.push_back(b);

  EXPECT_EQ(history.back().phase(), Phase::Bonus);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int bonus_plane = Model::HISTORY_LEN * 2 + 1 + 0;
  int first_plane = Model::HISTORY_LEN * 2 + 1 + 1;
  int second_plane = Model::HISTORY_LEN * 2 + 1 + 2;

  // Only bonus phase plane should be ones
  EXPECT_EQ(a[bonus_plane][0][0], 1.0f);
  EXPECT_EQ(a[first_plane][0][0], 0.0f);
  EXPECT_EQ(a[second_plane][0][0], 0.0f);
}

// ===== History Encoding =====

// Full history is encoded with oldest first
TEST(ModelEncode, FullHistoryEncoding) {
  Model model(9);

  std::deque<Board> history;
  for (int i = 0; i < Model::HISTORY_LEN; ++i) {
    Board b(9);
    // Place a stone at (i, 0) for each board in history
    if (i < 9) {
      b.apply(Action::place({i, 0}));
    }
    history.push_back(b);
  }

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // Check each history position has its stone
  for (int hist = 0; hist < Model::HISTORY_LEN; ++hist) {
    int black_plane = 2 * hist;
    // Board hist has a black stone at (hist, 0)
    if (hist < 9) {
      EXPECT_EQ(a[black_plane][hist][0], 1.0f)
          << "Missing stone at history " << hist;
    }
  }
}

// Less than full history pads with zeros
TEST(ModelEncode, PartialHistoryPadding) {
  Model model(9);

  // Only 3 boards in history
  std::deque<Board> history;
  for (int i = 0; i < 3; ++i) {
    Board b(9);
    b.apply(Action::place({i, 0}));
    history.push_back(b);
  }

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // First 5 history slots (indices 0-4) should be empty (padded)
  for (int hist = 0; hist < Model::HISTORY_LEN - 3; ++hist) {
    int black_plane = 2 * hist;
    int white_plane = black_plane + 1;
    for (int r = 0; r < 9; ++r) {
      for (int c = 0; c < 9; ++c) {
        EXPECT_EQ(a[black_plane][r][c], 0.0f);
        EXPECT_EQ(a[white_plane][r][c], 0.0f);
      }
    }
  }

  // Last 3 history slots (indices 5, 6, 7) should have stones
  // history[0] -> slot 5, history[1] -> slot 6, history[2] -> slot 7
  for (int i = 0; i < 3; ++i) {
    int hist = Model::HISTORY_LEN - 3 + i;
    int black_plane = 2 * hist;
    EXPECT_EQ(a[black_plane][i][0], 1.0f)
        << "Missing stone for history[" << i << "] at slot " << hist;
  }
}

// More than HISTORY_LEN boards uses only recent history
TEST(ModelEncode, ExcessHistoryTruncated) {
  Model model(9);

  // 12 boards in history (more than HISTORY_LEN = 8)
  std::deque<Board> history;
  for (int i = 0; i < 12; ++i) {
    Board b(9);
    if (i < 9) {
      b.apply(Action::place({i, 0}));
    }
    history.push_back(b);
  }

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // Only the last 8 boards should be encoded
  // history[4..11] -> slots 0..7
  // Board at history[4] has stone at (4, 0) -> slot 0
  // Board at history[11] has stone at (8, 0) -> slot 7 (current)
  for (int slot = 0; slot < Model::HISTORY_LEN; ++slot) {
    int hist_index = 4 + slot; // history[4] to history[11]
    int black_plane = 2 * slot;
    if (hist_index < 9) {
      EXPECT_EQ(a[black_plane][hist_index][0], 1.0f)
          << "Missing stone from history[" << hist_index << "] at slot "
          << slot;
    }
  }
}

// ===== Current Board State =====

// Current board state (last in history) is encoded at correct position
TEST(ModelEncode, CurrentBoardAtCorrectSlot) {
  Model model(9);

  std::deque<Board> history;
  Board b(9);
  b.apply(Action::place({8, 8})); // Distinctive position
  history.push_back(b);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // Current board should be at slot HISTORY_LEN - 1 = 7
  int current_black_plane = 2 * (Model::HISTORY_LEN - 1);
  EXPECT_EQ(a[current_black_plane][8][8], 1.0f);
}

// ===== Captures Reflected in Encoding =====

// Captured stones don't appear in encoding
TEST(ModelEncode, CapturedStonesNotEncoded) {
  Model model(9);
  Board b(9);

  // Surround and capture white stone at (1,1)
  b.play_single({0, 1}); // B
  b.play_single({1, 1}); // W (will be captured)
  b.play_single({1, 0}); // B
  b.pass();              // W
  b.play_single({1, 2}); // B
  b.pass();              // W
  b.play_single({2, 1}); // B captures

  std::deque<Board> history;
  history.push_back(b);

  EXPECT_EQ(b.at({1, 1}), Color::Empty);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int black_plane = 2 * (Model::HISTORY_LEN - 1);
  int white_plane = black_plane + 1;

  // (1,1) should be empty in encoding
  EXPECT_EQ(a[black_plane][1][1], 0.0f);
  EXPECT_EQ(a[white_plane][1][1], 0.0f);

  // Surrounding black stones should be present
  EXPECT_EQ(a[black_plane][0][1], 1.0f);
  EXPECT_EQ(a[black_plane][1][0], 1.0f);
  EXPECT_EQ(a[black_plane][1][2], 1.0f);
  EXPECT_EQ(a[black_plane][2][1], 1.0f);
}

// ===== History Shows Capture Progression =====

// History before and after capture shows stone disappearing
TEST(ModelEncode, HistoryShowsCaptureProgression) {
  Model model(9);

  std::deque<Board> history;

  // Build up to capture
  Board b1(9);
  b1.play_single({0, 1}); // B
  b1.play_single({1, 1}); // W
  history.push_back(b1);

  Board b2 = b1;
  b2.play_single({1, 0}); // B
  b2.pass();              // W
  history.push_back(b2);

  Board b3 = b2;
  b3.play_single({1, 2}); // B
  b3.pass();              // W
  history.push_back(b3);

  Board b4 = b3;
  b4.play_single({2, 1}); // B captures
  history.push_back(b4);

  EXPECT_EQ(b3.at({1, 1}), Color::White); // Before capture
  EXPECT_EQ(b4.at({1, 1}), Color::Empty); // After capture

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  // history has 4 boards, filling slots 4, 5, 6, 7
  // Slot 6 (history[2] = b3) should have white stone at (1,1)
  int slot_before = Model::HISTORY_LEN - 2; // slot 6
  int white_plane_before = 2 * slot_before + 1;
  EXPECT_EQ(a[white_plane_before][1][1], 1.0f);

  // Slot 7 (history[3] = b4) should NOT have white stone at (1,1)
  int slot_after = Model::HISTORY_LEN - 1; // slot 7
  int white_plane_after = 2 * slot_after + 1;
  EXPECT_EQ(a[white_plane_after][1][1], 0.0f);
}

// ===== Edge Cases =====

// Corner positions are encoded correctly
TEST(ModelEncode, CornerPositions) {
  Model model(9);
  Board b(9);
  b.apply(Action::place({0, 0}));

  std::deque<Board> history;
  history.push_back(b);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int black_plane = 2 * (Model::HISTORY_LEN - 1);
  EXPECT_EQ(a[black_plane][0][0], 1.0f);
}

// All corners can be encoded
TEST(ModelEncode, AllCorners) {
  Model model(9);
  Board b(9);
  // Black double move at two corners
  b.apply(Action::place({0, 0})); // Black (0,0), -> Second
  b.apply(Action::place({8, 8})); // Black (8,8), -> Bonus, White's turn
  // White double move at other two corners
  b.apply(Action::place({0, 8})); // White (0,8), -> First
  b.apply(Action::place({8, 0})); // White (8,0), -> Second
  b.pass();                       // White passes, -> First, Black's turn

  std::deque<Board> history;
  history.push_back(b);

  auto encoding = model.encode(history);
  auto a = encoding.accessor<float, 3>();

  int black_plane = 2 * (Model::HISTORY_LEN - 1);
  int white_plane = black_plane + 1;

  EXPECT_EQ(a[black_plane][0][0], 1.0f);
  EXPECT_EQ(a[black_plane][8][8], 1.0f);
  EXPECT_EQ(a[white_plane][0][8], 1.0f);
  EXPECT_EQ(a[white_plane][8][0], 1.0f);
}

// ===== Consistency Tests =====

// Same board state produces same encoding
TEST(ModelEncode, DeterministicEncoding) {
  Model model(9);
  Board b(9);
  b.play_single({3, 3});
  b.play_single({4, 4});

  std::deque<Board> history1, history2;
  history1.push_back(b);
  history2.push_back(b);

  auto enc1 = model.encode(history1);
  auto enc2 = model.encode(history2);

  EXPECT_TRUE(torch::equal(enc1, enc2));
}

// Different board states produce different encodings
TEST(ModelEncode, DifferentBoardsDifferentEncodings) {
  Model model(9);

  Board b1(9);
  b1.play_single({3, 3});

  Board b2(9);
  b2.play_single({4, 4});

  std::deque<Board> history1, history2;
  history1.push_back(b1);
  history2.push_back(b2);

  auto enc1 = model.encode(history1);
  auto enc2 = model.encode(history2);

  EXPECT_FALSE(torch::equal(enc1, enc2));
}

// ===== Data Type Tests =====

// Encoding is float tensor
TEST(ModelEncode, FloatTensor) {
  Model model(9);
  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);
  EXPECT_EQ(encoding.dtype(), torch::kFloat32);
}

// Values are 0.0 or 1.0 only
TEST(ModelEncode, BinaryValues) {
  Model model(9);
  Board b(9);
  b.play_single({3, 3});
  b.play_single({5, 5});
  b.apply(Action::place({2, 2})); // Enter second phase

  std::deque<Board> history;
  history.push_back(b);

  auto encoding = model.encode(history);
  auto flat = encoding.flatten();

  auto min_val = flat.min().item<float>();
  auto max_val = flat.max().item<float>();

  EXPECT_GE(min_val, 0.0f);
  EXPECT_LE(max_val, 1.0f);

  // All values should be exactly 0 or 1
  auto zeros_and_ones = (flat == 0.0f) | (flat == 1.0f);
  EXPECT_TRUE(zeros_and_ones.all().item<bool>());
}

// ===== Model Forward Pass Tests =====

// Model can be constructed with default parameters
TEST(ModelForward, Construction) {
  Model model(9);
  EXPECT_EQ(model.board_size, 9);
  EXPECT_EQ(model.num_blocks, 10);
  EXPECT_EQ(model.num_channels, 64);
}

// Model can be constructed with custom parameters
TEST(ModelForward, CustomConstruction) {
  Model model(19, 20, 128);
  EXPECT_EQ(model.board_size, 19);
  EXPECT_EQ(model.num_blocks, 20);
  EXPECT_EQ(model.num_channels, 128);
}

// Forward pass produces correct output shapes
TEST(ModelForward, OutputShapes) {
  Model model(9);
  model.eval();

  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);
  // Add batch dimension
  auto batched = encoding.unsqueeze(0);

  auto [policy, value] = model.forward(batched);

  // Policy: [batch, board_size^2 + 1] = [1, 82]
  EXPECT_EQ(policy.dim(), 2);
  EXPECT_EQ(policy.size(0), 1);
  EXPECT_EQ(policy.size(1), 9 * 9 + 1);

  // Value: [batch, 1] = [1, 1]
  EXPECT_EQ(value.dim(), 2);
  EXPECT_EQ(value.size(0), 1);
  EXPECT_EQ(value.size(1), 1);
}

// Forward pass works with different board sizes
TEST(ModelForward, DifferentBoardSizes) {
  for (int size : {9, 13, 19}) {
    Model model(size, 2, 32); // Smaller model for speed
    model.eval();

    std::deque<Board> history;
    history.push_back(Board(size));

    auto encoding = model.encode(history);
    auto batched = encoding.unsqueeze(0);

    auto [policy, value] = model.forward(batched);

    EXPECT_EQ(policy.size(1), size * size + 1)
        << "Wrong policy size for board " << size;
    EXPECT_EQ(value.size(1), 1);
  }
}

// Forward pass works with batched inputs
TEST(ModelForward, BatchedInput) {
  Model model(9, 2, 32); // Smaller model for speed
  model.eval();

  const int batch_size = 4;
  std::vector<torch::Tensor> encodings;

  for (int i = 0; i < batch_size; ++i) {
    std::deque<Board> history;
    Board b(9);
    b.apply(Action::place({i, i})); // Different positions
    history.push_back(b);
    encodings.push_back(model.encode(history));
  }

  auto batched = torch::stack(encodings);
  EXPECT_EQ(batched.size(0), batch_size);

  auto [policy, value] = model.forward(batched);

  EXPECT_EQ(policy.size(0), batch_size);
  EXPECT_EQ(policy.size(1), 9 * 9 + 1);
  EXPECT_EQ(value.size(0), batch_size);
  EXPECT_EQ(value.size(1), 1);
}

// Value output is in [-1, 1] range (due to tanh)
TEST(ModelForward, ValueRange) {
  Model model(9, 2, 32);
  model.eval();

  // Test with multiple different board states
  for (int i = 0; i < 10; ++i) {
    std::deque<Board> history;
    Board b(9);
    if (i > 0) {
      b.apply(Action::place({i % 9, i % 9}));
    }
    history.push_back(b);

    auto encoding = model.encode(history);
    auto batched = encoding.unsqueeze(0);

    auto [policy, value] = model.forward(batched);

    float v = value.item<float>();
    EXPECT_GE(v, -1.0f) << "Value below -1 at iteration " << i;
    EXPECT_LE(v, 1.0f) << "Value above 1 at iteration " << i;
  }
}

// Policy output contains finite values
TEST(ModelForward, PolicyFinite) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);
  auto batched = encoding.unsqueeze(0);

  auto [policy, value] = model.forward(batched);

  EXPECT_TRUE(torch::isfinite(policy).all().item<bool>());
}

// Value output contains finite values
TEST(ModelForward, ValueFinite) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);
  auto batched = encoding.unsqueeze(0);

  auto [policy, value] = model.forward(batched);

  EXPECT_TRUE(torch::isfinite(value).all().item<bool>());
}

// Model works in eval mode (no batch norm issues with batch size 1)
TEST(ModelForward, EvalModeSingleSample) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);
  auto batched = encoding.unsqueeze(0);

  // Should not throw with batch size 1 in eval mode
  auto result = model.forward(batched);
  EXPECT_TRUE(torch::isfinite(result.first).all().item<bool>());
  EXPECT_TRUE(torch::isfinite(result.second).all().item<bool>());
}

// Model works in train mode
TEST(ModelForward, TrainMode) {
  Model model(9, 2, 32);
  model.train();

  // Need batch size > 1 for batch norm in train mode
  std::vector<torch::Tensor> encodings;
  for (int i = 0; i < 4; ++i) {
    std::deque<Board> history;
    history.push_back(Board(9));
    encodings.push_back(model.encode(history));
  }

  auto batched = torch::stack(encodings);

  auto result = model.forward(batched);
  EXPECT_TRUE(torch::isfinite(result.first).all().item<bool>());
  EXPECT_TRUE(torch::isfinite(result.second).all().item<bool>());
}

// Different inputs produce different outputs
TEST(ModelForward, DifferentInputsDifferentOutputs) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history1, history2;
  Board b1(9);
  Board b2(9);
  b2.apply(Action::place({4, 4})); // Different board state

  history1.push_back(b1);
  history2.push_back(b2);

  auto enc1 = model.encode(history1).unsqueeze(0);
  auto enc2 = model.encode(history2).unsqueeze(0);

  auto [policy1, value1] = model.forward(enc1);
  auto [policy2, value2] = model.forward(enc2);

  // Outputs should differ
  EXPECT_FALSE(torch::equal(policy1, policy2));
}

// Same input produces same output (deterministic in eval mode)
TEST(ModelForward, DeterministicEvalMode) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history).unsqueeze(0);

  auto [policy1, value1] = model.forward(encoding);
  auto [policy2, value2] = model.forward(encoding);

  EXPECT_TRUE(torch::equal(policy1, policy2));
  EXPECT_TRUE(torch::equal(value1, value2));
}

// Model has trainable parameters
TEST(ModelForward, HasParameters) {
  Model model(9, 2, 32);

  int param_count = 0;
  for (const auto& p : model.parameters()) {
    param_count += p.numel();
  }

  EXPECT_GT(param_count, 0);
}

// Model parameters are initialized (not all zeros)
TEST(ModelForward, ParametersInitialized) {
  Model model(9, 2, 32);

  bool found_nonzero = false;
  for (const auto& p : model.parameters()) {
    if (p.abs().sum().item<float>() > 0) {
      found_nonzero = true;
      break;
    }
  }

  EXPECT_TRUE(found_nonzero);
}

// Forward pass with game history
TEST(ModelForward, WithGameHistory) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history;

  // Build up game history
  Board b(9);
  history.push_back(b);

  b.play_single({3, 3});
  history.push_back(b);

  b.play_single({4, 4});
  history.push_back(b);

  b.play_single({5, 5});
  history.push_back(b);

  auto encoding = model.encode(history);
  auto batched = encoding.unsqueeze(0);

  auto [policy, value] = model.forward(batched);

  EXPECT_EQ(policy.size(1), 9 * 9 + 1);
  EXPECT_TRUE(torch::isfinite(policy).all().item<bool>());
  EXPECT_TRUE(torch::isfinite(value).all().item<bool>());
}

// Forward pass with full history length
TEST(ModelForward, FullHistory) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history;

  // Fill history to HISTORY_LEN
  Board b(9);
  for (size_t i = 0; i < Model::HISTORY_LEN; ++i) {
    if (i > 0 && i < 9) {
      b.play_single({static_cast<int>(i), 0});
    }
    history.push_back(b);
  }

  EXPECT_EQ(history.size(), Model::HISTORY_LEN);

  auto encoding = model.encode(history);
  auto batched = encoding.unsqueeze(0);

  auto [policy, value] = model.forward(batched);

  EXPECT_EQ(policy.size(1), 9 * 9 + 1);
  EXPECT_TRUE(torch::isfinite(value).all().item<bool>());
}

// Softmax of policy sums to 1
TEST(ModelForward, PolicySoftmaxSumsToOne) {
  Model model(9, 2, 32);
  model.eval();

  std::deque<Board> history;
  history.push_back(Board(9));

  auto encoding = model.encode(history);
  auto batched = encoding.unsqueeze(0);

  auto [policy, value] = model.forward(batched);
  auto probs = torch::softmax(policy, /*dim=*/1);

  float sum = probs.sum().item<float>();
  EXPECT_NEAR(sum, 1.0f, 1e-5);
}

// Policy has correct number of moves (board_size^2 + 1 for pass)
TEST(ModelForward, PolicyIncludesPass) {
  for (int size : {9, 13, 19}) {
    Model model(size, 2, 32);
    model.eval();

    std::deque<Board> history;
    history.push_back(Board(size));

    auto encoding = model.encode(history);
    auto batched = encoding.unsqueeze(0);

    auto [policy, value] = model.forward(batched);

    // board_size^2 positions + 1 pass move
    int expected_moves = size * size + 1;
    EXPECT_EQ(policy.size(1), expected_moves);
  }
}
