#pragma once

#include "board.hpp"

namespace chess {

// Generate all pseudo-legal moves, then filter to legal ones
void generate_moves(const Board& board, MoveList& list);

// Generate only capture moves (for quiescence search)
void generate_captures(const Board& board, MoveList& list);

} // namespace chess
