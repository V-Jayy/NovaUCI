#include "search.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "see.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>
#include <cmath>

namespace chess {

// ============================================================
// MVV-LVA scoring table
// [victim][attacker] — higher = better capture
// ============================================================
static constexpr int MVV_LVA[PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    // victim:  NONE  PAWN  KNIGHT BISHOP ROOK  QUEEN KING
    /* NONE   */ {0,    0,    0,     0,     0,    0,    0},
    /* PAWN   */ {0,   105,  104,   103,   102,  101,  100},
    /* KNIGHT */ {0,   205,  204,   203,   202,  201,  200},
    /* BISHOP */ {0,   305,  304,   303,   302,  301,  300},
    /* ROOK   */ {0,   405,  404,   403,   402,  401,  400},
    /* QUEEN  */ {0,   505,  504,   503,   502,  501,  500},
    /* KING   */ {0,    0,    0,     0,     0,    0,    0},
};

// ============================================================
// Searcher implementation
// ============================================================
Searcher::Searcher() : tt_(TT_SIZE) {
    init_reductions();
    clear();
}

void Searcher::clear() {
    std::fill(tt_.begin(), tt_.end(), TTEntry{});
    tt_age_ = 0;
    std::memset(killers_, 0, sizeof(killers_));
    std::memset(history_, 0, sizeof(history_));
    std::memset(pv_table_, 0, sizeof(pv_table_));
    std::memset(pv_length_, 0, sizeof(pv_length_));
    last_best_move_ = MOVE_NONE;
    best_move_changes_ = 0;
}

// ============================================================
// TT operations
// ============================================================
TTEntry* Searcher::probe_tt(uint64_t key) {
    TTEntry& entry = tt_[key % TT_SIZE];
    if (entry.key == key) return &entry;
    return nullptr;
}

void Searcher::store_tt(uint64_t key, int depth, int score, TTFlag flag, Move best) {
    TTEntry& entry = tt_[key % TT_SIZE];
    
    // Replacement scheme:
    // 1. Always replace if it's a new position
    // 2. Replace if the new entry is from a more recent search (aging)
    // 3. Replace if the new entry search was deeper
    if (key != entry.key || (tt_age_ != entry.age) || depth >= entry.depth) {
        entry.key = key;
        entry.depth = depth;
        entry.score = score;
        entry.flag = flag;
        entry.best_move = best;
        entry.age = tt_age_;
    }
}

void Searcher::init_reductions() {
    for (int d = 0; d < 64; ++d) {
        for (int c = 0; c < 64; ++c) {
            if (d == 0 || c == 0) {
                reductions_[d][c] = 0;
            } else {
                reductions_[d][c] = static_cast<int>(0.5 + std::log(d) * std::log(c) / 2.0);
            }
        }
    }
}

// ============================================================
// Move ordering
// ============================================================
struct ScoredMove {
    Move move;
    int  score;
};

void Searcher::score_moves(const Board& board, MoveList& moves, Move tt_move, int ply) {
    // We'll sort moves in-place using a simple selection sort during search
    // For now, we assign scores to guide ordering
    // NOTE: We store scores externally in alpha_beta; this function is a helper
    (void)board; (void)moves; (void)tt_move; (void)ply;
}

// ============================================================
// Quiescence search
// ============================================================
int Searcher::quiescence(Board& board, SearchInfo& info, int alpha, int beta, int ply) {
    info.nodes++;
    info.check_time();
    if (info.stopped) return 0;

    int stand_pat = evaluate(board);

    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList captures;
    generate_captures(board, captures);

    // Score captures by MVV-LVA
    int scores[256];
    for (int i = 0; i < captures.count; ++i) {
        Move m = captures[i];
        PieceType victim = piece_type(board.piece_on(m.to()));
        PieceType attacker = piece_type(board.piece_on(m.from()));
        if (victim == NO_PIECE_TYPE) victim = PAWN; // en passant
        scores[i] = MVV_LVA[victim][attacker];
    }

    // Selection sort + search
    for (int i = 0; i < captures.count; ++i) {
        // Find best remaining capture
        int best_idx = i;
        for (int j = i + 1; j < captures.count; ++j) {
            if (scores[j] > scores[best_idx]) best_idx = j;
        }
        if (best_idx != i) {
            std::swap(captures[i], captures[best_idx]);
            std::swap(scores[i], scores[best_idx]);
        }

        Move m = captures[i];

        // SEE Pruning in Quiescence
        // Don't search captures that lose material
        if (!see_ge(board, m, 0)) continue;

        board.make_move(m);
        int score = -quiescence(board, info, -beta, -alpha, ply + 1);
        board.unmake_move(m);

        if (info.stopped) return 0;
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

// ============================================================
// Alpha-Beta with PVS
// ============================================================
int Searcher::alpha_beta(Board& board, SearchInfo& info, int alpha, int beta, int depth, int ply, Move excluded_move) {
    pv_length_[ply] = ply;

    if (depth <= 0) return quiescence(board, info, alpha, beta, ply);

    info.nodes++;
    info.check_time();
    if (info.stopped) return 0;

    bool is_root = (ply == 0);
    uint64_t key = board.hash_key();

    // TT lookup
    Move tt_move{};
    TTEntry* tt_entry = probe_tt(key);
    if (tt_entry && tt_entry->depth >= depth && !is_root) {
        if (tt_entry->flag == TT_EXACT)
            return tt_entry->score;
        if (tt_entry->flag == TT_ALPHA && tt_entry->score <= alpha)
            return alpha;
        if (tt_entry->flag == TT_BETA && tt_entry->score >= beta)
            return beta;
        tt_move = tt_entry->best_move;
    }
    if (tt_entry) tt_move = tt_entry->best_move;

    // Check extensions
    bool in_check = board.in_check();
    if (in_check) depth++;

    // Pruning that requires static evaluation
    int eval = evaluate(board);

    if (!is_root && !in_check) {
        // Razoring: if eval is way below alpha, it's likely a quiet node that won't improve alpha
        if (depth <= 2 && eval <= alpha - 300 * depth) {
            int score = quiescence(board, info, alpha, beta, ply);
            if (score <= alpha) return score;
        }

        // Reverse Futility Pruning (RFP) / Static Null Move Pruning
        // If static eval is way above beta, assume it will fail high
        int margin = 120 * depth;
        if (depth <= 4 && eval - margin >= beta) {
            return eval;
        }
    }

    // Internal Iterative Reductions (IIR)
    // If we have no TT move at high depth, reduce search depth to find a good move faster
    if (depth >= 6 && !tt_move && !in_check) {
        depth--;
    }

    // Null Move Pruning (NMP)
    // If we can fail high even if we give the opponent a free move, we can likely prune this node
    if (depth >= 3 && !in_check && !is_root && board.has_nonPawn_material(board.side_to_move())) {
        board.make_null_move();
        // R = 2 is standard for simple engines
        int score = -alpha_beta(board, info, -beta, -beta + 1, depth - 1 - 2, ply + 1);
        board.unmake_null_move();

        if (score >= beta) {
            // Verification search at high depths to avoid zugzwang risks
            if (depth >= 7) {
                int v_score = alpha_beta(board, info, alpha, beta, depth - 4, ply);
                if (v_score >= beta) return beta;
            } else {
                return beta;
            }
        }
    }

    // Singular Extensions
    int extension = 0;
    if (depth >= 8 && tt_entry && tt_entry->depth >= depth - 3 && tt_entry->flag != TT_ALPHA && !excluded_move && !is_root) {
        int margin = 2 * depth;
        int tt_score = tt_entry->score;
        int s_score = alpha_beta(board, info, tt_score - margin - 1, tt_score - margin, (depth - 1) / 2, ply, tt_move);
        
        if (s_score < tt_score - margin)
            extension = 1;
    }

    // Generate legal moves
    MoveList moves;
    generate_moves(board, moves);

    // Checkmate / Stalemate
    if (moves.count == 0) {
        if (board.in_check())
            return -VALUE_MATE + ply;  // checkmate
        return VALUE_DRAW;  // stalemate
    }

    // Score moves for ordering
    int move_scores[256];
    for (int i = 0; i < moves.count; ++i) {
        Move m = moves[i];
        if (m == tt_move) {
            move_scores[i] = 100000; // TT move first
        } else {
            Piece captured_piece = board.piece_on(m.to());
            if (captured_piece != NO_PIECE || m.is_en_passant()) {
                PieceType victim = (m.is_en_passant()) ? PAWN : piece_type(captured_piece);
                PieceType attacker = piece_type(board.piece_on(m.from()));
                move_scores[i] = 50000 + MVV_LVA[victim][attacker];
                // Boost captures that are safe/winning
                if (see_ge(board, m, 0)) move_scores[i] += 10000;
            } else if (m == killers_[ply][0]) {
                move_scores[i] = 40000;
            } else if (m == killers_[ply][1]) {
                move_scores[i] = 39000;
            } else {
                Color us = board.side_to_move();
                move_scores[i] = history_[us][m.from()][m.to()];
            }
        }
    }

    Move best_move{};
    int best_score = -VALUE_INFINITE;
    TTFlag tt_flag = TT_ALPHA;
    int legal_count = 0;

    for (int i = 0; i < moves.count; ++i) {
        // Selection sort: pick best-scoring move
        int best_idx = i;
        for (int j = i + 1; j < moves.count; ++j) {
            if (move_scores[j] > move_scores[best_idx]) best_idx = j;
        }
        if (best_idx != i) {
            std::swap(moves[i], moves[best_idx]);
            std::swap(move_scores[i], move_scores[best_idx]);
        }

        Move m = moves[i];
        if (m == excluded_move) continue;

        Piece captured = board.piece_on(m.to());

        // SEE Pruning in Search
        // Prune captures that lose material at shallow depths
        if (depth <= 4 && captured != NO_PIECE && !see_ge(board, m, 0)) {
            continue;
        }

        // Late Move Pruning (LMP)
        // Skip quiet moves deep in the move list at shallow depths
        int lmp_threshold = 3 + 2 * depth * depth;
        if (depth <= 4 && !in_check && legal_count > lmp_threshold && captured == NO_PIECE && !m.is_promotion() && !m.is_en_passant()) {
            continue;
        }

        board.make_move(m);
        legal_count++;

        int score;
        if (legal_count == 1) {
            // Full window search for the first move (PV move)
            score = -alpha_beta(board, info, -beta, -alpha, depth - 1 + extension, ply + 1);
        } else {
            // Late Move Reductions (LMR)
            int r = 0;
            if (depth >= 3 && legal_count > 4 && captured == NO_PIECE && !in_check && !m.is_promotion()) {
                r = reductions_[std::min(63, depth)][std::min(63, legal_count)];
                // Reduce less if not alpha-beta window (already handled by null window)
            }

            // Search with null window and possible reduction
            score = -alpha_beta(board, info, -alpha - 1, -alpha, depth - 1 - r, ply + 1);

            // Re-search if reduced search failed high
            if (score > alpha && r > 0) {
                score = -alpha_beta(board, info, -alpha - 1, -alpha, depth - 1, ply + 1);
            }

            // PVS re-search: if null window search failed high, re-search with full window
            if (score > alpha && score < beta) {
                score = -alpha_beta(board, info, -beta, -alpha, depth - 1, ply + 1);
            }
        }

        board.unmake_move(m);

        if (info.stopped) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = m;

            if (score > alpha) {
                alpha = score;
                tt_flag = TT_EXACT;

                // Update PV
                pv_table_[ply][ply] = m;
                for (int j = ply + 1; j < pv_length_[ply + 1]; ++j) {
                    pv_table_[ply][j] = pv_table_[ply + 1][j];
                }
                pv_length_[ply] = pv_length_[ply + 1];

                // History heuristic update (quiet moves only)
                if (captured == NO_PIECE && !m.is_en_passant()) {
                    Color us = board.side_to_move();
                    // Simple additive update with saturation
                    int bonus = depth * depth;
                    if (history_[us][m.from()][m.to()] < 1000000)
                        history_[us][m.from()][m.to()] += bonus;
                }
            }

            if (score >= beta) {
                // Beta cutoff
                store_tt(key, depth, beta, TT_BETA, m);

                // Killer/History update (quiet moves only)
                if (captured == NO_PIECE && !m.is_en_passant()) {
                    killers_[ply][1] = killers_[ply][0];
                    killers_[ply][0] = m;

                    Color us = board.side_to_move();
                    int bonus = depth * depth;
                    if (history_[us][m.from()][m.to()] < 1000000)
                        history_[us][m.from()][m.to()] += bonus;
                }

                return beta;
            }
        }
    }

    store_tt(key, depth, best_score, tt_flag, best_move);
    return best_score;
}

// ============================================================
// Iterative deepening
// ============================================================
Move Searcher::search(Board& board, SearchInfo& info) {
    info.start_time = std::chrono::steady_clock::now();
    info.nodes = 0;
    info.stopped = false;

    // Clear PV
    std::memset(pv_length_, 0, sizeof(pv_length_));

    Move best_move{};
    int max_depth = info.max_depth;
    if (max_depth <= 0) max_depth = 64;

    best_move_changes_ = 0;
    last_best_move_ = MOVE_NONE;

    // Increment TT age for the new search
    tt_age_++;

    for (int depth = 1; depth <= max_depth; ++depth) {
        int score;
        int alpha = -VALUE_INFINITE;
        int beta = VALUE_INFINITE;
        int delta = 16;

        // Aspiration windows: use a narrow window if we have a previous score
        if (depth >= 5) {
            alpha = std::max(-VALUE_INFINITE, info.last_score - delta);
            beta = std::min(VALUE_INFINITE, info.last_score + delta);
        }

        while (true) {
            score = alpha_beta(board, info, alpha, beta, depth, 0);

            if (info.stopped) break;

            if (score <= alpha) {
                // Fail low: widen alpha
                alpha = std::max(-VALUE_INFINITE, alpha - delta);
                beta = (alpha + beta) / 2; // Bring beta closer to avoid useless search
                delta += delta / 2;
            } else if (score >= beta) {
                // Fail high: widen beta
                beta = std::min(VALUE_INFINITE, beta + delta);
                delta += delta / 2;
            } else {
                // Score within bounds
                break;
            }
            
            // Safety: if window gets too wide, just do a full search
            if (delta > 1000) {
                alpha = -VALUE_INFINITE;
                beta = VALUE_INFINITE;
            }
        }

        if (info.stopped) break;

        info.last_score = score;
        Move current_best = pv_table_[0][0];

        if (depth > 1 && current_best != last_best_move_) {
            best_move_changes_++;
        }
        last_best_move_ = current_best;
        best_move = current_best;

        // Print UCI info
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - info.start_time).count();
        int64_t nps = (elapsed > 0) ? (static_cast<int64_t>(info.nodes) * 1000 / elapsed) : 0;

        std::cout << "info depth " << depth
                  << " score cp " << score
                  << " nodes " << info.nodes
                  << " nps " << nps
                  << " time " << elapsed
                  << " pv";

        for (int i = 0; i < pv_length_[0]; ++i) {
            std::cout << " " << pv_table_[0][i].to_uci();
        }
        std::cout << std::endl;

        // Adaptive time management
        if (info.time_limit_ms > 0 && !info.infinite) {
            // Soft limit: stop if we've used a significant portion of our time
            // and the best move has been stable for long enough.
            double stability_factor = (best_move_changes_ > 0) ? 1.5 : 0.8;
            int64_t optimum_time = info.time_limit_ms / 3;
            
            if (elapsed > optimum_time * stability_factor && depth >= 6) {
                break;
            }
        }

        // Mate found — no need to search deeper
        if (score >= MATE_IN_MAX_PLY || score <= MATED_IN_MAX_PLY)
            break;
    }

    return best_move;
}

} // namespace chess
