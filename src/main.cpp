// ============================================================
// Nova Chess — UCI-Compliant C++20 Chess Engine
// ============================================================

#include "types.hpp"
#include "attacks.hpp"
#include "board.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include <iostream>
#include <sstream>
#include <string>

using namespace chess;

static Board g_board;
static Searcher g_searcher;

// Parse a UCI move string (e.g. "e2e4", "e7e8q") and find matching legal move
static Move parse_uci_move(const Board& board, std::string_view str) {
    if (str.size() < 4) return MOVE_NONE;

    Square from = string_to_square(str.substr(0, 2));
    Square to   = string_to_square(str.substr(2, 2));
    if (from == SQ_NONE || to == SQ_NONE) return MOVE_NONE;

    MoveList moves;
    generate_moves(board, moves);

    for (int i = 0; i < moves.count; ++i) {
        Move m = moves[i];
        if (m.from() == from && m.to() == to) {
            // Check promotion match
            if (m.is_promotion()) {
                if (str.size() < 5) continue;
                PieceType promo = m.promotion_type();
                char pc = str[4];
                PieceType requested = NO_PIECE_TYPE;
                switch (pc) {
                    case 'n': requested = KNIGHT; break;
                    case 'b': requested = BISHOP; break;
                    case 'r': requested = ROOK;   break;
                    case 'q': requested = QUEEN;  break;
                }
                if (promo == requested) return m;
            } else {
                return m;
            }
        }
    }
    return MOVE_NONE;
}

// Handle "position" command
static void uci_position(std::istringstream& iss) {
    std::string token;
    iss >> token;

    if (token == "startpos") {
        g_board.set_startpos();
        iss >> token; // consume "moves" if present
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token && token != "moves") {
            if (!fen.empty()) fen += ' ';
            fen += token;
        }
        g_board.set_fen(fen);
    }

    // Apply moves
    while (iss >> token) {
        Move m = parse_uci_move(g_board, token);
        if (m != MOVE_NONE) {
            g_board.make_move(m);
        }
    }
}

// Handle "go" command
static void uci_go(std::istringstream& iss) {
    SearchInfo info;
    info.max_depth = 64;
    info.time_limit_ms = 0;
    info.infinite = false;

    std::string token;
    int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0;

    while (iss >> token) {
        if (token == "depth") {
            iss >> info.max_depth;
        } else if (token == "infinite") {
            info.infinite = true;
        } else if (token == "wtime") {
            iss >> wtime;
        } else if (token == "btime") {
            iss >> btime;
        } else if (token == "winc") {
            iss >> winc;
        } else if (token == "binc") {
            iss >> binc;
        } else if (token == "movestogo") {
            iss >> movestogo;
        } else if (token == "movetime") {
            iss >> info.time_limit_ms;
        }
    }

    // Simple time management
    if (!info.infinite && info.time_limit_ms == 0 && (wtime > 0 || btime > 0)) {
        int time_left = (g_board.side_to_move() == WHITE) ? wtime : btime;
        int inc = (g_board.side_to_move() == WHITE) ? winc : binc;

        if (movestogo > 0) {
            info.time_limit_ms = time_left / movestogo + inc;
        } else {
            // Use ~1/30 of remaining time + increment
            info.time_limit_ms = time_left / 30 + inc;
        }

        // Safety margin: don't use more than 80% of remaining time
        if (info.time_limit_ms > time_left * 4 / 5) {
            info.time_limit_ms = time_left * 4 / 5;
        }

        // Minimum search time
        if (info.time_limit_ms < 50) info.time_limit_ms = 50;
    }

    Move best = g_searcher.search(g_board, info);
    std::cout << "bestmove " << best.to_uci() << std::endl;
}

// ============================================================
// Main UCI loop
// ============================================================
int main() {
    // Initialize attack tables
    init_attacks();

    std::cout << "============================================" << std::endl;
    std::cout << "   Nova 1.2 - Advanced Chess Engine" << std::endl;
    std::cout << "   Ported with Stockfish Search DNA" << std::endl;
    std::cout << "============================================" << std::endl;

    // Set default position
    g_board.set_startpos();

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci") {
            std::cout << "id name Nova 1.2" << std::endl;
            std::cout << "id author JoshK & Antigravity" << std::endl;
            std::cout << "uciok" << std::endl;

        } else if (cmd == "isready") {
            std::cout << "readyok" << std::endl;

        } else if (cmd == "ucinewgame") {
            g_board.set_startpos();
            g_searcher.clear();

        } else if (cmd == "position") {
            uci_position(iss);

        } else if (cmd == "go") {
            uci_go(iss);

        } else if (cmd == "quit") {
            break;

        } else if (cmd == "d") {
            // Debug: print board FEN
            std::cout << g_board.to_fen() << std::endl;

        } else if (cmd == "perft") {
            // Debug: count legal moves at current depth
            MoveList moves;
            generate_moves(g_board, moves);
            std::cout << "Legal moves: " << moves.count << std::endl;
            for (int i = 0; i < moves.count; ++i) {
                std::cout << "  " << moves[i].to_uci() << std::endl;
            }
        }
    }

    return 0;
}
