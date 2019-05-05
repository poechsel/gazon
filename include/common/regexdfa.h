#pragma once
#include <algorithm>
#include <common/regexnfa.h>
#include <map>

/* Overlay of an nfa.
   Lazily builds a dfa over the nfa. To avoid
   an explosion of the space required use a fixed size
   cache (here 256 nodes).

   A dfa automata is deterministic. Their is only one starting
   states and from each states and transitions at most one next
   state

   Inspired by https://swtch.com/~rsc/regexp/regexp1.html
*/
class RegexDfa {
public:
    RegexDfa() {}
    ~RegexDfa() {
        flushCache(nullptr);
    }

    /* Attach an nfa to this dfa */
    void linkNfa(RegexNfa *nfa) {
        m_nfa = nfa;
    }

    /* Simulate a nfa: start by the list of start states and
       build the next state of states reachable. Stops when
       we do not have anymore characters in the input string and
       the list of current states contains a matching nfa state */
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
private:
    RegexNfa *m_nfa;

    struct DfaState {
        RegexNfa::statelist states;
        DfaState* next[256];
        bool isMatch;
    };

    /* Flush the cache -- remove every element of the cache
       but one */
    void flushCache(DfaState *do_not_delete) {
        RegexNfa::statelist backup;
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

    /* get the dfa state corresponding to a list of nfa states
       [states]: list of nfa states
       [do_not_delete]: pointer to a dfa state we do not want to delete
           (we don't want to delete a node we are currently using)

       It is also managing the cache, flushing it when it is full
    */
    DfaState* getDfaState(const RegexNfa::statelist &states, DfaState* do_not_delete) {
        auto it = m_dfa_cache.find(states);
        if (it != m_dfa_cache.end()) {
            return it->second;
        } else {
            if (m_dfa_cache.size() >= 256) {
                flushCache(do_not_delete);
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

    /* Get the start state of our dfa */
    DfaState *getDfaStart() {
        if (m_dfa_start_state == nullptr) {
            m_dfa_start_state = getDfaState(m_nfa->getStartStates(), nullptr);
        }
        return m_dfa_start_state;
    }

    std::map<RegexNfa::statelist, DfaState*> m_dfa_cache;
    DfaState *m_dfa_start_state = nullptr;
};
