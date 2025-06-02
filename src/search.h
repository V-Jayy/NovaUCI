#ifndef NOVA_SEARCH_H
#define NOVA_SEARCH_H

#include "board.h"
#include <vector>

namespace nova {

int evaluate(const Board& board);
int search(Board& board, int depth, int alpha, int beta);
Move findBestMove(Board& board, int maxDepth);
void orderMoves(std::vector<Move>& moves, const Board& board);

}

#endif // NOVA_SEARCH_H
