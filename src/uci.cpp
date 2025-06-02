#include "board.h"
#include "search.h"
#include <iostream>
#include <sstream>
#include <random>

namespace nova {

static std::mt19937 rng(42);

void uciLoop() {
    Board board;
    board.setFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "uci") {
            std::cout << "id name Nova" << std::endl;
            std::cout << "id author Codex" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            board.setFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        } else if (token == "position") {
            std::string type;
            iss >> type;
            if (type == "startpos") {
                board.setFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // default starting position
                std::string movesToken;
                if (iss >> movesToken && movesToken == "moves") {
                    std::string mv;
                    while (iss >> mv) {
                        // parse move from UCI string
                        Move m{};
                        m.from = (mv[1]-'1')*16 + (mv[0]-'a');
                        m.to = (mv[3]-'1')*16 + (mv[2]-'a');
                        m.promotion = EMPTY;
                        if (mv.size() == 5) {
                            switch (mv[4]) {
                                case 'q': m.promotion = (board.sideToMove()==WHITE)?WQ:BQ; break;
                                case 'r': m.promotion = (board.sideToMove()==WHITE)?WR:BR; break;
                                case 'b': m.promotion = (board.sideToMove()==WHITE)?WB:BB; break;
                                case 'n': m.promotion = (board.sideToMove()==WHITE)?WN:BN; break;
                            }
                        }
                        board.makeMove(m);
                    }
                }
            } else if (type == "fen") {
                std::string fen;
                std::string tmp;
                fen = "";
                while (iss >> tmp) {
                    if (tmp == "moves") break;
                    fen += tmp + " ";
                }
                board.setFEN(fen);
                if (tmp == "moves") {
                    std::string mv;
                    while (iss >> mv) {
                        Move m{};
                        m.from = (mv[1]-'1')*16 + (mv[0]-'a');
                        m.to = (mv[3]-'1')*16 + (mv[2]-'a');
                        m.promotion = EMPTY;
                        if (mv.size() == 5) {
                            switch (mv[4]) {
                                case 'q': m.promotion = (board.sideToMove()==WHITE)?WQ:BQ; break;
                                case 'r': m.promotion = (board.sideToMove()==WHITE)?WR:BR; break;
                                case 'b': m.promotion = (board.sideToMove()==WHITE)?WB:BB; break;
                                case 'n': m.promotion = (board.sideToMove()==WHITE)?WN:BN; break;
                            }
                        }
                        board.makeMove(m);
                    }
                }
            }
        } else if (token == "go") {
            int depth = 3;
            std::string param;
            while (iss >> param) {
                if (param == "depth") {
                    iss >> depth;
                }
            }
            std::vector<Move> avail = board.generateLegalMoves();
            if (avail.empty()) {
                std::cout << "bestmove 0000" << std::endl;
                continue;
            }
            Move best = findBestMove(board, depth);
            int fromFile = best.from & 7; int fromRank = best.from >> 4;
            int toFile = best.to & 7; int toRank = best.to >> 4;
            std::string moveStr;
            moveStr += char('a'+fromFile);
            moveStr += char('1'+fromRank);
            moveStr += char('a'+toFile);
            moveStr += char('1'+toRank);
            if (best.promotion != EMPTY) {
                char pc = 'q';
                switch (best.promotion) {
                    case WN: case BN: pc='n'; break;
                    case WB: case BB: pc='b'; break;
                    case WR: case BR: pc='r'; break;
                    default: pc='q'; break;
                }
                moveStr += pc;
            }
            board.makeMove(best);
            std::cout << "bestmove " << moveStr << std::endl;
        } else if (token == "quit") {
            break;
        }
    }
}

} // namespace nova
