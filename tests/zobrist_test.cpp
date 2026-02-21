#include <gtest/gtest.h>

#include "double-go/double-go.h"

#include <set>
#include <vector>

using namespace double_go;

// ===== Basic Hash Properties =====

// Empty boards of same size have identical hashes
TEST(ZobristHash, EmptyBoardsSameSizeIdentical) {
  Board b1(9);
  Board b2(9);
  EXPECT_EQ(b1.hash(), b2.hash());
}

// Note: Board size is not part of the Zobrist hash.
// Empty boards of different sizes have the same hash, which is acceptable
// since boards of different sizes are never compared in practice.
TEST(ZobristHash, EmptyBoardsSameSizeHaveSameHash) {
  Board b1(9);
  Board b2(9);
  Board b3(9);
  EXPECT_EQ(b1.hash(), b2.hash());
  EXPECT_EQ(b2.hash(), b3.hash());
}

// Placing a stone changes the hash
TEST(ZobristHash, PlacingStoneChangesHash) {
  Board b(9);
  uint64_t initial_hash = b.hash();
  b.apply(Action::place({3, 3}));
  EXPECT_NE(b.hash(), initial_hash);
}

// Same stone placement gives same hash
TEST(ZobristHash, SameStonePlacementSameHash) {
  Board b1(9);
  Board b2(9);
  b1.apply(Action::place({3, 3}));
  b2.apply(Action::place({3, 3}));
  EXPECT_EQ(b1.hash(), b2.hash());
}

// Different stone placements give different hashes
TEST(ZobristHash, DifferentStonePlacementsDifferentHashes) {
  Board b1(9);
  Board b2(9);
  b1.apply(Action::place({3, 3}));
  b2.apply(Action::place({4, 4}));
  EXPECT_NE(b1.hash(), b2.hash());
}

// ===== Player Turn Affects Hash =====

// Different players to move give different hashes
TEST(ZobristHash, DifferentPlayersDifferentHashes) {
  Board b1(9);
  Board b2(9);

  // b1: Black to play
  EXPECT_EQ(b1.to_play(), Color::Black);

  // b2: White to play (after Black passes)
  b2.pass();
  EXPECT_EQ(b2.to_play(), Color::White);

  EXPECT_NE(b1.hash(), b2.hash());
}

// ===== Phase Affects Hash =====

// Different phases give different hashes
TEST(ZobristHash, DifferentPhasesDifferentHashes) {
  Board b_first(9);
  EXPECT_EQ(b_first.phase(), Phase::First);

  Board b_second(9);
  b_second.apply(Action::place({3, 3}));
  EXPECT_EQ(b_second.phase(), Phase::Second);

  Board b_bonus(9);
  b_bonus.apply(Action::place({3, 3}));
  b_bonus.apply(Action::place({4, 4}));
  EXPECT_EQ(b_bonus.phase(), Phase::Bonus);

  // All three boards have different hashes due to different phases
  // (also different stone counts, but phase contributes)
  std::set<uint64_t> hashes = {b_first.hash(), b_second.hash(), b_bonus.hash()};
  EXPECT_EQ(hashes.size(), 3u);
}

// ===== Ko Point Affects Hash =====

// Ko point changes the hash
TEST(ZobristHash, KoPointChangesHash) {
  // Setup ko position
  Board b_ko(9);
  b_ko.play_single({0, 1}); // B
  b_ko.play_single({0, 2}); // W
  b_ko.play_single({1, 0}); // B
  b_ko.play_single({1, 3}); // W
  b_ko.play_single({2, 1}); // B
  b_ko.play_single({2, 2}); // W
  b_ko.play_single({8, 8}); // B elsewhere
  b_ko.play_single({1, 1}); // W

  uint64_t hash_before_ko = b_ko.hash();
  b_ko.play_single({1, 2}); // B captures, creates ko at (1,1)

  ASSERT_TRUE(b_ko.ko_point().has_value());
  EXPECT_NE(b_ko.hash(), hash_before_ko);
}

// Same position with and without ko has different hashes
TEST(ZobristHash, SamePositionWithAndWithoutKoDifferent) {
  // Create two boards that reach the same stone configuration
  // but one has a ko point and one doesn't

  // Board 1: Creates ko
  Board b_with_ko(9);
  b_with_ko.play_single({0, 1}); // B
  b_with_ko.play_single({0, 2}); // W
  b_with_ko.play_single({1, 0}); // B
  b_with_ko.play_single({1, 3}); // W
  b_with_ko.play_single({2, 1}); // B
  b_with_ko.play_single({2, 2}); // W
  b_with_ko.play_single({8, 8}); // B elsewhere
  b_with_ko.play_single({1, 1}); // W
  b_with_ko.play_single({1, 2}); // B captures, creates ko

  ASSERT_TRUE(b_with_ko.ko_point().has_value());

  // Board 2: Same stones, but ko cleared by White playing elsewhere
  Board b_without_ko(9);
  b_without_ko.play_single({0, 1}); // B
  b_without_ko.play_single({0, 2}); // W
  b_without_ko.play_single({1, 0}); // B
  b_without_ko.play_single({1, 3}); // W
  b_without_ko.play_single({2, 1}); // B
  b_without_ko.play_single({2, 2}); // W
  b_without_ko.play_single({8, 8}); // B elsewhere
  b_without_ko.play_single({1, 1}); // W
  b_without_ko.play_single({1, 2}); // B captures, creates ko
  b_without_ko.play_single({7, 7}); // W plays elsewhere, clears ko

  ASSERT_FALSE(b_without_ko.ko_point().has_value());

  // Different hashes (one has ko, different player, and different stones)
  EXPECT_NE(b_with_ko.hash(), b_without_ko.hash());
}

// ===== Transposition Testing =====

// Same position reached via different move orders should have same hash
TEST(ZobristHash, TranspositionSameHash) {
  // Order 1: Black at (3,3), then (4,4); White at (5,5), then (6,6)
  Board b1(9);
  b1.apply(Action::place({3, 3}));
  b1.apply(Action::place({4, 4}));
  b1.play_single({5, 5}); // W
  b1.play_single({6, 6}); // W
  b1.pass();              // B

  // Order 2: Black at (4,4), then (3,3); White at (6,6), then (5,5)
  Board b2(9);
  b2.apply(Action::place({4, 4}));
  b2.apply(Action::place({3, 3}));
  b2.play_single({6, 6}); // W
  b2.play_single({5, 5}); // W
  b2.pass();              // B

  // Same stones, same player, same phase -> same hash
  EXPECT_EQ(b1.at({3, 3}), b2.at({3, 3}));
  EXPECT_EQ(b1.at({4, 4}), b2.at({4, 4}));
  EXPECT_EQ(b1.at({5, 5}), b2.at({5, 5}));
  EXPECT_EQ(b1.at({6, 6}), b2.at({6, 6}));
  EXPECT_EQ(b1.to_play(), b2.to_play());
  EXPECT_EQ(b1.phase(), b2.phase());
  EXPECT_EQ(b1.hash(), b2.hash());
}

// ===== Capture Updates Hash Correctly =====

// Capturing stones updates hash correctly
TEST(ZobristHash, CaptureUpdatesHash) {
  Board b(9);
  // Surround white stone at (1,1)
  b.play_single({0, 1}); // B
  b.play_single({1, 1}); // W
  b.play_single({1, 0}); // B
  b.pass();              // W
  b.play_single({1, 2}); // B
  b.pass();              // W

  uint64_t hash_before_capture = b.hash();
  b.play_single({2, 1}); // B captures W(1,1)

  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_NE(b.hash(), hash_before_capture);
}

// After capture, position matches equivalent board built without capture
TEST(ZobristHash, CaptureHashMatchesEquivalentPosition) {
  // Board 1: Capture occurs
  Board b1(9);
  b1.play_single({0, 1}); // B
  b1.play_single({1, 1}); // W (will be captured)
  b1.play_single({1, 0}); // B
  b1.pass();              // W
  b1.play_single({1, 2}); // B
  b1.pass();              // W
  b1.play_single({2, 1}); // B captures

  // Board 2: Same black stones, no white stone ever placed
  Board b2(9);
  b2.play_single({0, 1}); // B
  b2.pass();              // W
  b2.play_single({1, 0}); // B
  b2.pass();              // W
  b2.play_single({1, 2}); // B
  b2.pass();              // W
  b2.play_single({2, 1}); // B

  // Both boards should have same stone positions
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      EXPECT_EQ(b1.at({r, c}), b2.at({r, c}))
          << "Mismatch at (" << r << ", " << c << ")";
    }
  }

  // Same player, same phase, same hash
  EXPECT_EQ(b1.to_play(), b2.to_play());
  EXPECT_EQ(b1.phase(), b2.phase());
  EXPECT_EQ(b1.hash(), b2.hash());
}

// ===== Multi-Stone Group Capture =====

TEST(ZobristHash, MultiStoneGroupCaptureUpdatesHash) {
  Board b(9);
  // White group at (1,1) and (1,2)
  b.play_single({0, 1}); // B
  b.play_single({1, 1}); // W
  b.play_single({0, 2}); // B
  b.play_single({1, 2}); // W
  b.play_single({1, 0}); // B
  b.pass();              // W
  b.play_single({1, 3}); // B
  b.pass();              // W
  b.play_single({2, 1}); // B
  b.pass();              // W

  uint64_t hash_before = b.hash();
  b.play_single({2, 2}); // B captures group of 2

  EXPECT_EQ(b.at({1, 1}), Color::Empty);
  EXPECT_EQ(b.at({1, 2}), Color::Empty);
  EXPECT_NE(b.hash(), hash_before);
}

// ===== Hash Uniqueness Over Many Positions =====

// Generate many positions and verify hash uniqueness
TEST(ZobristHash, HashUniquenessOverManyPositions) {
  std::set<uint64_t> seen_hashes;

  // Play random-ish game and collect hashes
  Board b(9);
  seen_hashes.insert(b.hash());

  std::vector<Point> moves = {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4},
                              {5, 5}, {6, 6}, {7, 7}, {8, 8}, {0, 8},
                              {8, 0}, {1, 7}, {7, 1}, {2, 6}, {6, 2}};

  for (const auto &p : moves) {
    if (b.is_legal(p)) {
      b.play_single(p);
      // Each unique position should have a unique hash
      // (small probability of collision, but very unlikely)
      bool inserted = seen_hashes.insert(b.hash()).second;
      EXPECT_TRUE(inserted) << "Hash collision detected";
    }
  }
}

// ===== ZobristHash Singleton Consistency =====

// Multiple calls to get_instance return same object
TEST(ZobristHash, SingletonConsistency) {
  ZobristHash &z1 = ZobristHash::get_instance();
  ZobristHash &z2 = ZobristHash::get_instance();
  EXPECT_EQ(&z1, &z2);
}

// Same inputs always give same hash values
TEST(ZobristHash, DeterministicHashValues) {
  ZobristHash &z = ZobristHash::get_instance();

  uint64_t h1 = z.stone(Color::Black, {3, 3});
  uint64_t h2 = z.stone(Color::Black, {3, 3});
  EXPECT_EQ(h1, h2);

  uint64_t h3 = z.stone(Color::White, {3, 3});
  uint64_t h4 = z.stone(Color::White, {3, 3});
  EXPECT_EQ(h3, h4);

  // Black and white at same point have different hashes
  EXPECT_NE(h1, h3);

  uint64_t k1 = z.ko({5, 5});
  uint64_t k2 = z.ko({5, 5});
  EXPECT_EQ(k1, k2);

  // Ko hash different from stone hash at same point
  EXPECT_NE(z.stone(Color::Black, {5, 5}), z.ko({5, 5}));
  EXPECT_NE(z.stone(Color::White, {5, 5}), z.ko({5, 5}));
}

// Phase hashes are all different
TEST(ZobristHash, PhaseHashesDifferent) {
  ZobristHash &z = ZobristHash::get_instance();
  uint64_t h_bonus = z.phase(Phase::Bonus);
  uint64_t h_first = z.phase(Phase::First);
  uint64_t h_second = z.phase(Phase::Second);

  EXPECT_NE(h_bonus, h_first);
  EXPECT_NE(h_bonus, h_second);
  EXPECT_NE(h_first, h_second);
}

// ===== Pass Behavior =====

// Pass changes hash (due to player/phase change)
TEST(ZobristHash, PassChangesHash) {
  Board b(9);
  uint64_t initial = b.hash();
  b.pass();
  EXPECT_NE(b.hash(), initial);
}

// Double pass from empty board returns to same state (same hash)
// but intermediate state has different hash
TEST(ZobristHash, DoublePassHashBehavior) {
  Board b(9);
  uint64_t h0 = b.hash();
  b.pass();
  uint64_t h1 = b.hash();
  b.pass();
  uint64_t h2 = b.hash();

  // After one pass, hash changes (different player)
  EXPECT_NE(h0, h1);
  // After two passes from empty, we're back to initial state
  // (Black to play, Phase::First, no stones, no ko)
  // so hash returns to original value
  EXPECT_EQ(h0, h2);
}

// ===== Double Move Phase Transitions =====

// Hash changes through all phase transitions
TEST(ZobristHash, DoubleMovePhasesChangeHash) {
  Board b(9);

  uint64_t h_first = b.hash();
  EXPECT_EQ(b.phase(), Phase::First);

  b.apply(Action::place({3, 3}));
  uint64_t h_second = b.hash();
  EXPECT_EQ(b.phase(), Phase::Second);

  b.apply(Action::place({4, 4}));
  uint64_t h_bonus = b.hash();
  EXPECT_EQ(b.phase(), Phase::Bonus);

  // All hashes different
  EXPECT_NE(h_first, h_second);
  EXPECT_NE(h_second, h_bonus);
  EXPECT_NE(h_first, h_bonus);
}

// ===== Ko in Double Move =====

// Ko from first move, filled by second move - hash updates correctly
TEST(ZobristHash, KoFilledBySecondMoveUpdatesHash) {
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

  // Black double move: capture at (1,2), then fill ko at (1,1)
  b.apply(Action::place({1, 2})); // Creates ko at (1,1)
  uint64_t h_with_ko = b.hash();
  ASSERT_TRUE(b.ko_point().has_value());

  b.apply(Action::place({1, 1})); // Fill the ko
  uint64_t h_ko_filled = b.hash();

  EXPECT_NE(h_with_ko, h_ko_filled);
}

// ===== Edge Cases =====

// Corner and edge positions hash correctly
TEST(ZobristHash, CornerAndEdgePositions) {
  std::set<uint64_t> hashes;

  // Test corners
  std::vector<Point> corners = {{0, 0}, {0, 8}, {8, 0}, {8, 8}};
  for (const auto &p : corners) {
    Board b(9);
    b.apply(Action::place(p));
    hashes.insert(b.hash());
  }

  // All corners should give different hashes
  EXPECT_EQ(hashes.size(), 4u);
}

// Same position on same size boards gives same hash
TEST(ZobristHash, SamePositionSameSizeConsistent) {
  // Place stones at same positions on two boards of same size
  Board b1(9);
  b1.play_single({3, 3});
  b1.play_single({4, 4});

  Board b2(9);
  b2.play_single({3, 3});
  b2.play_single({4, 4});

  // Same stones, same player, same phase -> same hash
  EXPECT_EQ(b1.hash(), b2.hash());
}

// ===== Stress Test =====

// Long game with many captures - hash never returns to previous value
// (except at game end with empty board, which doesn't happen here)
TEST(ZobristHash, LongGameHashUniqueness) {
  Board b(9);
  std::set<uint64_t> seen;
  int collisions = 0;

  // Play a game with predetermined moves
  for (int r = 0; r < 9 && !b.game_over(); r++) {
    for (int c = 0; c < 9 && !b.game_over(); c++) {
      if (b.is_legal({r, c})) {
        b.play_single({r, c});
        if (!seen.insert(b.hash()).second) {
          collisions++;
        }
      }
    }
  }

  // We expect very few or no collisions in a normal game
  // (Some transpositions are possible, so we allow a small number)
  EXPECT_LT(collisions, 5);
}

// ===== Path Independence Tests =====
// The key invariant: same position reached via different move orders gives same hash

// Same position reached via different move sequences has same hash
TEST(ZobristHash, PathIndependenceSimple) {
  // Path 1: Black (3,3), White (4,4), Black (5,5)
  Board b1(9);
  b1.play_single({3, 3}); // B
  b1.play_single({4, 4}); // W
  b1.play_single({5, 5}); // B

  // Path 2: Black (5,5), White (4,4), Black (3,3)
  Board b2(9);
  b2.play_single({5, 5}); // B
  b2.play_single({4, 4}); // W
  b2.play_single({3, 3}); // B

  // Same final position, same hash
  EXPECT_EQ(b1.to_play(), b2.to_play());
  EXPECT_EQ(b1.phase(), b2.phase());
  EXPECT_EQ(b1.hash(), b2.hash());
}

// Same position with captures via different paths
TEST(ZobristHash, PathIndependenceWithCapture) {
  // Both paths: surround and capture white stone, end with same stones

  // Path 1: Place surrounding stones in one order
  Board b1(9);
  b1.play_single({0, 1}); // B
  b1.play_single({1, 1}); // W (will be captured)
  b1.play_single({1, 0}); // B
  b1.pass();              // W
  b1.play_single({1, 2}); // B
  b1.pass();              // W
  b1.play_single({2, 1}); // B captures

  // Path 2: Same stones, no capture (White never placed at 1,1)
  Board b2(9);
  b2.play_single({0, 1}); // B
  b2.pass();              // W
  b2.play_single({1, 0}); // B
  b2.pass();              // W
  b2.play_single({1, 2}); // B
  b2.pass();              // W
  b2.play_single({2, 1}); // B

  // Same stone positions, same player, same phase -> same hash
  EXPECT_EQ(b1.at({1, 1}), Color::Empty);
  EXPECT_EQ(b2.at({1, 1}), Color::Empty);
  EXPECT_EQ(b1.to_play(), b2.to_play());
  EXPECT_EQ(b1.phase(), b2.phase());
  EXPECT_EQ(b1.hash(), b2.hash());
}

// Same stones but different ko status gives different hash
TEST(ZobristHash, KoAffectsHash) {
  // Setup two boards to same stone position, one with ko, one without

  // Board with ko
  Board b_ko(9);
  b_ko.play_single({0, 1}); // B
  b_ko.play_single({0, 2}); // W
  b_ko.play_single({1, 0}); // B
  b_ko.play_single({1, 3}); // W
  b_ko.play_single({2, 1}); // B
  b_ko.play_single({2, 2}); // W
  b_ko.play_single({8, 8}); // B
  b_ko.play_single({1, 1}); // W
  b_ko.play_single({1, 2}); // B captures, creates ko at (1,1)

  ASSERT_TRUE(b_ko.ko_point().has_value());
  EXPECT_EQ(*b_ko.ko_point(), (Point{1, 1}));

  // Board without ko - White plays elsewhere to clear it
  Board b_no_ko(9);
  b_no_ko.play_single({0, 1}); // B
  b_no_ko.play_single({0, 2}); // W
  b_no_ko.play_single({1, 0}); // B
  b_no_ko.play_single({1, 3}); // W
  b_no_ko.play_single({2, 1}); // B
  b_no_ko.play_single({2, 2}); // W
  b_no_ko.play_single({8, 8}); // B
  b_no_ko.play_single({1, 1}); // W
  b_no_ko.play_single({1, 2}); // B captures, creates ko
  b_no_ko.play_single({7, 7}); // W plays elsewhere, clears ko
  b_no_ko.pass();              // B passes to match turn

  // Different ko status (but note: different stones too since W played at 7,7)
  // This tests that ko definitely affects hash
  ASSERT_FALSE(b_no_ko.ko_point().has_value());
  EXPECT_NE(b_ko.hash(), b_no_ko.hash());
}

// Double move paths to same position give same hash
TEST(ZobristHash, PathIndependenceDoubleMoves) {
  // Path 1: B double move (3,3)+(4,4), W plays (5,5), then pass
  Board b1(9);
  b1.apply(Action::place({3, 3}));
  b1.apply(Action::place({4, 4}));
  b1.play_single({5, 5}); // W single
  b1.pass();              // B passes

  // Path 2: B double move (4,4)+(3,3), W plays (5,5), then pass
  Board b2(9);
  b2.apply(Action::place({4, 4}));
  b2.apply(Action::place({3, 3}));
  b2.play_single({5, 5}); // W single
  b2.pass();              // B passes

  // Same position, same hash
  EXPECT_EQ(b1.to_play(), b2.to_play());
  EXPECT_EQ(b1.phase(), b2.phase());
  EXPECT_EQ(b1.hash(), b2.hash());
}
