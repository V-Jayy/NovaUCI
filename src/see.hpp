#pragma once

#include "board.hpp"

namespace chess {

// Tests if the SEE (Static Exchange Evaluation) value of move is 
// greater or equal to the given threshold.
bool see_ge(const Board& board, Move m, int threshold = 0);

} // namespace chess
