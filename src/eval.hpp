#pragma once

#include "board.hpp"

namespace chess {

// Evaluate the position from the perspective of the side to move.
// Positive = advantage for side to move.
int evaluate(const Board& board);

} // namespace chess
