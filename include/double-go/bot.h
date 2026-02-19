#pragma once

#include "board.h"

#include <random>

namespace double_go {

class RandomBot {
public:
    explicit RandomBot(unsigned seed = std::random_device{}());
    Action pick_action(const Board& board);

private:
    std::mt19937 rng_;
};

} // namespace double_go
