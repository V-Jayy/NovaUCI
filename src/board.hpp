#pragma once

#include "types.hpp"
#include "attacks.hpp"
#include <array>
#include <string>
#include <string_view>
#include <vector>
#include <random>

namespace chess {

// ============================================================
// Zobrist hashing keys
// ============================================================
struct ZobristKeys {
    uint64_t piece_square[PIECE_NB][SQUARE_NB];
    uint64_t side;
    uint64_t castling[CASTLING_RIGHT_NB];
    uint64_t en_passant[FILE_NB + 1]; // +1 for "no ep" index 8

    ZobristKeys() {
        std::mt19937_64 rng(0xDEADBEEF42ULL);
        for (auto& ps : piece_square)
            for (auto& s : ps) s = rng();
        side = rng();
        for (auto& c : castling) c = rng();
        for (auto& e : en_passant) e = rng();
    }
};

inline const ZobristKeys& zobrist() {
    static ZobristKeys keys;
    return keys;
}

// ============================================================
// Undo information
// ============================================================
struct UndoInfo {
    Piece    captured;
    int      castling_rights;
    Square   en_passant;
    int      halfmove_clock;
    uint64_t hash;
};

// ============================================================
// Board class
// ============================================================
class Board {
public:
    Board();

    // Setup
    void set_fen(std::string_view fen);
    std::string to_fen() const;
    void set_startpos();

    // Queries
    Piece    piece_on(Square sq) const { return board_[sq]; }
    Bitboard pieces(Color c) const { return color_bb_[c]; }
    Bitboard pieces(PieceType pt) const { return type_bb_[pt]; }
    Bitboard pieces(Color c, PieceType pt) const { return color_bb_[c] & type_bb_[pt]; }
    Bitboard occupied() const { return color_bb_[WHITE] | color_bb_[BLACK]; }

    Color   side_to_move() const { return side_; }
    int     castling_rights() const { return castling_; }
    Square  en_passant_sq() const { return ep_square_; }
    int     halfmove_clock() const { return halfmove_; }
    int     fullmove_number() const { return fullmove_; }
    uint64_t hash_key() const { return hash_; }

    // King square for a given color
    Square king_sq(Color c) const { return lsb(pieces(c, KING)); }

    // Check if a square is attacked by a given color
    bool is_square_attacked(Square sq, Color by) const;

    // Is the side to move in check?
    bool in_check() const { return is_square_attacked(king_sq(side_), ~side_); }

    // Square attacks
    Bitboard attackers_to(Square sq, Bitboard occupied) const;

    // Pieces other than pawns and kings
    bool has_nonPawn_material(Color c) const {
        return pieces(c) & ~(pieces(c, PAWN) | pieces(c, KING));
    }

    // Move execution
    void make_move(Move m);
    void unmake_move(Move m);

    void make_null_move();
    void unmake_null_move();

private:
    // Piece placement
    Piece    board_[SQUARE_NB];
    Bitboard type_bb_[PIECE_TYPE_NB];
    Bitboard color_bb_[COLOR_NB];

    // Game state
    Color    side_;
    int      castling_;
    Square   ep_square_;
    int      halfmove_;
    int      fullmove_;
    uint64_t hash_;

    // Undo history
    std::vector<UndoInfo> history_;

    // Internal helpers
    void put_piece(Piece p, Square sq);
    void remove_piece(Square sq);
    void move_piece(Square from, Square to);
    void compute_hash();
};

} // namespace chess
