#include "search.h"
#include <algorithm>

namespace nova {

static const int pieceValues[13] = {
    0, 100, 320, 330, 500, 900, 20000,
    -100, -320, -330, -500, -900, -20000
};

// Piece square tables for white. Black uses mirrored indices.
static const int PawnTable[64] = {
     0, 0, 0, 0, 0, 0, 0, 0,
    50,50,50,50,50,50,50,50,
    10,10,20,30,30,20,10,10,
     5, 5,10,25,25,10, 5, 5,
     0, 0, 0,20,20, 0, 0, 0,
     5,-5,-10, 0, 0,-10,-5, 5,
     5,10,10,-20,-20,10,10, 5,
     0, 0, 0, 0, 0, 0, 0, 0
};

static const int KnightTable[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

static const int BishopTable[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

static const int RookTable[64] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     5,10,10,10,10,10,10, 5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
     0, 0, 0, 5, 5, 0, 0, 0
};

static const int QueenTable[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

static const int KingTable[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

static int mirror(int idx) { return ((7 - idx / 8) * 8) + (idx % 8); }

int evaluate(const Board& board) {
    int score = 0;
    for (int sq = 0; sq < 128; ++sq) {
        if ((sq & 0x88) != 0) continue;
        int piece = board.pieceAt(sq);
        if (piece == EMPTY) continue;
        int idx = (sq >> 4) * 8 + (sq & 7);
        switch (piece) {
            case WP:
                score += 100 + PawnTable[idx];
                break;
            case WN:
                score += 320 + KnightTable[idx];
                break;
            case WB:
                score += 330 + BishopTable[idx];
                break;
            case WR:
                score += 500 + RookTable[idx];
                break;
            case WQ:
                score += 900 + QueenTable[idx];
                break;
            case WK:
                score += KingTable[idx];
                break;
            case BP:
                score -= 100 + PawnTable[mirror(idx)];
                break;
            case BN:
                score -= 320 + KnightTable[mirror(idx)];
                break;
            case BB:
                score -= 330 + BishopTable[mirror(idx)];
                break;
            case BR:
                score -= 500 + RookTable[mirror(idx)];
                break;
            case BQ:
                score -= 900 + QueenTable[mirror(idx)];
                break;
            case BK:
                score -= KingTable[mirror(idx)];
                break;
            default:
                break;
        }
    }

    // Mobility for side to move
    int mobility = board.generateLegalMoves().size();
    if (board.sideToMove() == WHITE) score += mobility * 5; else score -= mobility * 5;

    // King safety: number of friendly pawns around king
    for (int sq = 0; sq < 128; ++sq) {
        if ((sq & 0x88) != 0) continue;
        int piece = board.pieceAt(sq);
        if (piece == WK || piece == BK) {
            int kingScore = 0;
            static const int KingMoves[8] = { -17,-16,-15,-1,1,15,16,17 };
            for (int off : KingMoves) {
                int t = sq + off;
                if ((t & 0x88) != 0) continue;
                int p = board.pieceAt(t);
                if (piece == WK && p == WP) kingScore += 10;
                if (piece == BK && p == BP) kingScore += 10;
            }
            if (piece == WK) score += kingScore; else score -= kingScore;
        }
    }

    return score;
}

void orderMoves(std::vector<Move>& moves, const Board& board) {
    auto value = [&](const Move& m) {
        int target = board.pieceAt(m.to);
        int attacker = board.pieceAt(m.from);
        int score = 0;
        if (target != EMPTY)
            score += (abs(pieceValues[target]) - abs(pieceValues[attacker])) + 1000;
        if (m.promotion != EMPTY) score += 800;
        return -score; // negative for ascending sort
    };
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){return value(a) < value(b);});
}

int search(Board& board, int depth, int alpha, int beta) {
    if (depth == 0) return evaluate(board);

    std::vector<Move> moves = board.generateLegalMoves();
    orderMoves(moves, board);
    if (moves.empty()) {
        if (board.inCheck()) {
            return board.sideToMove() == WHITE ? -100000 : 100000;
        }
        return 0; // stalemate
    }

    if (board.sideToMove() == WHITE) {
        int value = -1000000;
        for (const Move& m : moves) {
            board.makeMove(m);
            int score = search(board, depth-1, alpha, beta);
            board.undoMove();
            if (score > value) value = score;
            if (value > alpha) alpha = value;
            if (alpha >= beta) break;
        }
        return value;
    } else {
        int value = 1000000;
        for (const Move& m : moves) {
            board.makeMove(m);
            int score = search(board, depth-1, alpha, beta);
            board.undoMove();
            if (score < value) value = score;
            if (value < beta) beta = value;
            if (alpha >= beta) break;
        }
        return value;
    }
}

Move findBestMove(Board& board, int maxDepth) {
    Move best{};
    int bestScore = board.sideToMove() == WHITE ? -1000000 : 1000000;

    for (int depth = 1; depth <= maxDepth; ++depth) {
        std::vector<Move> moves = board.generateLegalMoves();
        orderMoves(moves, board);
        for (const Move& m : moves) {
            board.makeMove(m);
            int score = search(board, depth-1, -1000000, 1000000);
            board.undoMove();
            if (board.sideToMove() == WHITE) {
                if (score > bestScore || depth == 1 && m.from == best.from && m.to == best.to) {
                    bestScore = score;
                    best = m;
                }
            } else {
                if (score < bestScore || depth == 1 && m.from == best.from && m.to == best.to) {
                    bestScore = score;
                    best = m;
                }
            }
        }
    }
    return best;
}

} // namespace nova

