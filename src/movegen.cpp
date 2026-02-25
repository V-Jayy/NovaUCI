#include "movegen.hpp"

namespace chess {

// ============================================================
// Pseudo-legal move generation helpers
// ============================================================

static void generate_pawn_moves(const Board& board, MoveList& list, bool captures_only) {
    Color us = board.side_to_move();
    Color them = ~us;
    Bitboard our_pawns = board.pieces(us, PAWN);
    Bitboard occ = board.occupied();
    Bitboard their_pieces = board.pieces(them);
    Bitboard empty = ~occ;

    int push_dir = (us == WHITE) ? 8 : -8;
    Bitboard promo_rank = (us == WHITE) ? RANK_8_BB : RANK_1_BB;
    Bitboard start_rank = (us == WHITE) ? RANK_2_BB : RANK_7_BB;

    // --- Captures ---
    Bitboard pawns = our_pawns;
    while (pawns) {
        Square from = pop_lsb(pawns);
        Bitboard attacks = PawnAttacks[us][from] & their_pieces;
        while (attacks) {
            Square to = pop_lsb(attacks);
            if (square_bb(to) & promo_rank) {
                list.push(Move(from, to, PROMO_QUEEN));
                list.push(Move(from, to, PROMO_ROOK));
                list.push(Move(from, to, PROMO_BISHOP));
                list.push(Move(from, to, PROMO_KNIGHT));
            } else {
                list.push(Move(from, to));
            }
        }
    }

    // --- En passant ---
    if (board.en_passant_sq() != SQ_NONE) {
        Square ep = board.en_passant_sq();
        Bitboard ep_pawns = PawnAttacks[them][ep] & our_pawns;
        while (ep_pawns) {
            Square from = pop_lsb(ep_pawns);
            list.push(Move(from, ep, MT_EN_PASSANT));
        }
    }

    if (captures_only) return;

    // --- Single push ---
    pawns = our_pawns;
    while (pawns) {
        Square from = pop_lsb(pawns);
        Square to = Square(int(from) + push_dir);
        if (to >= SQ_A1 && to <= SQ_H8 && (square_bb(to) & empty)) {
            if (square_bb(to) & promo_rank) {
                list.push(Move(from, to, PROMO_QUEEN));
                list.push(Move(from, to, PROMO_ROOK));
                list.push(Move(from, to, PROMO_BISHOP));
                list.push(Move(from, to, PROMO_KNIGHT));
            } else {
                list.push(Move(from, to));

                // Double push
                if (square_bb(from) & start_rank) {
                    Square to2 = Square(int(to) + push_dir);
                    if (square_bb(to2) & empty) {
                        list.push(Move(from, to2));
                    }
                }
            }
        }
    }
}

static void generate_piece_moves(const Board& board, MoveList& list, PieceType pt, bool captures_only) {
    Color us = board.side_to_move();
    Bitboard our_pieces = board.pieces(us);
    Bitboard their_pieces = board.pieces(~us);
    Bitboard occ = board.occupied();
    Bitboard pieces = board.pieces(us, pt);

    while (pieces) {
        Square from = pop_lsb(pieces);
        Bitboard attacks = get_attacks(pt, from, occ);
        attacks &= ~our_pieces; // can't capture own pieces

        if (captures_only)
            attacks &= their_pieces;

        while (attacks) {
            Square to = pop_lsb(attacks);
            list.push(Move(from, to));
        }
    }
}

static void generate_castling_moves(const Board& board, MoveList& list) {
    Color us = board.side_to_move();
    Bitboard occ = board.occupied();

    if (us == WHITE) {
        // King side:  e1 -> g1
        if ((board.castling_rights() & WHITE_OO) &&
            !(occ & (square_bb(SQ_F1) | square_bb(SQ_G1))) &&
            !board.is_square_attacked(SQ_E1, BLACK) &&
            !board.is_square_attacked(SQ_F1, BLACK) &&
            !board.is_square_attacked(SQ_G1, BLACK))
        {
            list.push(Move(SQ_E1, SQ_G1, MT_CASTLING));
        }
        // Queen side: e1 -> c1
        if ((board.castling_rights() & WHITE_OOO) &&
            !(occ & (square_bb(SQ_D1) | square_bb(SQ_C1) | square_bb(SQ_B1))) &&
            !board.is_square_attacked(SQ_E1, BLACK) &&
            !board.is_square_attacked(SQ_D1, BLACK) &&
            !board.is_square_attacked(SQ_C1, BLACK))
        {
            list.push(Move(SQ_E1, SQ_C1, MT_CASTLING));
        }
    } else {
        // King side:  e8 -> g8
        if ((board.castling_rights() & BLACK_OO) &&
            !(occ & (square_bb(SQ_F8) | square_bb(SQ_G8))) &&
            !board.is_square_attacked(SQ_E8, WHITE) &&
            !board.is_square_attacked(SQ_F8, WHITE) &&
            !board.is_square_attacked(SQ_G8, WHITE))
        {
            list.push(Move(SQ_E8, SQ_G8, MT_CASTLING));
        }
        // Queen side: e8 -> c8
        if ((board.castling_rights() & BLACK_OOO) &&
            !(occ & (square_bb(SQ_D8) | square_bb(SQ_C8) | square_bb(SQ_B8))) &&
            !board.is_square_attacked(SQ_E8, WHITE) &&
            !board.is_square_attacked(SQ_D8, WHITE) &&
            !board.is_square_attacked(SQ_C8, WHITE))
        {
            list.push(Move(SQ_E8, SQ_C8, MT_CASTLING));
        }
    }
}

// ============================================================
// Public API
// ============================================================

void generate_moves(const Board& board, MoveList& list) {
    list.count = 0;

    generate_pawn_moves(board, list, false);
    generate_piece_moves(board, list, KNIGHT, false);
    generate_piece_moves(board, list, BISHOP, false);
    generate_piece_moves(board, list, ROOK, false);
    generate_piece_moves(board, list, QUEEN, false);
    generate_piece_moves(board, list, KING, false);
    generate_castling_moves(board, list);

    // Filter to legal moves: make move, check if own king is attacked, unmake
    Board temp = board;  // copy board for legality testing
    MoveList legal;
    legal.count = 0;

    for (int i = 0; i < list.count; ++i) {
        Move m = list[i];
        temp = board;
        temp.make_move(m);
        // After make_move, side has flipped, so the king we need to check is ~temp.side_to_move()
        if (!temp.is_square_attacked(temp.king_sq(~temp.side_to_move()), temp.side_to_move())) {
            legal.push(m);
        }
    }

    list = legal;
}

void generate_captures(const Board& board, MoveList& list) {
    list.count = 0;

    generate_pawn_moves(board, list, true);
    generate_piece_moves(board, list, KNIGHT, true);
    generate_piece_moves(board, list, BISHOP, true);
    generate_piece_moves(board, list, ROOK, true);
    generate_piece_moves(board, list, QUEEN, true);
    generate_piece_moves(board, list, KING, true);

    // Filter to legal
    Board temp = board;
    MoveList legal;
    legal.count = 0;

    for (int i = 0; i < list.count; ++i) {
        Move m = list[i];
        temp = board;
        temp.make_move(m);
        if (!temp.is_square_attacked(temp.king_sq(~temp.side_to_move()), temp.side_to_move())) {
            legal.push(m);
        }
    }

    list = legal;
}

} // namespace chess
