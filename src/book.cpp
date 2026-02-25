#include "book.hpp"
#include "movegen.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <ctime>

namespace chess {

// ============================================================
// Book entry: a move stored as UCI string with a weight
// ============================================================
struct BookEntry {
    Move move;
    int  weight;
};

// Zobrist-indexed book: hash -> list of candidate moves
static std::unordered_map<uint64_t, std::vector<BookEntry>> g_book;
static bool g_book_initialized = false;

// ============================================================
// Helper: parse a UCI move string against a board and find the legal move
// ============================================================
static Move find_legal_move(const Board& board, const std::string& uci_str) {
    if (uci_str.size() < 4) return MOVE_NONE;

    Square from = string_to_square(uci_str.substr(0, 2));
    Square to   = string_to_square(uci_str.substr(2, 2));
    if (from == SQ_NONE || to == SQ_NONE) return MOVE_NONE;

    MoveList moves;
    generate_moves(board, moves);

    for (int i = 0; i < moves.count; ++i) {
        Move m = moves[i];
        if (m.from() == from && m.to() == to) {
            if (m.is_promotion()) {
                if (uci_str.size() < 5) continue;
                PieceType promo = m.promotion_type();
                PieceType requested = NO_PIECE_TYPE;
                switch (uci_str[4]) {
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

// ============================================================
// Add a full opening line to the book
// Each position along the line gets the NEXT move as a book entry
// ============================================================
static void add_line(const std::string& moves_str, int weight = 100) {
    Board board;
    board.set_startpos();

    std::istringstream iss(moves_str);
    std::string uci_move;
    std::vector<std::string> move_list;

    while (iss >> uci_move) {
        move_list.push_back(uci_move);
    }

    // Replay the line, at each position add the next move as a book entry
    Board replay;
    replay.set_startpos();

    for (size_t i = 0; i < move_list.size(); ++i) {
        uint64_t key = replay.hash_key();
        Move m = find_legal_move(replay, move_list[i]);
        if (m == MOVE_NONE) break; // invalid move, stop

        // Check if this move is already in the book for this position
        auto& entries = g_book[key];
        bool found = false;
        for (auto& e : entries) {
            if (e.move == m) {
                e.weight += weight; // increase weight if already present
                found = true;
                break;
            }
        }
        if (!found) {
            entries.push_back({m, weight});
        }

        replay.make_move(m);
    }
}

// ============================================================
// Opening book data — comprehensive opening lines
// ============================================================
void init_book() {
    if (g_book_initialized) return;
    g_book_initialized = true;

    // Seed RNG for weighted random selection
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // ============================================================
    // OPEN GAMES (1.e4 e5)
    // ============================================================

    // Italian Game — Giuoco Piano
    add_line("e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 c2c3 g8f6 d2d4 e5d4 c3d4 c5b4 b1c3", 150);
    add_line("e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 d2d3 g8f6 c2c3", 120);

    // Italian — Two Knights Defense
    add_line("e2e4 e7e5 g1f3 b8c6 f1c4 g8f6 d2d4 e5d4 e4e5 d7d5 c4b5 f6e4", 120);
    add_line("e2e4 e7e5 g1f3 b8c6 f1c4 g8f6 f3g5 d7d5 e4d5 c6a5 c4b5 c7c6", 100);

    // Ruy Lopez — Main Line / Morphy Defense
    add_line("e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 d7d6 c2c3 e8g8", 200);
    add_line("e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 e8g8 c2c3 d7d6", 180);

    // Ruy Lopez — Berlin Defense
    add_line("e2e4 e7e5 g1f3 b8c6 f1b5 g8f6 e1g1 f6e4 d2d4 f8e7 d1e2 e4d6 b5c6 b7c6 d4e5 d6b7", 150);

    // Ruy Lopez — Exchange Variation
    add_line("e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5c6 d7c6 e1g1 f7f6 d2d4", 120);

    // Scotch Game
    add_line("e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4 g8f6 b1c3 f8b4 d4c6 b7c6 f1d3", 120);
    add_line("e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4 f8c5 b1c3 d8f6 d4b5", 100);

    // Petrov's Defense
    add_line("e2e4 e7e5 g1f3 g8f6 f3e5 d7d6 e5f3 f6e4 d2d4 d6d5 f1d3 b8c6 e1g1 f8e7", 130);

    // King's Gambit
    add_line("e2e4 e7e5 f2f4 e5f4 g1f3 g7g5 h2h4 g5g4 f3e5", 80);
    add_line("e2e4 e7e5 f2f4 e5f4 g1f3 d7d6 d2d4 g7g5 h2h4 g5g4", 80);

    // Vienna Game
    add_line("e2e4 e7e5 b1c3 g8f6 f2f4 d7d5 f4e5 f6e4 g1f3 f8c5", 80);

    // ============================================================
    // SICILIAN DEFENSE (1.e4 c5)
    // ============================================================

    // Sicilian — Open (Najdorf) 
    add_line("e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 f1e2 e7e5 d4b3", 200);
    add_line("e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 c1g5 e7e6 f2f4", 180);
    add_line("e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 f2f3 e7e5 d4b3", 150);

    // Sicilian — Dragon
    add_line("e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 g7g6 c1e3 f8g7 f2f3 e8g8 d1d2 b8c6", 150);

    // Sicilian — Sveshnikov  
    add_line("e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g8f6 b1c3 e7e5 d4b5 d7d6 c1g5 a7a6 b5a3 b7b5", 140);

    // Sicilian — Classical/Scheveningen
    add_line("e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 b8c6 f1e2 e7e6 e1g1 f8e7 c1e3", 130);

    // Sicilian — Kan
    add_line("e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 a7a6 f1d3 g8f6 e1g1 d7d6", 100);

    // Sicilian — Alapin (2.c3)
    add_line("e2e4 c7c5 c2c3 g8f6 e4e5 f6d5 d2d4 c5d4 c3d4 d7d6 g1f3 b8c6", 110);

    // Sicilian — Closed (2.Nc3)
    add_line("e2e4 c7c5 b1c3 b8c6 g2g3 g7g6 f1g2 f8g7 d2d3 d7d6 g1e2", 90);

    // Sicilian — Rossolimo (3.Bb5)
    add_line("e2e4 c7c5 g1f3 b8c6 f1b5 g7g6 e1g1 f8g7 f1e1 e7e5 b2b3", 120);

    // ============================================================
    // FRENCH DEFENSE (1.e4 e6)
    // ============================================================

    // French — Winawer  
    add_line("e2e4 e7e6 d2d4 d7d5 b1c3 f8b4 e4e5 c7c5 a2a3 b4c3 b2c3 g8e7 g1f3", 130);

    // French — Classical
    add_line("e2e4 e7e6 d2d4 d7d5 b1c3 g8f6 c1g5 f8e7 e4e5 f6d7 g5e7 d8e7", 120);

    // French — Tarrasch
    add_line("e2e4 e7e6 d2d4 d7d5 b1d2 g8f6 e4e5 f6d7 f1d3 c7c5 c2c3 b8c6 g1e2", 110);

    // French — Advance  
    add_line("e2e4 e7e6 d2d4 d7d5 e4e5 c7c5 c2c3 b8c6 g1f3 d8b6 a2a3", 100);

    // ============================================================
    // CARO-KANN DEFENSE (1.e4 c6)
    // ============================================================

    // Caro-Kann — Classical
    add_line("e2e4 c7c6 d2d4 d7d5 b1c3 d5e4 c3e4 c8f5 e4g3 f5g6 h2h4 h7h6 g1f3 b8d7", 130);

    // Caro-Kann — Advance
    add_line("e2e4 c7c6 d2d4 d7d5 e4e5 c8f5 g1f3 e7e6 f1e2 c7c5 e1g1 b8c6", 110);

    // Caro-Kann — Exchange
    add_line("e2e4 c7c6 d2d4 d7d5 e4d5 c6d5 f1d3 b8c6 c2c3 g8f6 c1f4", 100);

    // ============================================================
    // SCANDINAVIAN (1.e4 d5)
    // ============================================================
    add_line("e2e4 d7d5 e4d5 d8d5 b1c3 d5a5 d2d4 g8f6 g1f3 c8f5 f1c4", 100);
    add_line("e2e4 d7d5 e4d5 g8f6 d2d4 f6d5 g1f3 c8g4 f1e2 e7e6 e1g1", 90);

    // ============================================================
    // PIRC/MODERN (1.e4 d6 / 1.e4 g6)
    // ============================================================
    add_line("e2e4 d7d6 d2d4 g8f6 b1c3 g7g6 g1f3 f8g7 f1e2 e8g8 e1g1 c7c6", 90);
    add_line("e2e4 g7g6 d2d4 f8g7 b1c3 d7d6 g1f3 g8f6 f1e2 e8g8 e1g1", 80);

    // ============================================================
    // QUEEN'S GAMBIT (1.d4 d5 2.c4)
    // ============================================================

    // QGD — Orthodox 
    add_line("d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 c1g5 f8e7 e2e3 e8g8 g1f3 b8d7 a1c1 c7c6", 180);

    // QGD — Ragozin
    add_line("d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 f8b4 c4d5 e6d5 c1g5", 120);

    // QGA — Queen's Gambit Accepted
    add_line("d2d4 d7d5 c2c4 d5c4 g1f3 g8f6 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6 d1e2", 130);

    // Slav Defense  
    add_line("d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 d5c4 a2a4 c8f5 e2e3 e7e6 f1c4", 140);
    add_line("d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 e2e3 c8f5 b1c3 e7e6 f1d3 f5d3 d1d3", 110);

    // Semi-Slav
    add_line("d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 e7e6 e2e3 b8d7 f1d3 d5c4 d3c4", 130);

    // ============================================================
    // INDIAN DEFENSES (1.d4 Nf6)
    // ============================================================

    // King's Indian Defense — Classical
    add_line("d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 g1f3 e8g8 f1e2 e7e5 e1g1 b8c6", 170);

    // KID — Sämisch
    add_line("d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 f2f3 e8g8 c1e3 e7e5 d4d5", 110);

    // Nimzo-Indian
    add_line("d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 d1c2 e8g8 a2a3 b4c3 c2c3 b7b6 c1g5", 150);
    add_line("d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 e2e3 e8g8 f1d3 d7d5 g1f3 c7c5", 140);

    // Queen's Indian  
    add_line("d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 g2g3 c8b7 f1g2 f8e7 e1g1 e8g8 b1c3 f6e4", 130);

    // Grünfeld Defense
    add_line("d2d4 g8f6 c2c4 g7g6 b1c3 d7d5 c4d5 f6d5 e2e4 d5c3 b2c3 f8g7 g1f3 c7c5 f1e2", 140);

    // Benoni  
    add_line("d2d4 g8f6 c2c4 c7c5 d4d5 e7e6 b1c3 e6d5 c4d5 d7d6 e2e4 g7g6 g1f3 f8g7", 100);

    // Bogo-Indian
    add_line("d2d4 g8f6 c2c4 e7e6 g1f3 f8b4 c1d2 b4d2 d1d2 e8g8 b1c3 d7d5 e2e3", 90);

    // ============================================================
    // ENGLISH OPENING (1.c4)
    // ============================================================
    add_line("c2c4 e7e5 b1c3 g8f6 g1f3 b8c6 g2g3 f8b4 f1g2 e8g8 e1g1 e5e4 f3e1", 110);
    add_line("c2c4 c7c5 b1c3 g8f6 g2g3 d7d5 c4d5 f6d5 f1g2 b8c6 g1f3 e7e6", 100);
    add_line("c2c4 g8f6 b1c3 e7e5 g1f3 b8c6 g2g3 f8c5 f1g2 d7d6 e1g1 e8g8", 100);

    // ============================================================
    // RÉTI OPENING (1.Nf3)
    // ============================================================
    add_line("g1f3 d7d5 g2g3 g8f6 f1g2 g7g6 e1g1 f8g7 d2d3 e8g8 b1d2 b8c6 e2e4", 90);
    add_line("g1f3 d7d5 c2c4 e7e6 g2g3 g8f6 f1g2 f8e7 e1g1 e8g8 b2b3 b7b6", 80);

    // ============================================================
    // LONDON SYSTEM (1.d4 + 2.Bf4/Nf3)
    // ============================================================
    add_line("d2d4 d7d5 g1f3 g8f6 c1f4 c7c5 e2e3 b8c6 c2c3 d8b6 d1b3 c8f5", 130);
    add_line("d2d4 d7d5 g1f3 g8f6 c1f4 e7e6 e2e3 f8d6 f4d6 d8d6 f1d3 e8g8 e1g1 b7b6", 110);
    add_line("d2d4 g8f6 c1f4 d7d5 e2e3 e7e6 g1f3 f8d6 f4g3 e8g8 f1d3 c7c5", 110);

    // ============================================================
    // CATALAN (1.d4 Nf6 2.c4 e6 3.g3)
    // ============================================================
    add_line("d2d4 g8f6 c2c4 e7e6 g2g3 d7d5 f1g2 f8e7 g1f3 e8g8 e1g1 d5c4 d1c2 a7a6 c2c4", 130);
    add_line("d2d4 g8f6 c2c4 e7e6 g2g3 d7d5 f1g2 d5c4 g1f3 f8e7 e1g1 e8g8 d1c2 a7a6", 120);

    // ============================================================
    // TROMPOWSKY (1.d4 Nf6 2.Bg5)
    // ============================================================
    add_line("d2d4 g8f6 c1g5 f6e4 g5f4 d7d5 f2f3 e4f6 e2e4 e7e6 e4e5", 80);

    // ============================================================
    // DUTCH DEFENSE (1.d4 f5)
    // ============================================================
    add_line("d2d4 f7f5 g2g3 g8f6 f1g2 g7g6 g1f3 f8g7 e1g1 e8g8 c2c4 d7d6", 80);

    // ============================================================
    // Extra common responses to 1.e4 (Alekhine, Modern, etc.)
    // ============================================================

    // Alekhine's Defense
    add_line("e2e4 g8f6 e4e5 f6d5 d2d4 d7d6 g1f3 c8g4 f1e2 e7e6 e1g1 f8e7", 80);

    // ============================================================
    // Symmetrical English
    // ============================================================
    add_line("c2c4 c7c5 g1f3 b8c6 b1c3 g8f6 g2g3 g7g6 f1g2 f8g7 e1g1 e8g8 d2d3", 90);
}

// ============================================================
// Book probe — returns a move or MOVE_NONE
// ============================================================
Move probe_book(const Board& board) {
    if (!g_book_initialized) return MOVE_NONE;

    uint64_t key = board.hash_key();
    auto it = g_book.find(key);
    if (it == g_book.end()) return MOVE_NONE;

    const auto& entries = it->second;
    if (entries.empty()) return MOVE_NONE;

    // Weighted random selection
    int total_weight = 0;
    for (const auto& e : entries) total_weight += e.weight;

    int roll = std::rand() % total_weight;
    int cumulative = 0;
    for (const auto& e : entries) {
        cumulative += e.weight;
        if (roll < cumulative) {
            // Verify the move is still legal in the current position
            MoveList moves;
            generate_moves(board, moves);
            for (int i = 0; i < moves.count; ++i) {
                if (moves[i] == e.move) return e.move;
            }
            break;
        }
    }

    return MOVE_NONE;
}

} // namespace chess
