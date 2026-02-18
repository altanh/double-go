#include <iostream>

#include "double-go/double-go.h"

int main() {
    double_go::Board board(9);
    std::cout << "Double Go! Board size: " << board.size() << "x"
              << board.size() << "\n";

    // Black plays a double move
    board.apply(double_go::Action::double_first({2, 2}));
    board.apply(double_go::Action::double_second({2, 3}));
    std::cout << "Black double-moved at (2,2) and (2,3).\n";
    std::cout << "Turn: "
              << (board.to_play() == double_go::Color::Black ? "Black"
                                                              : "White")
              << "\n";

    // White plays a normal move
    board.play({4, 4});
    std::cout << "White played at (4,4).\n";

    // Black must pass
    std::cout << "Black must pass: " << (board.must_pass() ? "yes" : "no")
              << "\n";
    std::cout << "Legal actions: " << board.legal_actions().size() << "\n";
    board.pass();

    // Black can play normally again
    std::cout << "Black must pass after passing: "
              << (board.must_pass() ? "yes" : "no") << "\n";
    std::cout << "Legal actions: " << board.legal_actions().size() << "\n";
    return 0;
}
