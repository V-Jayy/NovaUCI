#ifndef NOVA_BOARD_H
#define NOVA_BOARD_H

#include <string>
#include <vector>

namespace nova {

enum Piece {
    EMPTY = 0,
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK
};

enum Color {
    WHITE = 0,
    BLACK = 1
};

struct Move {
    int from;
    int to;
    Piece promotion;
    bool isEnPassant;
    bool isCastle;
};

class Board {
public:
    Board();
    void setFEN(const std::string& fen);
    std::string getFEN() const;
    std::vector<Move> generateLegalMoves() const;
    bool makeMove(const Move& move);
    void undoMove();
    Color sideToMove() const { return side; }

    int pieceAt(int sq) const { return squares[sq]; }
    bool isAttacked(int sq, Color byColor) const { return isSquareAttacked(sq, byColor); }
    bool inCheck() const;
    void setSide(Color c) { side = c; }

private:
    int squares[128]; // 0x88 board
    Color side;
    int castlingRights; // 1=Wk,2=Wq,4=Bk,8=Bq
    int enPassant; // square index or -1

    mutable std::vector<Move> moveHistory;

    bool isSquareAttacked(int sq, Color byColor) const;
    void generateMoves(std::vector<Move>& moves) const;
};

} // namespace nova

#endif // NOVA_BOARD_H
