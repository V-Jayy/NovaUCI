#include "eval.hpp"
#include "movegen.hpp"

namespace chess {

// ============================================================
// Piece-Square Tables (from White's perspective, A1=index 0)
// Midgame (mg) and Endgame (eg) values
// ============================================================

// Pawn PST
static constexpr int PawnMG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 40, 40, 20, 10, 10,
     5,  5, 15, 30, 30, 15,  5,  5,
     0,  0, 10, 25, 25, 10,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};

static constexpr int PawnEG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    80, 80, 80, 80, 80, 80, 80, 80,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
    20, 20, 20, 20, 20, 20, 20, 20,
    10, 10, 10, 10, 10, 10, 10, 10,
     5,  5,  5,  5,  5,  5,  5,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};

// Knight PST
static constexpr int KnightMG[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

static constexpr int KnightEG[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50,
};

// Bishop PST
static constexpr int BishopMG[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20,
};

static constexpr int BishopEG[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20,
};

// Rook PST
static constexpr int RookMG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0,
};

static constexpr int RookEG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0,
};

// Queen PST
static constexpr int QueenMG[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20,
};

static constexpr int QueenEG[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20,
};

// King PST
static constexpr int KingMG[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20,
};

static constexpr int KingEG[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50,
};

// PST lookup: [piece_type][square] for MG and EG
static constexpr const int* PST_MG[PIECE_TYPE_NB] = {
    nullptr, PawnMG, KnightMG, BishopMG, RookMG, QueenMG, KingMG
};
static constexpr const int* PST_EG[PIECE_TYPE_NB] = {
    nullptr, PawnEG, KnightEG, BishopEG, RookEG, QueenEG, KingEG
};

// Phase weights per piece type
static constexpr int PhaseWeight[PIECE_TYPE_NB] = {
    0, 0, 1, 1, 2, 4, 0
};
static constexpr int TOTAL_PHASE = 24; // 4*1(N) + 4*1(B) + 4*2(R) + 2*4(Q)

// Mirror square for black (flip rank)
static constexpr int mirror_sq(int sq) {
    return sq ^ 56;  // flip rank: rank 0 <-> rank 7
}

// ============================================================
// Evaluation
// ============================================================
int evaluate(const Board& board) {
    int mg_score[COLOR_NB] = {0, 0};
    int eg_score[COLOR_NB] = {0, 0};
    int phase = 0;

    // Material + PST
    for (int sq = 0; sq < 64; ++sq) {
        Piece p = board.piece_on(Square(sq));
        if (p == NO_PIECE) continue;

        Color c = piece_color(p);
        PieceType pt = piece_type(p);

        // Material
        mg_score[c] += PieceValue[pt];
        eg_score[c] += PieceValue[pt];

        // Phase
        phase += PhaseWeight[pt];

        // PST (tables are from White's perspective)
        int pst_sq = (c == WHITE) ? sq : mirror_sq(sq);
        mg_score[c] += PST_MG[pt][pst_sq];
        eg_score[c] += PST_EG[pt][pst_sq];
    }

    // Pawn structure: doubled and isolated pawns
    for (int c = 0; c < 2; ++c) {
        Bitboard pawns = board.pieces(Color(c), PAWN);

        for (int f = 0; f < 8; ++f) {
            Bitboard file_pawns = pawns & file_bb(File(f));
            int count = popcount(file_pawns);

            // Doubled pawns penalty
            if (count > 1) {
                mg_score[c] -= 10 * (count - 1);
                eg_score[c] -= 20 * (count - 1);
            }

            // Isolated pawn penalty
            if (file_pawns) {
                Bitboard adj = EMPTY_BB;
                if (f > 0) adj |= file_bb(File(f - 1));
                if (f < 7) adj |= file_bb(File(f + 1));
                if (!(pawns & adj)) {
                    mg_score[c] -= 15 * count;
                    eg_score[c] -= 20 * count;
                }
            }
        }
    }

    // Simple mobility: count attacks for knights, bishops, rooks, queens
    Bitboard occ = board.occupied();
    for (int c = 0; c < 2; ++c) {
        Color col = Color(c);
        int mobility = 0;

        // Knights
        Bitboard bb = board.pieces(col, KNIGHT);
        while (bb) {
            Square sq = pop_lsb(bb);
            mobility += popcount(KnightAttacks[sq] & ~board.pieces(col));
        }
        // Bishops
        bb = board.pieces(col, BISHOP);
        while (bb) {
            Square sq = pop_lsb(bb);
            mobility += popcount(get_bishop_attacks(sq, occ) & ~board.pieces(col));
        }
        // Rooks
        bb = board.pieces(col, ROOK);
        while (bb) {
            Square sq = pop_lsb(bb);
            mobility += popcount(get_rook_attacks(sq, occ) & ~board.pieces(col));
        }
        // Queens
        bb = board.pieces(col, QUEEN);
        while (bb) {
            Square sq = pop_lsb(bb);
            mobility += popcount(get_queen_attacks(sq, occ) & ~board.pieces(col));
        }

        mg_score[c] += mobility * 3;
        eg_score[c] += mobility * 2;
    }

    // Tapered evaluation
    if (phase > TOTAL_PHASE) phase = TOTAL_PHASE;
    int mg = mg_score[WHITE] - mg_score[BLACK];
    int eg = eg_score[WHITE] - eg_score[BLACK];

    int score = (mg * phase + eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    // Return relative to side to move
    return (board.side_to_move() == WHITE) ? score : -score;
}

} // namespace chess
