#include "double-go/bot.h"

namespace double_go {

RandomBot::RandomBot(unsigned seed) : rng_(seed) {}

Action RandomBot::pick_action(const Board& board) {
    auto actions = board.legal_actions();
    std::uniform_int_distribution<int> dist(0, static_cast<int>(actions.size()) - 1);
    return actions[dist(rng_)];
}

} // namespace double_go
