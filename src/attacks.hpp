#pragma once

#include "types.hpp"

namespace chess {

// Precomputed attack tables
extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
extern Bitboard KnightAttacks[SQUARE_NB];
extern Bitboard KingAttacks[SQUARE_NB];

// Initialize all precomputed tables (call once at startup)
void init_attacks();

// Sliding piece attacks (classical ray approach)
Bitboard get_bishop_attacks(Square sq, Bitboard occupied);
Bitboard get_rook_attacks(Square sq, Bitboard occupied);

inline Bitboard get_queen_attacks(Square sq, Bitboard occupied) {
    return get_bishop_attacks(sq, occupied) | get_rook_attacks(sq, occupied);
}

// Get attacks for any piece type
inline Bitboard get_attacks(PieceType pt, Square sq, Bitboard occupied) {
    switch (pt) {
        case KNIGHT: return KnightAttacks[sq];
        case BISHOP: return get_bishop_attacks(sq, occupied);
        case ROOK:   return get_rook_attacks(sq, occupied);
        case QUEEN:  return get_queen_attacks(sq, occupied);
        case KING:   return KingAttacks[sq];
        default:     return EMPTY_BB;
    }
}

} // namespace chess
