#include <gtest/gtest.h>

#include "double-go/double-go.h"

using namespace double_go;

// Empty board construction
TEST(Board, EmptyBoardConstruction) {
  Board b(9);
  EXPECT_EQ(b.size(), 9);
  EXPECT_EQ(b.to_play(), Color::Black);
  EXPECT_EQ(b.captures(Color::Black), 0);
  EXPECT_EQ(b.captures(Color::White), 0);
  EXPECT_FALSE(b.ko_point().has_value());
  for (int r = 0; r < 9; r++)
    for (int c = 0; c < 9; c++)
      EXPECT_EQ(b.at({r, c}), Color::Empty);
}

// Single stone placement and turn alternation
TEST(Board, PlaceStoneAndAlternate) {
  Board b(9);
  EXPECT_TRUE(b.play_single({3, 3}));
  EXPECT_EQ(b.at({3, 3}), Color::Black);
  EXPECT_EQ(b.to_play(), Color::White);

  EXPECT_TRUE(b.play_single({4, 4}));
  EXPECT_EQ(b.at({4, 4}), Color::White);
  EXPECT_EQ(b.to_play(), Color::Black);

  // Can't play on occupied
  EXPECT_FALSE(b.play_single({3, 3}));
}

// Single-stone capture
TEST(Board, SingleStoneCapture) {
  Board b(9);
  // Surround a white stone at (1,1)
  b.play_single({0, 1}); // B
  b.play_single({1, 1}); // W
  b.play_single({1, 0}); // B
  b.pass();              // W pass
  b.play_single({1, 2}); // B
  b.pass();              // W pass
  b.play_single({2, 1}); // B captures

  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);
}

// Multi-stone group capture
TEST(Board, MultiStoneGroupCapture) {
  Board b(9);
  // White group at (1,1) and (1,2)
  EXPECT_TRUE(b.play_single({0, 1})); // B
  EXPECT_TRUE(b.play_single({1, 1})); // W
  EXPECT_TRUE(b.play_single({0, 2})); // B
  EXPECT_TRUE(b.play_single({1, 2})); // W
  EXPECT_TRUE(b.play_single({1, 0})); // B
  b.pass();                           // W
  EXPECT_TRUE(b.play_single({1, 3})); // B
  b.pass();                           // W
  EXPECT_TRUE(b.play_single({2, 1})); // B
  b.pass();                           // W
  EXPECT_TRUE(b.play_single({2, 2})); // B captures group of 2

  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.at({1, 2}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 2);
}

// Edge/corner captures
TEST(Board, EdgeCapture) {
  Board b(9);
  // White at (0,0), surround with black at (0,1) and (1,0)
  b.play_single({0, 1}); // B
  b.play_single({0, 0}); // W
  b.play_single({1, 0}); // B captures corner

  EXPECT_EQ(b.at({0, 0}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);
}

TEST(Board, EdgeCapture2) {
  Board b(9);
  // White at (0,0) and (0,1), capture on edge
  b.play_single({1, 0}); // B
  b.play_single({0, 0}); // W
  b.play_single({1, 1}); // B
  b.play_single({0, 1}); // W
  b.play_single({0, 2}); // B captures both

  EXPECT_EQ(b.at({0, 0}), Color::Empty);
  EXPECT_EQ(b.at({0, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 2);
}

// Suicide is illegal
TEST(Board, SuicideIllegal) {
  Board b(9);
  // Fill around (0,0) with black stones, try to play white there
  b.play_single({0, 1}); // B
  b.play_single({4, 4}); // W (elsewhere)
  b.play_single({1, 0}); // B
  // Now white tries (0,0) — suicide
  EXPECT_FALSE(b.play_single({0, 0}));
  EXPECT_EQ(b.at({0, 0}), Color::Empty);
}

// Ko — immediate recapture blocked
TEST(Board, KoImmediateRecaptureBlocked) {
  Board k(9);
  // Setup the board:
  //   col:  0  1  2  3
  // row 0:  .  B  W  .
  // row 1:  B  W  .  W
  // row 2:  .  B  W  .
  //
  // W(1,1) has 1 liberty at (1,2). Black plays (1,2), captures W(1,1).
  // After capture: B(1,2) neighbors: (0,2)=W, (1,1)=empty, (1,3)=W, (2,2)=W
  // B(1,2) has 1 liberty at (1,1). Single stone, 1 liberty. Ko!

  k.play_single({0, 1}); // B
  k.play_single({0, 2}); // W
  k.play_single({1, 0}); // B
  k.play_single({1, 3}); // W
  k.play_single({2, 1}); // B
  k.play_single({2, 2}); // W
  k.play_single({8, 8}); // B elsewhere
  k.play_single({1, 1}); // W

  // Now black captures W(1,1) by playing (1,2)
  EXPECT_TRUE(k.play_single({1, 2}));
  EXPECT_EQ(k.at({1, 1}), Color::Empty);
  EXPECT_EQ(k.captures(Color::Black), 1);

  // Ko point should be set at (1,1)
  ASSERT_TRUE(k.ko_point().has_value());
  EXPECT_EQ(k.ko_point()->row, 1);
  EXPECT_EQ(k.ko_point()->col, 1);

  // White cannot immediately recapture at (1,1)
  EXPECT_FALSE(k.is_legal({1, 1}));
  EXPECT_FALSE(k.play_single({1, 1}));
}

// Ko — cleared after a different move
TEST(Board, KoClearedAfterDifferentMove) {
  Board k(9);
  k.play_single({0, 1}); // B
  k.play_single({0, 2}); // W
  k.play_single({1, 0}); // B
  k.play_single({1, 3}); // W
  k.play_single({2, 1}); // B
  k.play_single({2, 2}); // W
  k.play_single({8, 8}); // B elsewhere
  k.play_single({1, 1}); // W
  k.play_single({1, 2}); // B captures, ko at (1,1)

  ASSERT_TRUE(k.ko_point().has_value());

  // White plays elsewhere
  k.play_single({7, 7});

  // Ko cleared
  EXPECT_FALSE(k.ko_point().has_value());
}

// Ko — cleared after pass
TEST(Board, KoClearedAfterPass) {
  Board k(9);
  k.play_single({0, 1}); // B
  k.play_single({0, 2}); // W
  k.play_single({1, 0}); // B
  k.play_single({1, 3}); // W
  k.play_single({2, 1}); // B
  k.play_single({2, 2}); // W
  k.play_single({8, 8}); // B elsewhere
  k.play_single({1, 1}); // W
  k.play_single({1, 2}); // B captures, ko at (1,1)

  ASSERT_TRUE(k.ko_point().has_value());

  k.pass(); // White passes

  EXPECT_FALSE(k.ko_point().has_value());
}

// Not ko when capturing stone has multiple liberties
TEST(Board, NotKoMultipleLiberties) {
  Board b(9);
  // W(1,1) has 1 liberty at (0,1). B plays (0,1) captures W(1,1).
  // B(0,1) has 3 liberties: (0,0), (0,2), (1,1). Single stone, 3 libs. Not ko.
  //
  //   col: 0  1  2
  // row0:  .  *  .      ← B plays here
  // row1:  B  W  B
  // row2:  .  B  .

  b.play_single({1, 0}); // B
  b.play_single({1, 1}); // W
  b.play_single({1, 2}); // B
  b.play_single({8, 8}); // W elsewhere
  b.play_single({2, 1}); // B
  b.play_single({8, 7}); // W elsewhere

  EXPECT_TRUE(b.play_single({0, 1})); // B captures W(1,1)
  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);

  // Not a ko because capturing stone has 3 liberties (need exactly 1 for ko)
  EXPECT_FALSE(b.ko_point().has_value());
}

// legal_moves returns correct set
TEST(Board, LegalMoves) {
  Board b(9);
  auto moves = b.legal_moves();
  EXPECT_EQ(static_cast<int>(moves.size()), 81); // 9x9 = 81

  b.play_single({0, 0});
  moves = b.legal_moves();
  EXPECT_EQ(static_cast<int>(moves.size()), 80); // one occupied
}

// Multiple simultaneous captures
TEST(Board, MultipleSimultaneousCaptures) {
  Board b(9);
  // Two separate W stones, each with 1 liberty at the same point (1,2).
  // B plays (1,2) to capture both simultaneously.
  //
  //   col:  0  1  2  3  4
  // row 0:  .  B  .  B  .
  // row 1:  B  W  .  W  B
  // row 2:  .  B  .  B  .
  //
  // W(1,1) neighbors: (0,1)=B, (1,0)=B, (1,2)=empty, (2,1)=B → 1 lib
  // W(1,3) neighbors: (0,3)=B, (1,2)=empty, (1,4)=B, (2,3)=B → 1 lib

  b.play_single({0, 1}); // B  (1)
  b.play_single({1, 1}); // W  (2)
  b.play_single({1, 0}); // B  (3)
  b.play_single({1, 3}); // W  (4)
  b.play_single({0, 3}); // B  (5)
  b.play_single({8, 8}); // W elsewhere (6)
  b.play_single({1, 4}); // B  (7)
  b.play_single({8, 7}); // W elsewhere (8)
  b.play_single({2, 1}); // B  (9)
  b.play_single({8, 6}); // W elsewhere (10)
  b.play_single({2, 3}); // B  (11)
  b.play_single({8, 5}); // W elsewhere (12)

  // Black captures both W(1,1) and W(1,3) by playing (1,2)
  EXPECT_TRUE(b.play_single({1, 2}));
  EXPECT_EQ(b.captures(Color::Black), 2);
  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.at({1, 3}), Color::Empty);

  // Not ko (captured 2, not 1)
  EXPECT_FALSE(b.ko_point().has_value());
}

// ===== Double Move Tests =====

// Double move places two same-color stones, turn flips once
TEST(DoubleMove, PlacesTwoSameColorStones) {
  Board b(9);
  EXPECT_TRUE(b.apply(Action::place({3, 3})));
  EXPECT_EQ(b.at({3, 3}), Color::Black);
  EXPECT_EQ(b.to_play(), Color::Black); // still Black's turn
  EXPECT_EQ(b.phase(), Phase::Second);

  EXPECT_TRUE(b.apply(Action::place({4, 4})));
  EXPECT_EQ(b.at({4, 4}), Color::Black);
  EXPECT_EQ(b.to_play(), Color::White); // now White's turn
  EXPECT_EQ(b.phase(), Phase::Bonus);
}

TEST(DoubleMove, BonusAfterDoubleMove) {
  Board b(9);
  b.apply(Action::place({3, 3}));
  b.apply(Action::place({4, 4}));
  EXPECT_EQ(b.phase(), Phase::Bonus);
  // White plays
  b.apply(Action::place({5, 5}));
  EXPECT_EQ(b.phase(), Phase::First);
  b.apply(Action::place({6, 6}));
  EXPECT_EQ(b.phase(), Phase::Second);
}

// Forced pass clears must_pass, player can play normally
TEST(DoubleMove, ForcedPassClearsMustPass) {
  Board b(9);
  b.apply(Action::place({3, 3}));
  b.apply(Action::place({4, 4}));

  b.play_single({5, 5}); // White
  b.play_single({6, 6}); // White
  b.pass();

  // Black's turn again, must_pass should be cleared
  EXPECT_EQ(b.phase(), Phase::First);
}

// Double move captures counted from both stones
TEST(DoubleMove, CapturesFromBothStones) {
  Board b(9);
  // Setup: White stone at (0,0), surround with black double move
  // Place white at corner (0,0)
  b.play_single({0, 1}); // B
  b.play_single({0, 0}); // W
  // Black plays double move to capture: first (1,0) then something else
  // (0,0) is surrounded by B(0,1) and after B(1,0)
  b.apply(Action::place({1, 0})); // B captures W(0,0)
  EXPECT_EQ(b.at({0, 0}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);

  // Now set up another capture scenario for second stone
  // Place another white stone to capture with second move
  // W(2,0) surrounded by B(1,0) already placed, need B(2,1) and B(3,0)
  // For simplicity, just place second stone normally
  b.apply(Action::place({5, 5}));
  EXPECT_EQ(b.captures(Color::Black), 1); // only 1 capture total
  EXPECT_EQ(b.to_play(), Color::White);
}

// Illegal Place in Second phase preserves state
TEST(DoubleMove, IllegalPlaceInSecondPhasePreservesState) {
  Board b(9);
  b.apply(Action::place({3, 3}));

  // Try to place on occupied square
  EXPECT_FALSE(b.apply(Action::place({3, 3})));
  EXPECT_EQ(b.phase(), Phase::Second);
  EXPECT_EQ(b.to_play(), Color::Black);
  EXPECT_EQ(b.at({3, 3}), Color::Black);

  // Can still complete with a legal move
  EXPECT_TRUE(b.apply(Action::place({4, 4})));
  EXPECT_EQ(b.phase(), Phase::Bonus);
}

// Ko from first stone doesn't block second stone at ko point
TEST(DoubleMove, KoFromFirstStoneDoesNotBlockSecond) {
  Board b(9);
  // Setup ko pattern, then use double move to trigger it
  // Standard ko setup:
  //   col:  0  1  2  3
  // row 0:  .  B  W  .
  // row 1:  B  W  .  W
  // row 2:  .  B  W  .
  b.play_single({0, 1}); // B
  b.play_single({0, 2}); // W
  b.play_single({1, 0}); // B
  b.play_single({1, 3}); // W
  b.play_single({2, 1}); // B
  b.play_single({2, 2}); // W
  b.play_single({8, 8}); // B elsewhere
  b.play_single({1, 1}); // W

  // Black uses double move: first stone captures at (1,2) creating ko
  b.apply(Action::place({1, 2}));
  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);
  ASSERT_TRUE(b.ko_point().has_value());
  EXPECT_EQ(*b.ko_point(), (Point{1, 1}));

  // Second stone can fill ko
  EXPECT_TRUE(b.apply(Action::place({1, 1})));
}

// Both players use double moves independently
TEST(DoubleMove, BothPlayersDoubleMove) {
  Board b(9);
  // Black double move
  b.apply(Action::place({3, 3}));
  b.apply(Action::place({3, 4}));
  EXPECT_EQ(b.at({3, 3}), Color::Black);
  EXPECT_EQ(b.at({3, 4}), Color::Black);

  // White double move, pass
  b.apply(Action::place({5, 5}));
  b.apply(Action::place({5, 6}));
  b.pass();
  EXPECT_EQ(b.at({5, 5}), Color::White);
  EXPECT_EQ(b.at({5, 6}), Color::White);

  // Both can play normally now
  EXPECT_EQ(b.to_play(), Color::Black);
  EXPECT_EQ(b.phase(), Phase::First);
  EXPECT_TRUE(b.play_single({7, 7}));
}

// legal_actions in Second phase returns Pass + Place actions
TEST(DoubleMove, LegalActionsInSecondPhase) {
  Board b(9);
  b.apply(Action::place({0, 0}));
  EXPECT_EQ(b.phase(), Phase::Second);

  auto actions = b.legal_actions();
  EXPECT_FALSE(actions.empty());

  bool has_pass = false, has_place = false;
  for (const auto &a : actions) {
    if (a.type == ActionType::Pass)
      has_pass = true;
    if (a.type == ActionType::Place)
      has_place = true;
  }
  EXPECT_TRUE(has_pass);
  EXPECT_TRUE(has_place);
}

// Pass during Second phase completes as single move
TEST(DoubleMove, PassDuringSecondPhaseCompletesAsSingleMove) {
  Board b(9);
  b.apply(Action::place({3, 3}));
  EXPECT_EQ(b.phase(), Phase::Second);
  EXPECT_TRUE(b.apply(Action::pass()));
  EXPECT_EQ(b.phase(), Phase::First);
  EXPECT_EQ(b.to_play(), Color::White);
  b.pass(); // White passes
  EXPECT_EQ(b.to_play(), Color::Black);
  EXPECT_EQ(b.phase(), Phase::First);
}

// First phase legal_actions includes Pass and Place
TEST(DoubleMove, FirstPhaseLegalActionsIncludePassAndPlace) {
  Board b(9);
  auto actions = b.legal_actions();

  bool has_pass = false, has_place = false;
  for (const auto &a : actions) {
    if (a.type == ActionType::Pass)
      has_pass = true;
    if (a.type == ActionType::Place)
      has_place = true;
  }
  EXPECT_TRUE(has_pass);
  EXPECT_TRUE(has_place);

  // Count: 1 Pass + 81 Place = 82 on empty 9x9
  EXPECT_EQ(actions.size(), 82u);
}

// Taking a ko on the second move correctly sets the ko point, and
// the forced pass resets the ko.

TEST(DoubleMove, SecondMoveKo) {
  Board b(9);

  // Setup ko pattern, then use second double move to trigger it
  // Standard ko setup:
  //   col:  0  1  2  3
  // row 0:  .  B  W  .
  // row 1:  B  W  .  W
  // row 2:  .  B  W  .
  b.play_single({0, 1}); // B
  b.play_single({0, 2}); // W
  b.play_single({1, 0}); // B
  b.play_single({1, 3}); // W
  b.play_single({2, 1}); // B
  b.play_single({2, 2}); // W
  b.play_single({8, 8}); // B elsewhere
  b.play_single({1, 1}); // W

  b.apply(Action::place({7, 7}));
  b.apply(Action::place({1, 2})); // B take ko

  EXPECT_FALSE(b.apply(Action::place({1, 1}))); // W can't retake
  EXPECT_TRUE(b.apply(Action::place({6, 6})));  // W play away
  EXPECT_TRUE(b.apply(Action::place({1, 1})));
}

// ===== Scoring Tests =====

// Empty board has no territory for either side
TEST(Scoring, EmptyBoardScore) {
  Board b(9);
  auto sr = b.score(6.5);
  EXPECT_EQ(sr.black_stones, 0);
  EXPECT_EQ(sr.white_stones, 0);
  EXPECT_EQ(sr.black_territory, 0);
  EXPECT_EQ(sr.white_territory, 0);
  EXPECT_DOUBLE_EQ(sr.black_score, 0.0);
  EXPECT_DOUBLE_EQ(sr.white_score, 6.5);
}

// Full black board (last point is suicide, so 80 stones + 1 territory)
TEST(Scoring, FullBlackBoard) {
  Board b(9);
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      b.play_single({r, c}); // Black (last one fails — suicide)
      if (r * 9 + c < 80)
        b.pass(); // White pass
    }
  }
  auto sr = b.score(0.0);
  // Last corner is suicide, so 80 stones + 1 empty point owned by black
  EXPECT_EQ(sr.black_stones, 80);
  EXPECT_EQ(sr.white_stones, 0);
  EXPECT_EQ(sr.black_territory, 1);
  EXPECT_DOUBLE_EQ(sr.black_score, 81.0);
}

// Simple territory — black owns top, white owns bottom
TEST(Scoring, SimpleTerritory) {
  Board b(9);
  // Black wall along row 2, white wall along row 6
  for (int c = 0; c < 9; c++) {
    b.play_single({2, c}); // Black
    b.play_single({6, c}); // White
  }
  auto sr = b.score(0.0);
  // Rows 0-1 enclosed by black (18 pts), rows 7-8 enclosed by white (18 pts)
  // Rows 3-5 bordered by both → dame
  EXPECT_EQ(sr.black_territory, 18);
  EXPECT_EQ(sr.white_territory, 18);
  EXPECT_EQ(sr.black_stones, 9);
  EXPECT_EQ(sr.white_stones, 9);
  EXPECT_DOUBLE_EQ(sr.black_score, 27.0);
  EXPECT_DOUBLE_EQ(sr.white_score, 27.0);
}

// Neutral territory — empty region bordered by both colors
TEST(Scoring, NeutralTerritory) {
  Board b(9);
  // Black at (0,0), White at (0,2). The point (0,1) borders both — neutral.
  b.play_single({0, 0}); // B
  b.play_single({0, 2}); // W
  auto sr = b.score(0.0);
  // (0,1) is in a region that borders both colors — dame
  // The rest of the empty board also borders both colors
  EXPECT_EQ(sr.black_territory, 0);
  EXPECT_EQ(sr.white_territory, 0);
}

// Komi is added to white's score
TEST(Scoring, ScoreWithKomi) {
  Board b(9);
  auto sr = b.score(6.5);
  EXPECT_DOUBLE_EQ(sr.white_score, 6.5);
  EXPECT_DOUBLE_EQ(sr.black_score, 0.0);

  auto sr2 = b.score(7.5);
  EXPECT_DOUBLE_EQ(sr2.white_score, 7.5);
}

// Place is accepted even when no second move is possible (can pass instead)
TEST(DoubleMove, PlaceAcceptedWhenNoSecondMove) {
  // 4x4 board with Black at (0,1) and (1,0), White everywhere else
  // except (0,0), (1,1), (1,3), (2,2) which are empty.
  //
  //   .  B  W  W
  //   B  .  W  .
  //   W  W  .  W
  //   W  W  W  W
  //
  // (0,0) and (1,1) are the only legal points for Black (the others are
  // suicide). After placing at either one, the B group has only 1 liberty
  // and the remaining empty points are all suicide — no legal second move.
  // But in the new model, Place IS accepted because player can pass to
  // complete as a single move.
  Board b(4);
  b.play_single({0, 1}); // B
  b.play_single({3, 0}); // W
  b.play_single({1, 0}); // B
  b.play_single({3, 1}); // W
  b.pass();              // B
  b.play_single({3, 2}); // W
  b.pass();              // B
  b.play_single({3, 3}); // W
  b.pass();              // B
  b.play_single({2, 3}); // W
  b.pass();              // B
  b.play_single({2, 0}); // W
  b.pass();              // B
  b.play_single({2, 1}); // W
  b.pass();              // B
  b.play_single({0, 2}); // W
  b.pass();              // B
  b.play_single({0, 3}); // W
  b.pass();              // B
  b.play_single({1, 2}); // W

  // Verify board state
  EXPECT_EQ(b.to_play(), Color::Black);
  EXPECT_EQ(b.at({0, 0}), Color::Empty);
  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.at({1, 3}), Color::Empty);
  EXPECT_EQ(b.at({2, 2}), Color::Empty);
  EXPECT_EQ(b.at({0, 1}), Color::Black);
  EXPECT_EQ(b.at({1, 0}), Color::Black);

  // (0,0) and (1,1) are legal as moves
  EXPECT_TRUE(b.is_legal({0, 0}));
  EXPECT_TRUE(b.is_legal({1, 1}));
  // (1,3) and (2,2) are suicide
  EXPECT_FALSE(b.is_legal({1, 3}));
  EXPECT_FALSE(b.is_legal({2, 2}));

  // Place IS accepted (enters Second phase)
  EXPECT_TRUE(b.apply(Action::place({0, 0})));
  EXPECT_EQ(b.phase(), Phase::Second);

  // Pass to complete as single move
  b.apply(Action::pass());
  EXPECT_EQ(b.phase(), Phase::First);
  EXPECT_EQ(b.to_play(), Color::White);
}

// ===== Game Over Tests =====

// Two consecutive passes end the game
TEST(GameOver, ConsecutivePassesEndGame) {
  Board b(9);
  EXPECT_FALSE(b.game_over());
  b.pass(); // Black passes
  EXPECT_FALSE(b.game_over());
  b.pass(); // White passes
  EXPECT_TRUE(b.game_over());
  EXPECT_EQ(b.consecutive_passes(), 2);
}

// A move resets consecutive passes
TEST(GameOver, MoveResetsConsecutivePasses) {
  Board b(9);
  b.pass(); // Black passes
  EXPECT_EQ(b.consecutive_passes(), 1);
  b.play_single({4, 4}); // White plays
  EXPECT_EQ(b.consecutive_passes(), 0);
  EXPECT_FALSE(b.game_over());
}

// Double move resets consecutive passes
TEST(GameOver, DoubleMoveThenPass) {
  Board b(9);
  b.pass(); // Black passes
  EXPECT_EQ(b.consecutive_passes(), 1);

  // White does a double move — should reset consecutive passes
  b.apply(Action::place({3, 3}));
  EXPECT_EQ(b.consecutive_passes(), 0);

  b.apply(Action::place({4, 4}));
  EXPECT_EQ(b.consecutive_passes(), 0);
  EXPECT_FALSE(b.game_over());
}

// Forced pass after double move does not count toward game end
TEST(GameOver, ForcedPassDoesNotEndGame) {
  Board b(9);
  // Black double moves
  b.apply(Action::place({3, 3}));
  b.apply(Action::place({4, 4}));
  // White's turn — white passes voluntarily
  b.pass();
  EXPECT_EQ(b.consecutive_passes(), 1);
  EXPECT_FALSE(b.game_over());
}

// Forced pass followed by voluntary pass does not end game
TEST(GameOver, ForcedThenVoluntaryPassDoesNotEndGame) {
  Board b(9);
  b.apply(Action::place({3, 3}));
  b.apply(Action::place({4, 4}));
  // White plays normally
  b.play_single({5, 5});
  EXPECT_EQ(b.consecutive_passes(), 0);
  // White passes voluntarily
  b.pass();
  EXPECT_EQ(b.consecutive_passes(), 1);
  // Only one voluntary pass — game NOT over
  EXPECT_FALSE(b.game_over());
}

// ===== RandomBot Tests =====

// RandomBot always returns a legal action
TEST(RandomBot, ReturnsLegalAction) {
  Board b(9);
  RandomBot bot(42);
  for (int i = 0; i < 50 && !b.game_over(); ++i) {
    auto action = bot.pick_action(b);
    auto legal = b.legal_actions();
    EXPECT_NE(std::find(legal.begin(), legal.end(), action), legal.end());
    b.apply(action);
  }
}

// RandomBot plays a full game until game_over in <1000 moves
TEST(RandomBot, PlaysFullGame) {
  Board b(9);
  RandomBot bot(123);
  int moves = 0;
  while (!b.game_over() && moves < 1000) {
    b.apply(bot.pick_action(b));
    ++moves;
  }
  EXPECT_TRUE(b.game_over());
  EXPECT_LT(moves, 1000);
}

// Different seeds produce different move sequences
TEST(RandomBot, DifferentSeedsDifferentGames) {
  auto play_game = [](unsigned seed) {
    Board b(9);
    RandomBot bot(seed);
    std::vector<Action> history;
    for (int i = 0; i < 20 && !b.game_over(); ++i) {
      auto a = bot.pick_action(b);
      history.push_back(a);
      b.apply(a);
    }
    return history;
  };

  auto game1 = play_game(42);
  auto game2 = play_game(99);
  EXPECT_NE(game1, game2);
}

// Same seed produces the same game (deterministic)
TEST(RandomBot, DeterministicWithSameSeed) {
  auto play_game = [](unsigned seed) {
    Board b(9);
    RandomBot bot(seed);
    std::vector<Action> history;
    for (int i = 0; i < 50 && !b.game_over(); ++i) {
      auto a = bot.pick_action(b);
      history.push_back(a);
      b.apply(a);
    }
    return history;
  };

  auto game1 = play_game(42);
  auto game2 = play_game(42);
  EXPECT_EQ(game1, game2);
}

// ===== New Tests =====

// Pass as second place preserves ko from first stone
TEST(DoubleMove, PassAsSecondPlacePreservesKo) {
  Board b(9);
  // Standard ko setup
  b.play_single({0, 1}); // B
  b.play_single({0, 2}); // W
  b.play_single({1, 0}); // B
  b.play_single({1, 3}); // W
  b.play_single({2, 1}); // B
  b.play_single({2, 2}); // W
  b.play_single({8, 8}); // B elsewhere
  b.play_single({1, 1}); // W

  // Black places first stone capturing at (1,2), creating ko at (1,1)
  b.apply(Action::place({1, 2}));
  ASSERT_TRUE(b.ko_point().has_value());
  EXPECT_EQ(*b.ko_point(), (Point{1, 1}));

  // Pass to complete as single move — ko should be preserved
  b.apply(Action::pass());
  ASSERT_TRUE(b.ko_point().has_value());
  EXPECT_EQ(*b.ko_point(), (Point{1, 1}));

  // Opponent can't play at ko point
  EXPECT_FALSE(b.is_legal({1, 1}));
}

// Single move (place + pass) incurs no penalty
TEST(DoubleMove, SingleMoveNoPenalty) {
  Board b(9);
  // Black places then passes (single move)
  b.apply(Action::place({3, 3}));
  b.apply(Action::pass());
  EXPECT_EQ(b.to_play(), Color::White);
  EXPECT_EQ(b.phase(), Phase::First);
  // White plays
  b.play_single({5, 5});
}
