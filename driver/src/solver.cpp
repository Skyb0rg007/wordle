#include <algorithm>
#include <queue>
#include <cassert>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <wordle.hpp>
#include <resources.hpp>

using namespace wordle;

struct hash_pair {
    size_t operator()(const std::pair<State, Word>& p) const noexcept {
        auto h1 = std::hash<State>{}(p.first);
        auto h2 = std::hash<Word>{}(p.second);
        return h1 ^ h2;
    }
};

static std::unordered_map<State, int> player_cache;
static std::unordered_map<std::pair<State, Word>, int, hash_pair> server_cache;

void save_caches(std::ostream& out) {
    size_t size = player_cache.size();
    out.write(reinterpret_cast<char*>(&size), sizeof size);
    for (auto [state, rank] : player_cache) {
        state.serialize(out);
        out.write(reinterpret_cast<char*>(&rank), sizeof rank);
    }

    size = server_cache.size();
    out.write(reinterpret_cast<char*>(&size), sizeof size);
    for (auto [p, rank] : server_cache) {
        auto [state, word] = p;
        state.serialize(out);
        word.serialize(out);
        out.write(reinterpret_cast<char*>(&rank), sizeof rank);
    }
}

void load_caches(std::istream& in) {
    size_t size;
    in.read(reinterpret_cast<char*>(&size), sizeof size);
    for (size_t i = 0; i < size; i++) {
        auto state = State::deserialize(in);
        int rank;
        in.read(reinterpret_cast<char*>(&rank), sizeof rank);
        player_cache.insert(std::pair(state, rank));
    }

    in.read(reinterpret_cast<char*>(&size), sizeof size);
    for (size_t i = 0; i < size; i++) {
        auto state = State::deserialize(in);
        auto word = Word::deserialize(in);
        int rank;
        in.read(reinterpret_cast<char*>(&rank), sizeof rank);
        server_cache.insert(std::pair(std::pair(state, word), rank));
    }
}

static int player_rank(const State& s);
static int server_rank(const State& s, const Word& w);

static std::queue<State> player_work_queue;
static std::queue<std::pair<State, Word>> server_work_queue;

// Returns the rank of the best word (minimizes rank)
// Returns -1 if there aren't any valid words
// Returns -2 if we need to wait on a server decision (server_work_queue updated)
static int player_decide(const State& s) {
    int best_rank = std::numeric_limits<int>::max();

    bool ok = true;
    for (auto w : wordlist) {
        auto it = server_cache.find(std::pair(s, w));
        if (it == server_cache.end()) {
            ok = false;
            server_work_queue.push(std::pair(s, w));
        } else if (ok) {
            int rank = (*it).second;
            if (rank != -1 && rank < best_rank) {
                best_rank = rank;
            }
        }
    }

    if (!ok) {
        return -2;
    } else if (best_rank == std::numeric_limits<int>::max()) {
        return -1;
    } else {
        return best_rank;
    }
}

// Returns the rank of the best response (maximizes rank)
// Returns -1 if there aren't any valid responses
// Returns -2 if we need to wait on a player decision (player_work_queue updated)
static int server_decide(const State& s, const Word& w) {
    // Check if the final word is already decided
    auto final = s.final();
    if (final.has_value() && final.value() == w) {
        return 0;
    }

    int best_rank = std::numeric_limits<int>::min();

    bool ok = true;
    Response r{};
    do {
        State state = s;
        if (!state.update(w, r))
            continue;
        auto it = player_cache.find(s);
        if (it == player_cache.end()) {
            ok = false;
            player_work_queue.push(s);
        } else if (ok) {
            int rank = (*it).second;
            if (rank != -1 && rank > best_rank) {
                best_rank = rank;
            }
        }
    } while (r.next_combination());

    if (!ok) {
        return -2;
    } else if (best_rank == std::numeric_limits<int>::min()) {
        return -1;
    } else {
        return best_rank + 1;
    }
}

void run() {
    int rank = player_decide(State{});
    std::clog << "Returned " << rank << "\n";
    if (rank != -2) {
        return;
    }
    while (!server_work_queue.empty() || !player_work_queue.empty()) {
        std::clog << "Server Done: " << server_cache.size() << "\n";
        std::clog << "Player Done: " << player_cache.size() << "\n";
        std::clog << "Server: " << server_work_queue.size() << "\n";
        std::clog << "Player: " << player_work_queue.size() << "\n";
        // Advance server queue as much as possble
        {
            std::unordered_set<std::pair<State, Word>, hash_pair> still_waiting;
            for (; !server_work_queue.empty(); server_work_queue.pop()) {
                auto p = server_work_queue.front();
                if (!server_cache.contains(p) && !still_waiting.contains(p)) {
                    int ret = server_decide(p.first, p.second);
                    if (ret != -2) {
                        server_cache.insert(std::pair(p, ret));
                    } else {
                        still_waiting.insert(p);
                    }
                }
            }

            for (auto p : still_waiting) {
                server_work_queue.push(p);
            }
        }

        // Advance player queue as much as possible
        {
            std::unordered_set<State> still_waiting;
            for (; !player_work_queue.empty(); player_work_queue.pop()) {
                auto s = player_work_queue.front();
                if (!player_cache.contains(s) && !still_waiting.contains(s)) {
                    int ret = player_decide(s);
                    if (ret != -2) {
                        player_cache.insert(std::pair(s, ret));
                    } else {
                        still_waiting.insert(s);
                    }
                }
            }

            for (auto p : still_waiting) {
                player_work_queue.push(p);
            }
        }       

        // Save the work
        std::ofstream out("log.bin", std::ios_base::binary | std::ios_base::trunc);
        save_caches(out);
    }
    std::clog << "Queues are empty\n";
}

// Memoization

/* static int player_rank(const State& s) { */
/*     auto it = player_cache.find(s); */
/*     if (it == player_cache.end()) { */
/*         it = player_cache.insert(std::pair(s, player_decide(s))).first; */
/*         assert(it != player_cache.end()); */
/*     } */
/*     return (*it).second; */
/* } */

/* static int server_rank(const State& s, const Word& w) { */
/*     auto p = std::pair(s, w); */
/*     auto it = server_cache.find(p); */
/*     if (it == server_cache.end()) { */
/*         it = server_cache.insert(std::pair(p, server_decide(s, w))).first; */
/*         assert(it != server_cache.end()); */
/*     } */
/*     return (*it).second; */
/* } */
