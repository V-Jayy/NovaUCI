#include "board.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace chess {

// ============================================================
// Castling rights update table
// When a piece moves from/to a square, AND castling with this
// ============================================================
static constexpr int CastlingMask[SQUARE_NB] = {
    ~WHITE_OOO, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ~(WHITE_OO | WHITE_OOO), ALL_CASTLING, ALL_CASTLING, ~WHITE_OO,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING,
    ~BLACK_OOO, ALL_CASTLING, ALL_CASTLING, ALL_CASTLING, ~(BLACK_OO | BLACK_OOO), ALL_CASTLING, ALL_CASTLING, ~BLACK_OO,
};

// ============================================================
// Constructor & setup
// ============================================================
Board::Board() {
    set_startpos();
}

void Board::set_startpos() {
    set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

// ============================================================
// Internal bitboard helpers
// ============================================================
void Board::put_piece(Piece p, Square sq) {
    board_[sq] = p;
    Bitboard bb = square_bb(sq);
    type_bb_[piece_type(p)] |= bb;
    color_bb_[piece_color(p)] |= bb;
}

void Board::remove_piece(Square sq) {
    Piece p = board_[sq];
    if (p == NO_PIECE) return;
    Bitboard bb = square_bb(sq);
    type_bb_[piece_type(p)] ^= bb;
    color_bb_[piece_color(p)] ^= bb;
    board_[sq] = NO_PIECE;
}

void Board::move_piece(Square from, Square to) {
    Piece p = board_[from];
    Bitboard fb = square_bb(from) | square_bb(to);
    type_bb_[piece_type(p)] ^= fb;
    color_bb_[piece_color(p)] ^= fb;
    board_[from] = NO_PIECE;
    board_[to] = p;
}

// ============================================================
// FEN parsing
// ============================================================
void Board::set_fen(std::string_view fen) {
    // Clear
    for (auto& sq : board_) sq = NO_PIECE;
    for (auto& bb : type_bb_) bb = EMPTY_BB;
    for (auto& bb : color_bb_) bb = EMPTY_BB;
    history_.clear();

    std::string fen_str(fen);
    std::istringstream ss(fen_str);
    std::string piece_placement, side, castling, ep;
    int hm = 0, fm = 1;

    ss >> piece_placement >> side >> castling >> ep;
    if (!(ss >> hm)) hm = 0;
    if (!(ss >> fm)) fm = 1;

    // Parse pieces
    int rank = 7, file = 0;
    for (char c : piece_placement) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            file += c - '0';
        } else {
            Color color = std::isupper(c) ? WHITE : BLACK;
            PieceType pt = NO_PIECE_TYPE;
            switch (std::tolower(c)) {
                case 'p': pt = PAWN;   break;
                case 'n': pt = KNIGHT; break;
                case 'b': pt = BISHOP; break;
                case 'r': pt = ROOK;   break;
                case 'q': pt = QUEEN;  break;
                case 'k': pt = KING;   break;
            }
            if (pt != NO_PIECE_TYPE) {
                put_piece(make_piece(color, pt), make_square(File(file), Rank(rank)));
            }
            file++;
        }
    }

    // Side to move
    side_ = (side == "b") ? BLACK : WHITE;

    // Castling rights
    castling_ = NO_CASTLING;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castling_ |= WHITE_OO;  break;
                case 'Q': castling_ |= WHITE_OOO; break;
                case 'k': castling_ |= BLACK_OO;  break;
                case 'q': castling_ |= BLACK_OOO; break;
            }
        }
    }

    // En passant
    ep_square_ = (ep != "-") ? string_to_square(ep) : SQ_NONE;

    halfmove_ = hm;
    fullmove_ = fm;

    compute_hash();
}

std::string Board::to_fen() const {
    std::string fen;

    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece p = board_[make_square(File(f), Rank(r))];
            if (p == NO_PIECE) {
                empty++;
            } else {
                if (empty > 0) { fen += char('0' + empty); empty = 0; }
                constexpr char PieceChar[] = " PNBRQK  pnbrqk";
                fen += PieceChar[p];
            }
        }
        if (empty > 0) fen += char('0' + empty);
        if (r > 0) fen += '/';
    }

    fen += ' ';
    fen += (side_ == WHITE) ? 'w' : 'b';

    fen += ' ';
    if (castling_ == NO_CASTLING) {
        fen += '-';
    } else {
        if (castling_ & WHITE_OO)  fen += 'K';
        if (castling_ & WHITE_OOO) fen += 'Q';
        if (castling_ & BLACK_OO)  fen += 'k';
        if (castling_ & BLACK_OOO) fen += 'q';
    }

    fen += ' ';
    fen += (ep_square_ != SQ_NONE) ? square_to_string(ep_square_) : "-";

    fen += ' ';
    fen += std::to_string(halfmove_);
    fen += ' ';
    fen += std::to_string(fullmove_);

    return fen;
}

// ============================================================
// Zobrist hash computation
// ============================================================
void Board::compute_hash() {
    hash_ = 0;
    for (int sq = 0; sq < 64; ++sq) {
        if (board_[sq] != NO_PIECE)
            hash_ ^= zobrist().piece_square[board_[sq]][sq];
    }
    if (side_ == BLACK) hash_ ^= zobrist().side;
    hash_ ^= zobrist().castling[castling_];
    if (ep_square_ != SQ_NONE)
        hash_ ^= zobrist().en_passant[file_of(ep_square_)];
    else
        hash_ ^= zobrist().en_passant[FILE_NB]; // "no ep" key
}

// ============================================================
// Attack detection
// ============================================================
bool Board::is_square_attacked(Square sq, Color by) const {
    Bitboard occ = occupied();

    // Pawn attacks (check if any pawn of 'by' attacks 'sq')
    if (PawnAttacks[~by][sq] & pieces(by, PAWN)) return true;
    if (KnightAttacks[sq] & pieces(by, KNIGHT)) return true;
    if (KingAttacks[sq] & pieces(by, KING)) return true;
    if (get_bishop_attacks(sq, occ) & (pieces(by, BISHOP) | pieces(by, QUEEN))) return true;
    if (get_rook_attacks(sq, occ) & (pieces(by, ROOK) | pieces(by, QUEEN))) return true;

    return false;
}

Bitboard Board::attackers_to(Square sq, Bitboard occupied) const {
    return (PawnAttacks[WHITE][sq] & pieces(BLACK, PAWN))
         | (PawnAttacks[BLACK][sq] & pieces(WHITE, PAWN))
         | (KnightAttacks[sq] & pieces(KNIGHT))
         | (get_rook_attacks(sq, occupied) & (pieces(ROOK) | pieces(QUEEN)))
         | (get_bishop_attacks(sq, occupied) & (pieces(BISHOP) | pieces(QUEEN)))
         | (KingAttacks[sq] & pieces(KING));
}

// ============================================================
// Make / Unmake move
// ============================================================
void Board::make_move(Move m) {
    // Save undo info
    history_.push_back({
        piece_on(m.to()),   // captured piece (NO_PIECE if none)
        castling_,
        ep_square_,
        halfmove_,
        hash_
    });

    Square from = m.from();
    Square to = m.to();
    Piece moving = board_[from];
    Piece captured = board_[to];
    PieceType pt = piece_type(moving);

    // Update hash: remove old castling/ep keys
    hash_ ^= zobrist().castling[castling_];
    if (ep_square_ != SQ_NONE)
        hash_ ^= zobrist().en_passant[file_of(ep_square_)];
    else
        hash_ ^= zobrist().en_passant[FILE_NB];

    // Reset ep
    ep_square_ = SQ_NONE;

    // Update halfmove clock
    halfmove_++;
    if (pt == PAWN || captured != NO_PIECE)
        halfmove_ = 0;

    if (m.is_castling()) {
        // Move the king
        move_piece(from, to);
        hash_ ^= zobrist().piece_square[moving][from];
        hash_ ^= zobrist().piece_square[moving][to];

        // Move the rook
        Square rook_from, rook_to;
        if (to > from) { // King side
            rook_from = Square(to + 1);   // h1 or h8
            rook_to   = Square(to - 1);   // f1 or f8
        } else { // Queen side
            rook_from = Square(to - 2);   // a1 or a8
            rook_to   = Square(to + 1);   // d1 or d8
        }
        Piece rook = board_[rook_from];
        move_piece(rook_from, rook_to);
        hash_ ^= zobrist().piece_square[rook][rook_from];
        hash_ ^= zobrist().piece_square[rook][rook_to];

    } else if (m.is_en_passant()) {
        // Remove the captured pawn
        Square cap_sq = make_square(file_of(to), rank_of(from));
        Piece cap = board_[cap_sq];
        remove_piece(cap_sq);
        hash_ ^= zobrist().piece_square[cap][cap_sq];

        // Move our pawn
        move_piece(from, to);
        hash_ ^= zobrist().piece_square[moving][from];
        hash_ ^= zobrist().piece_square[moving][to];

        // Store the actual captured piece in undo
        history_.back().captured = cap;

    } else if (m.is_promotion()) {
        // Remove captured piece if any
        if (captured != NO_PIECE) {
            remove_piece(to);
            hash_ ^= zobrist().piece_square[captured][to];
        }
        // Remove the pawn
        remove_piece(from);
        hash_ ^= zobrist().piece_square[moving][from];

        // Place the promoted piece
        Piece promo = make_piece(side_, m.promotion_type());
        put_piece(promo, to);
        hash_ ^= zobrist().piece_square[promo][to];

    } else {
        // Normal move
        if (captured != NO_PIECE) {
            remove_piece(to);
            hash_ ^= zobrist().piece_square[captured][to];
        }
        move_piece(from, to);
        hash_ ^= zobrist().piece_square[moving][from];
        hash_ ^= zobrist().piece_square[moving][to];

        // Double pawn push — set ep square
        if (pt == PAWN && std::abs(int(to) - int(from)) == 16) {
            ep_square_ = Square((int(from) + int(to)) / 2);
        }
    }

    // Update castling rights
    castling_ &= CastlingMask[from];
    castling_ &= CastlingMask[to];

    // Add new castling/ep keys to hash
    hash_ ^= zobrist().castling[castling_];
    if (ep_square_ != SQ_NONE)
        hash_ ^= zobrist().en_passant[file_of(ep_square_)];
    else
        hash_ ^= zobrist().en_passant[FILE_NB];

    // Flip side
    side_ = ~side_;
    hash_ ^= zobrist().side;

    if (side_ == WHITE) fullmove_++;
}

void Board::unmake_move(Move m) {
    UndoInfo undo = history_.back();
    
    // Flip side back
    side_ = ~side_;
    if (side_ == BLACK) fullmove_--;

    Square from = m.from();
    Square to = m.to();

    if (m.is_castling()) {
        // Move king back
        move_piece(to, from);

        // Move rook back
        Square rook_from, rook_to;
        if (to > from) { // King side
            rook_from = Square(to + 1);
            rook_to   = Square(to - 1);
        } else { // Queen side
            rook_from = Square(to - 2);
            rook_to   = Square(to + 1);
        }
        move_piece(rook_to, rook_from);

    } else if (m.is_en_passant()) {
        // Move pawn back
        move_piece(to, from);
        // Restore captured pawn
        Square cap_sq = make_square(file_of(to), rank_of(from));
        put_piece(undo.captured, cap_sq);

    } else if (m.is_promotion()) {
        // Remove promoted piece
        remove_piece(to);
        // Restore pawn
        put_piece(make_piece(side_, PAWN), from);
        // Restore captured piece
        if (undo.captured != NO_PIECE) {
            put_piece(undo.captured, to);
        }

    } else {
        // Normal move — move piece back
        move_piece(to, from);
        // Restore captured piece
        if (undo.captured != NO_PIECE) {
            put_piece(undo.captured, to);
        }
    }

    // Restore state
    castling_ = undo.castling_rights;
    ep_square_ = undo.en_passant;
    halfmove_ = undo.halfmove_clock;
    hash_ = undo.hash;

    history_.pop_back();
}

void Board::make_null_move() {
    history_.push_back({
        NO_PIECE,
        castling_,
        ep_square_,
        halfmove_,
        hash_
    });

    if (ep_square_ != SQ_NONE)
        hash_ ^= zobrist().en_passant[file_of(ep_square_)];
    else
        hash_ ^= zobrist().en_passant[FILE_NB];

    ep_square_ = SQ_NONE;
    side_ = ~side_;
    hash_ ^= zobrist().side;
    hash_ ^= zobrist().en_passant[FILE_NB];

    if (side_ == WHITE) fullmove_++;
    halfmove_++;
}

void Board::unmake_null_move() {
    UndoInfo undo = history_.back();
    
    side_ = ~side_;
    if (side_ == BLACK) fullmove_--;

    castling_ = undo.castling_rights;
    ep_square_ = undo.en_passant;
    halfmove_ = undo.halfmove_clock;
    hash_ = undo.hash;

    history_.pop_back();
}

} // namespace chess
