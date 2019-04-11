#pragma once
#include <algorithm>
#include <common/regexnfa.h>
#include <map>

template <typename T>
class RegexDfa {
public:
    RegexDfa() {}
    ~RegexDfa() {
        emptyCache(nullptr);
    }

    void linkNfa(RegexNfa<T> *nfa) {
        m_nfa = nfa;
    }

    bool match(std::string const s) {
        DfaState* current_state = getDfaStart();
        bool isMatchState = false;
        for (unsigned char c : s) {
            isMatchState = false;
            if (current_state->next[c] == nullptr) {
                const auto &next_steps = m_nfa->step(current_state->states, c, isMatchState);
                current_state->next[c] = getDfaState(next_steps, current_state);
                current_state->next[c]->isMatch = isMatchState;
            }
            current_state = current_state->next[c];
        }
        return current_state->isMatch;
    }

    bool asMatch(std::string const s) {
        DfaState* current_state = getDfaStart();
        bool isMatchState = false;
        for (unsigned char c : s) {
            isMatchState = false;
            if (current_state->next[c] == nullptr) {
                const auto &next_steps = m_nfa->step(current_state->states, c, isMatchState);
                current_state->next[c] = getDfaState(next_steps, current_state);
                current_state->next[c]->isMatch = isMatchState;
            }
            current_state = current_state->next[c];
            if (current_state->isMatch) return true;
        }
        return false;
    }

private:
    RegexNfa<T> *m_nfa;

    struct DfaState {
        std::vector<T> states;
        DfaState* next[256];
        bool isMatch;
    };

    void emptyCache(DfaState *do_not_delete) {
        std::vector<T> backup;
        for (auto it : m_dfa_cache) {
            if (it.second != do_not_delete) {
                delete it.second;
            } else {
                backup = it.first;
            }
        }
        m_dfa_cache.clear();
        if (backup.size() > 0) {
            m_dfa_cache[backup] = do_not_delete;
            for (unsigned int i = 0; i < 256; ++i) {
                do_not_delete->next[i] = nullptr;
            }
        }
        m_dfa_start_state = nullptr;
    }

    std::map<std::vector<T>, DfaState*> m_dfa_cache;

    DfaState *m_dfa_start_state = nullptr;

    DfaState* getDfaState(const std::vector<T> &states, DfaState* do_not_delete) {
        auto it = m_dfa_cache.find(states);
        if (it != m_dfa_cache.end()) {
            return it->second;
        } else {
            if (m_dfa_cache.size() >= 256) {
                emptyCache(do_not_delete);
            }
            DfaState *dfa = new DfaState;
            dfa->states = states;
            for (unsigned int i = 0; i < 256; ++i)
                dfa->next[i] = nullptr;
            dfa->isMatch = false;
            m_dfa_cache[states] = dfa;
            return dfa;
        }
    }

    DfaState *getDfaStart() {
        if (m_dfa_start_state == nullptr) {
            m_dfa_start_state = getDfaState(m_nfa->getStartStates(), nullptr);
        }
        return m_dfa_start_state;
    }
};
