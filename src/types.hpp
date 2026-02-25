#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <cassert>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace chess {

// ============================================================
// Bitboard type
// ============================================================
using Bitboard = uint64_t;

constexpr Bitboard EMPTY_BB  = 0ULL;
constexpr Bitboard FULL_BB   = ~0ULL;

// ============================================================
// Enumerations
// ============================================================
enum Color : int { WHITE, BLACK, COLOR_NB = 2 };

constexpr Color operator~(Color c) { return Color(c ^ 1); }

enum PieceType : int {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 7
};

enum Piece : int {
    NO_PIECE,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return Piece((c << 3) | pt);
}

constexpr Color piece_color(Piece p) {
    assert(p != NO_PIECE);
    return Color(p >> 3);
}

constexpr PieceType piece_type(Piece p) {
    return PieceType(p & 7);
}

enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE = 64,
    SQUARE_NB = 64
};

enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB = 8 };
enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB = 8 };

constexpr Square make_square(File f, Rank r) { return Square((r << 3) | f); }
constexpr File   file_of(Square s) { return File(s & 7); }
constexpr Rank   rank_of(Square s) { return Rank(s >> 3); }

constexpr Bitboard square_bb(Square s) { return 1ULL << s; }

// Square/string conversions
inline std::string square_to_string(Square s) {
    if (s == SQ_NONE) return "-";
    return std::string{char('a' + file_of(s)), char('1' + rank_of(s))};
}

inline Square string_to_square(std::string_view sv) {
    if (sv.size() < 2 || sv[0] < 'a' || sv[0] > 'h' || sv[1] < '1' || sv[1] > '8')
        return SQ_NONE;
    return make_square(File(sv[0] - 'a'), Rank(sv[1] - '1'));
}

// ============================================================
// File & Rank bitmasks
// ============================================================
constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
constexpr Bitboard FILE_B_BB = FILE_A_BB << 1;
constexpr Bitboard FILE_C_BB = FILE_A_BB << 2;
constexpr Bitboard FILE_D_BB = FILE_A_BB << 3;
constexpr Bitboard FILE_E_BB = FILE_A_BB << 4;
constexpr Bitboard FILE_F_BB = FILE_A_BB << 5;
constexpr Bitboard FILE_G_BB = FILE_A_BB << 6;
constexpr Bitboard FILE_H_BB = FILE_A_BB << 7;

constexpr Bitboard RANK_1_BB = 0xFFULL;
constexpr Bitboard RANK_2_BB = RANK_1_BB << 8;
constexpr Bitboard RANK_3_BB = RANK_1_BB << 16;
constexpr Bitboard RANK_4_BB = RANK_1_BB << 24;
constexpr Bitboard RANK_5_BB = RANK_1_BB << 32;
constexpr Bitboard RANK_6_BB = RANK_1_BB << 40;
constexpr Bitboard RANK_7_BB = RANK_1_BB << 48;
constexpr Bitboard RANK_8_BB = RANK_1_BB << 56;

constexpr Bitboard file_bb(File f) { return FILE_A_BB << f; }
constexpr Bitboard rank_bb(Rank r) { return RANK_1_BB << (8 * r); }
constexpr Bitboard file_bb(Square s) { return file_bb(file_of(s)); }
constexpr Bitboard rank_bb(Square s) { return rank_bb(rank_of(s)); }

// ============================================================
// Bit manipulation
// ============================================================
inline int popcount(Bitboard b) {
#ifdef _MSC_VER
    return static_cast<int>(__popcnt64(b));
#else
    return __builtin_popcountll(b);
#endif
}

inline Square lsb(Bitboard b) {
    assert(b);
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return Square(idx);
#else
    return Square(__builtin_ctzll(b));
#endif
}

inline Square pop_lsb(Bitboard& b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

inline bool more_than_one(Bitboard b) {
    return b & (b - 1);
}

// ============================================================
// Move encoding (16-bit)
//   bits  0-5: from square
//   bits  6-11: to square
//   bits 12-15: flags
// ============================================================
enum MoveType : uint16_t {
    MT_NORMAL    = 0,
    MT_CASTLING  = 1 << 12,
    MT_EN_PASSANT = 2 << 12,
    MT_PROMOTION = 3 << 12,
};

// Promotion sub-flags encoded in bits 12-13 as promotion piece offset
// We encode: bits 14-15 = promotion piece type - KNIGHT
// Combined with MT_PROMOTION in bits 12-13
enum PromotionType : uint16_t {
    PROMO_KNIGHT = MT_PROMOTION | (0 << 14),
    PROMO_BISHOP = MT_PROMOTION | (1 << 14),
    PROMO_ROOK   = MT_PROMOTION | (2 << 14),
    PROMO_QUEEN  = MT_PROMOTION | (3 << 14),
};

struct Move {
    uint16_t data = 0;

    constexpr Move() = default;
    constexpr explicit Move(uint16_t d) : data(d) {}

    constexpr Move(Square from, Square to, uint16_t flags = MT_NORMAL)
        : data(static_cast<uint16_t>(from | (to << 6) | flags)) {}

    constexpr Square from()  const { return Square(data & 0x3F); }
    constexpr Square to()    const { return Square((data >> 6) & 0x3F); }
    constexpr uint16_t flags() const { return data & 0xF000; }
    constexpr uint16_t type() const { return data & 0x3000; }

    constexpr bool is_castling()   const { return type() == MT_CASTLING; }
    constexpr bool is_en_passant() const { return type() == MT_EN_PASSANT; }
    constexpr bool is_promotion()  const { return type() == MT_PROMOTION; }

    constexpr PieceType promotion_type() const {
        return PieceType(KNIGHT + ((data >> 14) & 3));
    }

    constexpr bool operator==(Move other) const { return data == other.data; }
    constexpr bool operator!=(Move other) const { return data != other.data; }
    constexpr explicit operator bool() const { return data != 0; }

    std::string to_uci() const {
        std::string s = square_to_string(from()) + square_to_string(to());
        if (is_promotion()) {
            constexpr char promo_chars[] = "nbrq";
            s += promo_chars[(data >> 14) & 3];
        }
        return s;
    }
};

constexpr Move MOVE_NONE{};

// ============================================================
// Castling rights (4-bit)
// ============================================================
enum CastlingRight : int {
    NO_CASTLING = 0,
    WHITE_OO  = 1,
    WHITE_OOO = 2,
    BLACK_OO  = 4,
    BLACK_OOO = 8,
    ALL_CASTLING = 15,
    CASTLING_RIGHT_NB = 16
};

constexpr CastlingRight king_side(Color c)  { return c == WHITE ? WHITE_OO  : BLACK_OO; }
constexpr CastlingRight queen_side(Color c) { return c == WHITE ? WHITE_OOO : BLACK_OOO; }

// ============================================================
// Score constants
// ============================================================
constexpr int VALUE_ZERO     = 0;
constexpr int VALUE_DRAW     = 0;
constexpr int VALUE_MATE     = 32000;
constexpr int VALUE_INFINITE = 32001;
constexpr int VALUE_NONE     = 32002;

constexpr int MATE_IN_MAX_PLY  =  VALUE_MATE - 256;
constexpr int MATED_IN_MAX_PLY = -VALUE_MATE + 256;

constexpr int PieceValue[PIECE_TYPE_NB] = {
    0, 100, 320, 330, 500, 900, 20000
};

// ============================================================
// MoveList helper
// ============================================================
struct MoveList {
    Move moves[256];
    int count = 0;

    void push(Move m) { moves[count++] = m; }
    Move& operator[](int i) { return moves[i]; }
    const Move& operator[](int i) const { return moves[i]; }
    int size() const { return count; }
    Move* begin() { return moves; }
    Move* end()   { return moves + count; }
    const Move* begin() const { return moves; }
    const Move* end()   const { return moves + count; }
};

} // namespace chess
