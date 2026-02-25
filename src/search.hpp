#pragma once

#include "board.hpp"
#include <chrono>
#include <functional>

namespace chess {

// ============================================================
// Transposition table entry
// ============================================================
enum TTFlag : uint8_t {
    TT_EXACT,
    TT_ALPHA,  // upper bound (failed low)
    TT_BETA    // lower bound (failed high)
};

struct TTEntry {
    uint64_t key = 0;
    int      depth = 0;
    int      score = 0;
    TTFlag   flag = TT_EXACT;
    Move     best_move{};
    uint8_t  age = 0;
};

// ============================================================
// Search information
// ============================================================
struct SearchInfo {
    int      depth = 0;
    int      max_depth = 64;
    int      nodes = 0;
    bool     stopped = false;

    // Aspiration windows
    int      last_score = 0;

    // Time management
    std::chrono::steady_clock::time_point start_time;
    int64_t  time_limit_ms = 0;  // 0 = no limit
    bool     infinite = false;

    void check_time() {
        if (time_limit_ms > 0 && (nodes & 2047) == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            if (elapsed >= time_limit_ms)
                stopped = true;
        }
    }
};

// ============================================================
// Searcher
// ============================================================
class Searcher {
public:
    Searcher();

    // Run iterative deepening search and return the best move
    Move search(Board& board, SearchInfo& info);

    // Clear transposition table and heuristics
    void clear();

private:
    // Transposition table
    static constexpr int TT_SIZE = 1 << 20; // ~1M entries
    std::vector<TTEntry> tt_;

    // Killer moves (2 per ply)
    Move killers_[256][2];

    // History heuristic [color][from][to]
    int history_[2][64][64];

    // PV tracking
    Move pv_table_[256][256];
    int  pv_length_[256];

    // Core search functions
    int alpha_beta(Board& board, SearchInfo& info, int alpha, int beta, int depth, int ply, Move excluded_move = MOVE_NONE);
    int quiescence(Board& board, SearchInfo& info, int alpha, int beta, int ply);

    // Move ordering
    void score_moves(const Board& board, MoveList& moves, Move tt_move, int ply);

    // TT probing
    TTEntry* probe_tt(uint64_t key);
    void store_tt(uint64_t key, int depth, int score, TTFlag flag, Move best);

    // LMR
    int reductions_[64][64];
    void init_reductions();

    // TT aging / Stability
    uint8_t tt_age_ = 0;
    Move    last_best_move_{};
    int     best_move_changes_ = 0;
};

} // namespace chess
