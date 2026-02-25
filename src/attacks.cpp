#include "attacks.hpp"

namespace chess {

Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
Bitboard KnightAttacks[SQUARE_NB];
Bitboard KingAttacks[SQUARE_NB];

// Ray directions for sliding pieces: {file_delta, rank_delta}
static constexpr int BishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
static constexpr int RookDirs[4][2]   = {{0,1},{0,-1},{1,0},{-1,0}};

static Bitboard compute_ray(Square sq, int df, int dr, Bitboard occupied) {
    Bitboard attacks = EMPTY_BB;
    int f = file_of(sq);
    int r = rank_of(sq);
    while (true) {
        f += df;
        r += dr;
        if (f < 0 || f > 7 || r < 0 || r > 7) break;
        Bitboard bit = square_bb(make_square(File(f), Rank(r)));
        attacks |= bit;
        if (bit & occupied) break;  // blocked
    }
    return attacks;
}

Bitboard get_bishop_attacks(Square sq, Bitboard occupied) {
    Bitboard attacks = EMPTY_BB;
    for (auto& d : BishopDirs)
        attacks |= compute_ray(sq, d[0], d[1], occupied);
    return attacks;
}

Bitboard get_rook_attacks(Square sq, Bitboard occupied) {
    Bitboard attacks = EMPTY_BB;
    for (auto& d : RookDirs)
        attacks |= compute_ray(sq, d[0], d[1], occupied);
    return attacks;
}

void init_attacks() {
    // Knight offsets
    static constexpr int KnightOffsets[8][2] = {
        {-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}
    };
    // King offsets
    static constexpr int KingOffsets[8][2] = {
        {-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}
    };

    for (int sq = 0; sq < 64; ++sq) {
        int f = file_of(Square(sq));
        int r = rank_of(Square(sq));

        // Pawn attacks
        if (r < 7) {
            if (f > 0) PawnAttacks[WHITE][sq] |= square_bb(make_square(File(f-1), Rank(r+1)));
            if (f < 7) PawnAttacks[WHITE][sq] |= square_bb(make_square(File(f+1), Rank(r+1)));
        }
        if (r > 0) {
            if (f > 0) PawnAttacks[BLACK][sq] |= square_bb(make_square(File(f-1), Rank(r-1)));
            if (f < 7) PawnAttacks[BLACK][sq] |= square_bb(make_square(File(f+1), Rank(r-1)));
        }

        // Knight attacks
        KnightAttacks[sq] = EMPTY_BB;
        for (auto& o : KnightOffsets) {
            int nf = f + o[0], nr = r + o[1];
            if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7)
                KnightAttacks[sq] |= square_bb(make_square(File(nf), Rank(nr)));
        }

        // King attacks
        KingAttacks[sq] = EMPTY_BB;
        for (auto& o : KingOffsets) {
            int nf = f + o[0], nr = r + o[1];
            if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7)
                KingAttacks[sq] |= square_bb(make_square(File(nf), Rank(nr)));
        }
    }
}

} // namespace chess
