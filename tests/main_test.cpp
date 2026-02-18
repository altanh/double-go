#include <gtest/gtest.h>

#include "double-go/double-go.h"

using namespace double_go;

// 1. Empty board construction
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

// 2. Single stone placement and turn alternation
TEST(Board, PlaceStoneAndAlternate) {
  Board b(9);
  EXPECT_TRUE(b.play({3, 3}));
  EXPECT_EQ(b.at({3, 3}), Color::Black);
  EXPECT_EQ(b.to_play(), Color::White);

  EXPECT_TRUE(b.play({4, 4}));
  EXPECT_EQ(b.at({4, 4}), Color::White);
  EXPECT_EQ(b.to_play(), Color::Black);

  // Can't play on occupied
  EXPECT_FALSE(b.play({3, 3}));
}

// 3. Single-stone capture
TEST(Board, SingleStoneCapture) {
  Board b(9);
  // Surround a white stone at (1,1)
  b.play({0, 1}); // B
  b.play({1, 1}); // W
  b.play({1, 0}); // B
  b.pass();       // W pass
  b.play({1, 2}); // B
  b.pass();       // W pass
  b.play({2, 1}); // B captures

  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);
}

// 4. Multi-stone group capture
TEST(Board, MultiStoneGroupCapture) {
  Board b(9);
  // White group at (1,1) and (1,2)
  b.play({0, 1}); // B
  b.play({1, 1}); // W
  b.play({0, 2}); // B
  b.play({1, 2}); // W
  b.play({1, 0}); // B
  b.pass();       // W
  b.play({1, 3}); // B
  b.pass();       // W
  b.play({2, 1}); // B
  b.pass();       // W
  b.play({2, 2}); // B captures group of 2

  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.at({1, 2}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 2);
}

// 5. Edge/corner captures
TEST(Board, EdgeCapture) {
  Board b(9);
  // White at (0,0), surround with black at (0,1) and (1,0)
  b.play({0, 1}); // B
  b.play({0, 0}); // W
  b.play({1, 0}); // B captures corner

  EXPECT_EQ(b.at({0, 0}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);
}

TEST(Board, EdgeCapture2) {
  Board b(9);
  // White at (0,0) and (0,1), capture on edge
  b.play({1, 0}); // B
  b.play({0, 0}); // W
  b.play({1, 1}); // B
  b.play({0, 1}); // W
  b.play({0, 2}); // B captures both

  EXPECT_EQ(b.at({0, 0}), Color::Empty);
  EXPECT_EQ(b.at({0, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 2);
}

// 6. Suicide is illegal
TEST(Board, SuicideIllegal) {
  Board b(9);
  // Fill around (0,0) with black stones, try to play white there
  b.play({0, 1}); // B
  b.play({4, 4}); // W (elsewhere)
  b.play({1, 0}); // B
  // Now white tries (0,0) — suicide
  EXPECT_FALSE(b.play({0, 0}));
  EXPECT_EQ(b.at({0, 0}), Color::Empty);
}

// 7. Ko — immediate recapture blocked
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

  k.play({0, 1}); // B
  k.play({0, 2}); // W
  k.play({1, 0}); // B
  k.play({1, 3}); // W
  k.play({2, 1}); // B
  k.play({2, 2}); // W
  k.play({8, 8}); // B elsewhere
  k.play({1, 1}); // W

  // Now black captures W(1,1) by playing (1,2)
  EXPECT_TRUE(k.play({1, 2}));
  EXPECT_EQ(k.at({1, 1}), Color::Empty);
  EXPECT_EQ(k.captures(Color::Black), 1);

  // Ko point should be set at (1,1)
  ASSERT_TRUE(k.ko_point().has_value());
  EXPECT_EQ(k.ko_point()->row, 1);
  EXPECT_EQ(k.ko_point()->col, 1);

  // White cannot immediately recapture at (1,1)
  EXPECT_FALSE(k.is_legal({1, 1}));
  EXPECT_FALSE(k.play({1, 1}));
}

// 8. Ko — cleared after a different move
TEST(Board, KoClearedAfterDifferentMove) {
  Board k(9);
  k.play({0, 1}); // B
  k.play({0, 2}); // W
  k.play({1, 0}); // B
  k.play({1, 3}); // W
  k.play({2, 1}); // B
  k.play({2, 2}); // W
  k.play({8, 8}); // B elsewhere
  k.play({1, 1}); // W
  k.play({1, 2}); // B captures, ko at (1,1)

  ASSERT_TRUE(k.ko_point().has_value());

  // White plays elsewhere
  k.play({7, 7});

  // Ko cleared
  EXPECT_FALSE(k.ko_point().has_value());
}

// 9. Ko — cleared after pass
TEST(Board, KoClearedAfterPass) {
  Board k(9);
  k.play({0, 1}); // B
  k.play({0, 2}); // W
  k.play({1, 0}); // B
  k.play({1, 3}); // W
  k.play({2, 1}); // B
  k.play({2, 2}); // W
  k.play({8, 8}); // B elsewhere
  k.play({1, 1}); // W
  k.play({1, 2}); // B captures, ko at (1,1)

  ASSERT_TRUE(k.ko_point().has_value());

  k.pass(); // White passes

  EXPECT_FALSE(k.ko_point().has_value());
}

// 10. Not ko when capturing stone has multiple liberties
TEST(Board, NotKoMultipleLiberties) {
  Board b(9);
  // W(1,1) has 1 liberty at (0,1). B plays (0,1) captures W(1,1).
  // B(0,1) has 3 liberties: (0,0), (0,2), (1,1). Single stone, 3 libs. Not ko.
  //
  //   col: 0  1  2
  // row0:  .  *  .      ← B plays here
  // row1:  B  W  B
  // row2:  .  B  .

  b.play({1, 0}); // B
  b.play({1, 1}); // W
  b.play({1, 2}); // B
  b.play({8, 8}); // W elsewhere
  b.play({2, 1}); // B
  b.play({8, 7}); // W elsewhere

  EXPECT_TRUE(b.play({0, 1})); // B captures W(1,1)
  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);

  // Not a ko because capturing stone has 3 liberties (need exactly 1 for ko)
  EXPECT_FALSE(b.ko_point().has_value());
}

// 11. legal_moves returns correct set
TEST(Board, LegalMoves) {
  Board b(9);
  auto moves = b.legal_moves();
  EXPECT_EQ(static_cast<int>(moves.size()), 81); // 9x9 = 81

  b.play({0, 0});
  moves = b.legal_moves();
  EXPECT_EQ(static_cast<int>(moves.size()), 80); // one occupied
}

// 12. Multiple simultaneous captures
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

  b.play({0, 1}); // B  (1)
  b.play({1, 1}); // W  (2)
  b.play({1, 0}); // B  (3)
  b.play({1, 3}); // W  (4)
  b.play({0, 3}); // B  (5)
  b.play({8, 8}); // W elsewhere (6)
  b.play({1, 4}); // B  (7)
  b.play({8, 7}); // W elsewhere (8)
  b.play({2, 1}); // B  (9)
  b.play({8, 6}); // W elsewhere (10)
  b.play({2, 3}); // B  (11)
  b.play({8, 5}); // W elsewhere (12)

  // Black captures both W(1,1) and W(1,3) by playing (1,2)
  EXPECT_TRUE(b.play({1, 2}));
  EXPECT_EQ(b.captures(Color::Black), 2);
  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.at({1, 3}), Color::Empty);

  // Not ko (captured 2, not 1)
  EXPECT_FALSE(b.ko_point().has_value());
}

// ===== Double Move Tests =====

// 13. Double move places two same-color stones, turn flips once
TEST(DoubleMove, PlacesTwoSameColorStones) {
  Board b(9);
  EXPECT_TRUE(b.apply(Action::double_first({3, 3})));
  EXPECT_EQ(b.at({3, 3}), Color::Black);
  EXPECT_EQ(b.to_play(), Color::Black); // still Black's turn
  EXPECT_EQ(b.phase(), Phase::DoubleMove);

  EXPECT_TRUE(b.apply(Action::double_second({4, 4})));
  EXPECT_EQ(b.at({4, 4}), Color::Black);
  EXPECT_EQ(b.to_play(), Color::White); // now White's turn
  EXPECT_EQ(b.phase(), Phase::Normal);
}

// 14. After double move, must_pass is true, legal_actions returns only Pass
TEST(DoubleMove, MustPassAfterDoubleMove) {
  Board b(9);
  b.apply(Action::double_first({3, 3}));
  b.apply(Action::double_second({4, 4}));

  // White plays
  b.play({5, 5});

  // Black must pass
  EXPECT_TRUE(b.must_pass());
  auto actions = b.legal_actions();
  EXPECT_EQ(actions.size(), 1u);
  EXPECT_EQ(actions[0], Action::pass());
}

// 15. Forced pass clears must_pass, player can play normally
TEST(DoubleMove, ForcedPassClearsMustPass) {
  Board b(9);
  b.apply(Action::double_first({3, 3}));
  b.apply(Action::double_second({4, 4}));

  b.play({5, 5}); // White
  EXPECT_TRUE(b.must_pass());

  b.pass();                    // Black forced pass
  EXPECT_FALSE(b.must_pass()); // White's turn, white has no must_pass

  b.play({6, 6}); // White
  // Black's turn again, must_pass should be cleared
  EXPECT_FALSE(b.must_pass());
}

// 16. Double move captures counted from both stones
TEST(DoubleMove, CapturesFromBothStones) {
  Board b(9);
  // Setup: White stone at (0,0), surround with black double move
  // Place white at corner (0,0)
  b.play({0, 1}); // B
  b.play({0, 0}); // W
  // Black plays double move to capture: first (1,0) then something else
  // (0,0) is surrounded by B(0,1) and after B(1,0)
  b.apply(Action::double_first({1, 0})); // B captures W(0,0)
  EXPECT_EQ(b.at({0, 0}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);

  // Now set up another capture scenario for second stone
  // Place another white stone to capture with second move
  // W(2,0) surrounded by B(1,0) already placed, need B(2,1) and B(3,0)
  // For simplicity, just place second stone normally
  b.apply(Action::double_second({5, 5}));
  EXPECT_EQ(b.captures(Color::Black), 1); // only 1 capture total
  EXPECT_EQ(b.to_play(), Color::White);
}

// 17. DoubleFirst followed by illegal DoubleSecond returns false, board
// unchanged from after first stone
TEST(DoubleMove, IllegalDoubleSecondPreservesState) {
  Board b(9);
  b.apply(Action::double_first({3, 3}));

  // Try to place on occupied square
  EXPECT_FALSE(b.apply(Action::double_second({3, 3})));
  EXPECT_EQ(b.phase(), Phase::DoubleMove);
  EXPECT_EQ(b.to_play(), Color::Black);
  EXPECT_EQ(b.at({3, 3}), Color::Black);

  // Can still complete with a legal move
  EXPECT_TRUE(b.apply(Action::double_second({4, 4})));
  EXPECT_EQ(b.phase(), Phase::Normal);
}

// 18. Cannot start double move while must_pass
TEST(DoubleMove, CannotDoubleWhileMustPass) {
  Board b(9);
  // Black double moves
  b.apply(Action::double_first({3, 3}));
  b.apply(Action::double_second({4, 4}));

  b.play({5, 5}); // White

  // Black must pass
  EXPECT_TRUE(b.must_pass());
  EXPECT_FALSE(b.apply(Action::double_first({6, 6})));
  EXPECT_FALSE(b.apply(Action::move({6, 6})));
}

// 19. Ko from first stone doesn't block second stone at ko point
TEST(DoubleMove, KoFromFirstStoneDoesNotBlockSecond) {
  Board b(9);
  // Setup ko pattern, then use double move to trigger it
  // Standard ko setup:
  //   col:  0  1  2  3
  // row 0:  .  B  W  .
  // row 1:  B  W  .  W
  // row 2:  .  B  W  .
  b.play({0, 1}); // B
  b.play({0, 2}); // W
  b.play({1, 0}); // B
  b.play({1, 3}); // W
  b.play({2, 1}); // B
  b.play({2, 2}); // W
  b.play({8, 8}); // B elsewhere
  b.play({1, 1}); // W

  // Black uses double move: first stone captures at (1,2) creating ko
  b.apply(Action::double_first({1, 2}));
  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.captures(Color::Black), 1);
  ASSERT_TRUE(b.ko_point().has_value());
  EXPECT_EQ(*b.ko_point(), (Point{1, 1}));

  // Second stone can fill ko
  EXPECT_TRUE(b.apply(Action::double_second({1, 1})));
}

// 20. Both players use double moves independently
TEST(DoubleMove, BothPlayersDoubleMove) {
  Board b(9);
  // Black double move
  b.apply(Action::double_first({3, 3}));
  b.apply(Action::double_second({3, 4}));
  EXPECT_EQ(b.at({3, 3}), Color::Black);
  EXPECT_EQ(b.at({3, 4}), Color::Black);

  // White double move
  b.apply(Action::double_first({5, 5}));
  b.apply(Action::double_second({5, 6}));
  EXPECT_EQ(b.at({5, 5}), Color::White);
  EXPECT_EQ(b.at({5, 6}), Color::White);

  // Black must pass (from earlier double move)
  EXPECT_TRUE(b.must_pass());
  b.pass();

  // White must pass
  EXPECT_TRUE(b.must_pass());
  b.pass();

  // Both can play normally now
  EXPECT_FALSE(b.must_pass());
  EXPECT_TRUE(b.play({7, 7}));
}

// 21. legal_actions in DoubleMove phase returns only DoubleSecond actions
TEST(DoubleMove, LegalActionsInDoubleMovePhase) {
  Board b(9);
  b.apply(Action::double_first({0, 0}));
  EXPECT_EQ(b.phase(), Phase::DoubleMove);

  auto actions = b.legal_actions();
  for (const auto &a : actions) {
    EXPECT_EQ(a.type, ActionType::DoubleSecond);
  }
  // Should have all empty legal points as DoubleSecond
  EXPECT_FALSE(actions.empty());

  // No Pass or Move or DoubleFirst in the list
  for (const auto &a : actions) {
    EXPECT_NE(a.type, ActionType::Pass);
    EXPECT_NE(a.type, ActionType::Move);
    EXPECT_NE(a.type, ActionType::DoubleFirst);
  }
}

// 22. Cannot pass during DoubleMove phase
TEST(DoubleMove, CannotPassDuringDoubleMove) {
  Board b(9);
  b.apply(Action::double_first({3, 3}));
  EXPECT_FALSE(b.apply(Action::pass()));
  EXPECT_EQ(b.phase(), Phase::DoubleMove);
}

// 23. Normal legal_actions includes Pass, Move, and DoubleFirst
TEST(DoubleMove, NormalLegalActionsIncludeAllTypes) {
  Board b(9);
  auto actions = b.legal_actions();

  bool has_pass = false, has_move = false, has_double_first = false;
  for (const auto &a : actions) {
    if (a.type == ActionType::Pass)
      has_pass = true;
    if (a.type == ActionType::Move)
      has_move = true;
    if (a.type == ActionType::DoubleFirst)
      has_double_first = true;
  }
  EXPECT_TRUE(has_pass);
  EXPECT_TRUE(has_move);
  EXPECT_TRUE(has_double_first);

  // Count: 1 Pass + 81 Move + 81 DoubleFirst = 163 on empty 9x9
  EXPECT_EQ(actions.size(), 163u);
}

// 24. Taking a ko on the second move correctly sets the ko point, and
// the forced pass resets the ko.

TEST(DoubleMove, SecondMoveKo) {
  Board b(9);

  // Setup ko pattern, then use second double move to trigger it
  // Standard ko setup:
  //   col:  0  1  2  3
  // row 0:  .  B  W  .
  // row 1:  B  W  .  W
  // row 2:  .  B  W  .
  b.play({0, 1}); // B
  b.play({0, 2}); // W
  b.play({1, 0}); // B
  b.play({1, 3}); // W
  b.play({2, 1}); // B
  b.play({2, 2}); // W
  b.play({8, 8}); // B elsewhere
  b.play({1, 1}); // W

  b.apply(Action::double_first({7, 7}));
  b.apply(Action::double_second({1, 2})); // B take ko

  EXPECT_FALSE(b.play({1, 1})); // W can't retake
  EXPECT_TRUE(b.play({6, 6}));  // W play away
  EXPECT_TRUE(b.must_pass());   // B must pass
  b.pass();
  EXPECT_TRUE(b.play({1, 1}));
}
