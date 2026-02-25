#pragma once

#include "board.hpp"

namespace chess {

// Initialize the opening book (call once at startup, after init_attacks)
void init_book();

// Probe the book for the current position. Returns MOVE_NONE if not in book.
Move probe_book(const Board& board);

} // namespace chess
