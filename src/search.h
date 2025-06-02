#ifndef NOVA_SEARCH_H
#define NOVA_SEARCH_H

#include "board.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace nova {

struct TTEntry {
    int depth;
    int score;
};

extern std::unordered_map<std::string, TTEntry> TransTable;

int evaluate(const Board& board);
int search(Board& board, int depth, int alpha, int beta);
Move findBestMove(Board& board, int maxDepth);
void orderMoves(std::vector<Move>& moves, const Board& board);

}

#endif // NOVA_SEARCH_H
