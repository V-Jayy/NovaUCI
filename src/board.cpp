#include "board.h"
#include <sstream>
#include <cctype>
#include <array>

namespace nova {

static const int KnightMoves[8] = { -33, -31, -18, -14, 14, 18, 31, 33 };
static const int BishopMoves[4] = { -17, -15, 15, 17 };
static const int RookMoves[4] = { -16, -1, 1, 16 };
static const int KingMoves[8] = { -17, -16, -15, -1, 1, 15, 16, 17 };

static bool onBoard(int sq) {
    return (sq & 0x88) == 0;
}

Board::Board() : side(WHITE), castlingRights(0), enPassant(-1) {
    for (int i = 0; i < 128; ++i) squares[i] = EMPTY;
}

void Board::setFEN(const std::string& fen) {
    for (int i = 0; i < 128; ++i) squares[i] = EMPTY;
    std::istringstream iss(fen);
    std::string boardPart, sidePart, castlePart, epPart;
    iss >> boardPart >> sidePart >> castlePart >> epPart;
    int rank = 7;
    int file = 0;
    for (char c : boardPart) {
        if (c == '/') { rank--; file = 0; continue; }
        if (std::isdigit(c)) {
            int empty = c - '0';
            for (int i = 0; i < empty; ++i) {
                squares[rank * 16 + file] = EMPTY;
                ++file;
            }
        } else {
            Piece p = EMPTY;
            switch (c) {
                case 'P': p = WP; break;
                case 'N': p = WN; break;
                case 'B': p = WB; break;
                case 'R': p = WR; break;
                case 'Q': p = WQ; break;
                case 'K': p = WK; break;
                case 'p': p = BP; break;
                case 'n': p = BN; break;
                case 'b': p = BB; break;
                case 'r': p = BR; break;
                case 'q': p = BQ; break;
                case 'k': p = BK; break;
            }
            squares[rank * 16 + file] = p;
            ++file;
        }
    }
    side = (sidePart == "w") ? WHITE : BLACK;
    castlingRights = 0;
    for (char c : castlePart) {
        switch (c) {
            case 'K': castlingRights |= 1; break;
            case 'Q': castlingRights |= 2; break;
            case 'k': castlingRights |= 4; break;
            case 'q': castlingRights |= 8; break;
        }
    }
    if (epPart == "-") enPassant = -1;
    else {
        int file = epPart[0] - 'a';
        int rank = epPart[1] - '1';
        enPassant = rank * 16 + file;
    }
}

std::string Board::getFEN() const {
    std::ostringstream fen;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 16 + file;
            int piece = squares[sq];
            if (piece == EMPTY) {
                ++empty;
            } else {
                if (empty) { fen << empty; empty = 0; }
                char c = '?';
                switch (piece) {
                    case WP: c = 'P'; break;
                    case WN: c = 'N'; break;
                    case WB: c = 'B'; break;
                    case WR: c = 'R'; break;
                    case WQ: c = 'Q'; break;
                    case WK: c = 'K'; break;
                    case BP: c = 'p'; break;
                    case BN: c = 'n'; break;
                    case BB: c = 'b'; break;
                    case BR: c = 'r'; break;
                    case BQ: c = 'q'; break;
                    case BK: c = 'k'; break;
                    default: break;
                }
                fen << c;
            }
        }
        if (empty) fen << empty;
        if (rank) fen << '/';
    }
    fen << ' ' << (side == WHITE ? 'w' : 'b') << ' ';
    if (!castlingRights) fen << '-';
    else {
        if (castlingRights & 1) fen << 'K';
        if (castlingRights & 2) fen << 'Q';
        if (castlingRights & 4) fen << 'k';
        if (castlingRights & 8) fen << 'q';
    }
    fen << ' ';
    if (enPassant == -1) fen << '-';
    else {
        int file = enPassant & 7;
        int rank = enPassant >> 4;
        fen << char('a' + file) << char('1' + rank);
    }
    fen << " 0 1"; // halfmove and fullmove placeholders
    return fen.str();
}

bool Board::isSquareAttacked(int sq, Color byColor) const {
    int dir = (byColor == WHITE) ? 16 : -16;
    // pawns
    if (onBoard(sq - dir - 1) && squares[sq - dir - 1] == (byColor == WHITE ? WP : BP)) return true;
    if (onBoard(sq - dir + 1) && squares[sq - dir + 1] == (byColor == WHITE ? WP : BP)) return true;
    // knights
    for (int offset : KnightMoves) {
        int target = sq + offset;
        if (onBoard(target)) {
            if (squares[target] == (byColor == WHITE ? WN : BN)) return true;
        }
    }
    // bishops/queens
    for (int i = 0; i < 4; ++i) {
        int offset = BishopMoves[i];
        int target = sq + offset;
        while (onBoard(target)) {
            int piece = squares[target];
            if (piece != EMPTY) {
                if (piece == (byColor == WHITE ? WB : BB) || piece == (byColor == WHITE ? WQ : BQ)) return true;
                break;
            }
            target += offset;
        }
    }
    // rooks/queens
    for (int i = 0; i < 4; ++i) {
        int offset = RookMoves[i];
        int target = sq + offset;
        while (onBoard(target)) {
            int piece = squares[target];
            if (piece != EMPTY) {
                if (piece == (byColor == WHITE ? WR : BR) || piece == (byColor == WHITE ? WQ : BQ)) return true;
                break;
            }
            target += offset;
        }
    }
    // king
    for (int offset : KingMoves) {
        int target = sq + offset;
        if (onBoard(target)) {
            if (squares[target] == (byColor == WHITE ? WK : BK)) return true;
        }
    }
    return false;
}

void Board::generateMoves(std::vector<Move>& moves) const {
    Color us = side;
    Color them = (side == WHITE) ? BLACK : WHITE;
    for (int sq = 0; sq < 128; ++sq) {
        if (!onBoard(sq)) continue;
        int piece = squares[sq];
        if (piece == EMPTY) continue;
        if ((piece <= WK && us == WHITE) || (piece >= BP && us == BLACK)) {
            switch (piece) {
                case WP: {
                    int fwd = sq + 16;
                    if (onBoard(fwd) && squares[fwd] == EMPTY) {
                        if (fwd >= 0x70) {
                            moves.push_back({sq,fwd,WQ,false,false});
                            moves.push_back({sq,fwd,WR,false,false});
                            moves.push_back({sq,fwd,WB,false,false});
                            moves.push_back({sq,fwd,WN,false,false});
                        } else {
                            moves.push_back({sq,fwd,EMPTY,false,false});
                        }
                        if (sq / 16 == 1) {
                            int fwd2 = sq + 32;
                            if (squares[fwd2] == EMPTY) {
                                moves.push_back({sq,fwd2,EMPTY,false,false});
                            }
                        }
                    }
                    int capL = sq + 15;
                    int capR = sq + 17;
                    if (onBoard(capL) && squares[capL] != EMPTY && squares[capL] >= BP)
                        moves.push_back({sq,capL,EMPTY,false,false});
                    if (onBoard(capR) && squares[capR] != EMPTY && squares[capR] >= BP)
                        moves.push_back({sq,capR,EMPTY,false,false});
                    if (enPassant != -1) {
                        if (capL == enPassant) moves.push_back({sq,capL,EMPTY,true,false});
                        if (capR == enPassant) moves.push_back({sq,capR,EMPTY,true,false});
                    }
                    break;
                }
                case BP: {
                    int fwd = sq - 16;
                    if (onBoard(fwd) && squares[fwd] == EMPTY) {
                        if (fwd < 0x10) {
                            moves.push_back({sq,fwd,BQ,false,false});
                            moves.push_back({sq,fwd,BR,false,false});
                            moves.push_back({sq,fwd,BB,false,false});
                            moves.push_back({sq,fwd,BN,false,false});
                        } else {
                            moves.push_back({sq,fwd,EMPTY,false,false});
                        }
                        if (sq / 16 == 6) {
                            int fwd2 = sq - 32;
                            if (squares[fwd2] == EMPTY) {
                                moves.push_back({sq,fwd2,EMPTY,false,false});
                            }
                        }
                    }
                    int capL = sq - 17;
                    int capR = sq - 15;
                    if (onBoard(capL) && squares[capL] != EMPTY && squares[capL] <= WK)
                        moves.push_back({sq,capL,EMPTY,false,false});
                    if (onBoard(capR) && squares[capR] != EMPTY && squares[capR] <= WK)
                        moves.push_back({sq,capR,EMPTY,false,false});
                    if (enPassant != -1) {
                        if (capL == enPassant) moves.push_back({sq,capL,EMPTY,true,false});
                        if (capR == enPassant) moves.push_back({sq,capR,EMPTY,true,false});
                    }
                    break;
                }
                case WN: case BN: {
                    for (int offset : KnightMoves) {
                        int to = sq + offset;
                        if (!onBoard(to)) continue;
                        int target = squares[to];
                        if (target == EMPTY || (us == WHITE ? target >= BP : target <= WK))
                            moves.push_back({sq,to,EMPTY,false,false});
                    }
                    break;
                }
                case WB: case BB: {
                    for (int offset : BishopMoves) {
                        int to = sq + offset;
                        while (onBoard(to)) {
                            int target = squares[to];
                            if (target == EMPTY) {
                                moves.push_back({sq,to,EMPTY,false,false});
                            } else {
                                if (us == WHITE ? target >= BP : target <= WK)
                                    moves.push_back({sq,to,EMPTY,false,false});
                                break;
                            }
                            to += offset;
                        }
                    }
                    break;
                }
                case WR: case BR: {
                    for (int offset : RookMoves) {
                        int to = sq + offset;
                        while (onBoard(to)) {
                            int target = squares[to];
                            if (target == EMPTY) {
                                moves.push_back({sq,to,EMPTY,false,false});
                            } else {
                                if (us == WHITE ? target >= BP : target <= WK)
                                    moves.push_back({sq,to,EMPTY,false,false});
                                break;
                            }
                            to += offset;
                        }
                    }
                    break;
                }
                case WQ: case BQ: {
                    for (int offset : BishopMoves) {
                        int to = sq + offset;
                        while (onBoard(to)) {
                            int target = squares[to];
                            if (target == EMPTY) {
                                moves.push_back({sq,to,EMPTY,false,false});
                            } else {
                                if (us == WHITE ? target >= BP : target <= WK)
                                    moves.push_back({sq,to,EMPTY,false,false});
                                break;
                            }
                            to += offset;
                        }
                    }
                    for (int offset : RookMoves) {
                        int to = sq + offset;
                        while (onBoard(to)) {
                            int target = squares[to];
                            if (target == EMPTY) {
                                moves.push_back({sq,to,EMPTY,false,false});
                            } else {
                                if (us == WHITE ? target >= BP : target <= WK)
                                    moves.push_back({sq,to,EMPTY,false,false});
                                break;
                            }
                            to += offset;
                        }
                    }
                    break;
                }
                case WK: case BK: {
                    for (int offset : KingMoves) {
                        int to = sq + offset;
                        if (!onBoard(to)) continue;
                        int target = squares[to];
                        if (target == EMPTY || (us == WHITE ? target >= BP : target <= WK))
                            moves.push_back({sq,to,EMPTY,false,false});
                    }
                    // castling
                    if (us == WHITE) {
                        if ((castlingRights & 1) && squares[5] == EMPTY && squares[6] == EMPTY &&
                            !isSquareAttacked(4, them) && !isSquareAttacked(5, them) && !isSquareAttacked(6, them)) {
                            moves.push_back({4,6,EMPTY,false,true});
                        }
                        if ((castlingRights & 2) && squares[1] == EMPTY && squares[2] == EMPTY && squares[3] == EMPTY &&
                            !isSquareAttacked(4, them) && !isSquareAttacked(3, them) && !isSquareAttacked(2, them)) {
                            moves.push_back({4,2,EMPTY,false,true});
                        }
                    } else {
                        if ((castlingRights & 4) && squares[117] == EMPTY && squares[118] == EMPTY &&
                            !isSquareAttacked(116, them) && !isSquareAttacked(117, them) && !isSquareAttacked(118, them)) {
                            moves.push_back({116,118,EMPTY,false,true});
                        }
                        if ((castlingRights & 8) && squares[113] == EMPTY && squares[114] == EMPTY && squares[115] == EMPTY &&
                            !isSquareAttacked(116, them) && !isSquareAttacked(115, them) && !isSquareAttacked(114, them)) {
                            moves.push_back({116,114,EMPTY,false,true});
                        }
                    }
                    break;
                }
                default: break;
            }
        }
    }
}

std::vector<Move> Board::generateLegalMoves() const {
    std::vector<Move> moves;
    generateMoves(moves);
    std::vector<Move> legal;
    for (const Move& m : moves) {
        Board copy = *this;
        if (copy.makeMove(m)) {
            int kingSq = -1;
            for (int sq = 0; sq < 128; ++sq) if (onBoard(sq)) {
                if (copy.squares[sq] == (side == WHITE ? WK : BK)) { kingSq = sq; break; }
            }
            if (kingSq != -1 && !copy.isSquareAttacked(kingSq, side == WHITE ? BLACK : WHITE)) {
                legal.push_back(m);
            }
            copy.undoMove();
        }
    }
    return legal;
}

bool Board::makeMove(const Move& move) {
    int piece = squares[move.from];
    int captured = squares[move.to];
    moveHistory.push_back(move);
    squares[move.to] = piece;
    squares[move.from] = EMPTY;
    if (move.isEnPassant) {
        int capSq = (side == WHITE) ? move.to - 16 : move.to + 16;
        squares[capSq] = EMPTY;
    }
    if (piece == WK) {
        castlingRights &= ~(1 | 2);
        if (move.isCastle) {
            if (move.to == 6) { squares[7] = EMPTY; squares[5] = WR; }
            else if (move.to == 2) { squares[0] = EMPTY; squares[3] = WR; }
        }
    } else if (piece == BK) {
        castlingRights &= ~(4 | 8);
        if (move.isCastle) {
            if (move.to == 118) { squares[119] = EMPTY; squares[117] = BR; }
            else if (move.to == 114) { squares[112] = EMPTY; squares[115] = BR; }
        }
    } else if (piece == WR && move.from == 7) {
        castlingRights &= ~1;
    } else if (piece == WR && move.from == 0) {
        castlingRights &= ~2;
    } else if (piece == BR && move.from == 119) {
        castlingRights &= ~4;
    } else if (piece == BR && move.from == 112) {
        castlingRights &= ~8;
    }
    if (captured != EMPTY) {
        if (move.to == 7) castlingRights &= ~1;
        else if (move.to == 0) castlingRights &= ~2;
        else if (move.to == 119) castlingRights &= ~4;
        else if (move.to == 112) castlingRights &= ~8;
    }
    if (move.promotion != EMPTY) {
        squares[move.to] = move.promotion;
    }
    if (piece == WP && move.to - move.from == 32) {
        enPassant = move.from + 16;
    } else if (piece == BP && move.from - move.to == 32) {
        enPassant = move.from - 16;
    } else {
        enPassant = -1;
    }
    side = (side == WHITE) ? BLACK : WHITE;
    return true;
}

void Board::undoMove() {
    if (moveHistory.empty()) return;
    Move move = moveHistory.back();
    moveHistory.pop_back();
    side = (side == WHITE) ? BLACK : WHITE;
    int piece = squares[move.to];
    squares[move.from] = piece;
    squares[move.to] = EMPTY;
    if (move.promotion != EMPTY) {
        squares[move.from] = (side == WHITE ? WP : BP);
    }
    if (move.isEnPassant) {
        int capSq = (side == WHITE) ? move.to - 16 : move.to + 16;
        squares[capSq] = (side == WHITE ? BP : WP);
    }
    if (piece == WK && move.isCastle) {
        if (move.to == 6) { squares[7] = WR; squares[5] = EMPTY; }
        else if (move.to == 2) { squares[0] = WR; squares[3] = EMPTY; }
    } else if (piece == BK && move.isCastle) {
        if (move.to == 118) { squares[119] = BR; squares[117] = EMPTY; }
        else if (move.to == 114) { squares[112] = BR; squares[115] = EMPTY; }
    }
    // NOTE: castling rights and enPassant are not fully restored in this demo implementation
}

bool Board::inCheck() const {
    int kingSq = -1;
    for (int sq = 0; sq < 128; ++sq) if (onBoard(sq)) {
        if (squares[sq] == (side == WHITE ? WK : BK)) { kingSq = sq; break; }
    }
    if (kingSq == -1) return false;
    return isSquareAttacked(kingSq, side == WHITE ? BLACK : WHITE);
}

} // namespace nova
