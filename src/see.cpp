#include "see.hpp"
#include "attacks.hpp"
#include <algorithm>

namespace chess {

bool see_ge(const Board& board, Move m, int threshold) {
    Square from = m.from();
    Square to = m.to();
    
    // Non-normal moves (castling) generally pass SEE
    if (m.type() == MT_CASTLING) return 0 >= threshold;

    Piece moving = board.piece_on(from);
    Piece captured = board.piece_on(to);
    
    if (m.is_en_passant()) captured = make_piece(~board.side_to_move(), PAWN);

    int swap = PieceValue[piece_type(captured)] - threshold;
    if (swap < 0) return false;

    swap = PieceValue[piece_type(moving)] - swap;
    if (swap <= 0) return true;

    Bitboard occupied = board.occupied() ^ square_bb(from) ^ square_bb(to);
    Color stm = ~board.side_to_move();
    Bitboard attackers = board.attackers_to(to, occupied);
    int res = 1;

    while (true) {
        Bitboard stm_attackers = attackers & board.pieces(stm);
        if (!stm_attackers) break;

        // Locate and remove the next least valuable attacker
        PieceType pt = NO_PIECE_TYPE;
        for (int t = PAWN; t <= KING; ++t) {
            if (stm_attackers & board.pieces(PieceType(t))) {
                pt = PieceType(t);
                break;
            }
        }
        
        Square attacker_sq = lsb(stm_attackers & board.pieces(pt));
        occupied ^= square_bb(attacker_sq);
        
        // Add X-ray attackers behind the captured piece
        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= get_bishop_attacks(to, occupied) & (board.pieces(BISHOP) | board.pieces(QUEEN));
        if (pt == ROOK || pt == QUEEN)
            attackers |= get_rook_attacks(to, occupied) & (board.pieces(ROOK) | board.pieces(QUEEN));
        
        attackers &= occupied;
        
        swap = PieceValue[pt] - swap;
        res ^= 1;

        // stm loses if swap < res
        if (swap < res) break;
        
        stm = ~stm;
    }

    return bool(res);
}

} // namespace chess
