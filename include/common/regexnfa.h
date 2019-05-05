#pragma once
#include <algorithm>
#include <common/regexpostfix.h>
#include <map>


/* Nfa automata, inspired by
   https://swtch.com/~rsc/regexp/regexp1.html

   An nfa automata has several states. There can be
   several start states, several end states, and for a given
   transition several next states

   Uses epsilon transitions (transition corresponding to no character,
   they mean nothing and can be understood as "teleportation gates")
*/

class RegexNfa {
public:
    /* We are allocating our own memory space. Each
       state is represented by a 16 bit integer which
       is a pointer in this memory space */
    typedef uint16_t stateptr;
    /* A list of states */
    typedef std::vector<stateptr> statelist;

    RegexNfa() {
    }

    /* Generate a nfa from a postfix expression
       It proceeds incrementally: for each operator corresponds
       a 'fragment'. Fragment are then reunited to form the nfa.
     */
    void from(RegexPostfix const &postfix) {
        std::stack<Fragment> fragments;

        // We iterate through our postfix expression
        for (char c : postfix.get()) {
            switch(c) {
            case (char)RegexPostfix::SpecialChar::Any: {
                /* If the current postfix expression is *, then we a new fragment
                   which is (state) ----> ?
                */
                stateptr s = allocateState(NodeType::Any);
                fragments.push({s, {&m_states[s].left}});
                break;
            }
            case (char)RegexPostfix::SpecialChar::Concat: {
                /* If the current postfix expression is '.', we pop
                   the (a)--->? and (b)--->? from the fragment stack.
                   We concat both of them to form a new fragment (a)--->(b)--->
                */
                auto e2 = popFragment(fragments);
                auto e1 = popFragment(fragments);
                link(e1.needs_linking, e2.state);
                fragments.push({e1.state, e2.needs_linking});
                break;
            }
            case (char)RegexPostfix::SpecialChar::Pipe: {
                /* If the current postfix expression is '.', we pop
                   the (a)--->? and (b)--->? from the fragment stack.
                   We create a new state (c) and we build the fragment
                   (c)___/----(a)--->?
                         \____(b)---->?
                */
                auto e2 = popFragment(fragments);
                auto e1 = popFragment(fragments);
                stateptr s = allocateState(NodeType::Split, e1.state, e2.state);
                e1.needs_linking.insert(e1.needs_linking.end(),
                                        e2.needs_linking.begin(), e2.needs_linking.end());
                fragments.push({s, e1.needs_linking});
                break;
            }
            case (char)RegexPostfix::SpecialChar::ZeroOrOne: {
                /* If the current postfix expression is '.', we pop
                   the (a)--->? from the fragment stack.
                   We create a new state (s) and we build the fragment
                   (s)------>?
                     \_(a)-->?
                */
                auto e = popFragment(fragments);
                stateptr s = allocateState(NodeType::Split, e.state);
                e.needs_linking.push_back(&m_states[s].right);
                fragments.push({s, e.needs_linking});
                break;
            }
            case (char)RegexPostfix::SpecialChar::ZeroOrMore: {
                /* If the current postfix expression is '.', we pop
                   the (a)--->? from the fragment stack.
                   We create a new state (s) and we build the fragment

                   >--------(s)-->?
                     ^      /
                      \_(a)/
                */
                auto e = popFragment(fragments);
                stateptr s = allocateState(NodeType::Split, e.state);
                link(e.needs_linking, s);
                fragments.push({s, {&m_states[s].right}});
                break;
            }
            case (char)RegexPostfix::SpecialChar::OneOrMore: {
                /* If the current postfix expression is '.', we pop
                   the (a)--->? from the fragment stack.
                   We create a new state (s) and we build the fragment

                   >--(a)-----(s)-->?
                       ^      /
                        \____/
                */
                auto e = popFragment(fragments);
                stateptr s = allocateState(NodeType::Split, e.state);
                link(e.needs_linking, s);
                fragments.push({e.state, {&m_states[s].right}});
                break;
            }
            default: {
                /* If we got a character c we add a new fragment
                   (c)--->?
                */
                stateptr s = allocateState(c);
                fragments.push({s, {&m_states[s].left}});
                break;
            }
            }
        }
        // Finally we allocate the final state and link the
        // Remaining free fragments to it
        // Node that we are building the regex in such a way that we have
        // Only one final state.
        m_final_state = allocateState(NodeType::Match);
        auto e = popFragment(fragments);
        m_start_state = e.state;
        link(e.needs_linking, m_final_state);
        // If all fragments weren't consummated, it means that something wrong
        // happened. The regex must be invalid
        if (!fragments.empty())
            throw RegexException("[Nfa] Invalid postfix expression");
    }

    statelist getStartStates() {
        m_current_step_uid++;
        statelist current_states;
        addStateToStateList(current_states, m_start_state);
        return current_states;
    }

    /* Match a nfa on a string. Returns true
       if s is matched by the regex */
    bool match(std::string const s) {
        statelist current_states = getStartStates();
        bool isMatchState = false;
        for (auto c : s) {
            isMatchState = false;
            current_states = step(current_states, c, isMatchState);
        }
        return isMatchState;
    }

    /* Given the list of states we are at and the char we are seeing
       returns the next list of states. isMatched will be true if
       the next list of states includes a match state */
    statelist step(statelist const& states, char c, bool &isMatch) {
        m_current_step_uid++;
        statelist out;
        for (stateptr state_id : states) {
            State &state = m_states[state_id];
            std::string f;
            f += state.c;
            if (state.c == NodeType::Any || state.c == c) {
                isMatch |= addStateToStateList(out, state.left);
            }
        }
        return out;
    }


private:

    struct State {
        char c; /* Char matched by this state
                   If c=1, then it matches any character
                   If c=2, both left and right are valid transitions
                           and this states is an epsilon transition
                   If c=3 this state is a final state */

        stateptr left;
        stateptr right;
        int last_step_seen; // field used to cache information during traversal
    };
    // Clean way to refer to the constant 1, 2 and 3.
    enum NodeType {
        Any = 1,
        Split = 2,
        Match = 3
    };


    // We don't mind overflowing this bit
    int m_current_step_uid = 0;

    static constexpr unsigned int max_size = std::numeric_limits<stateptr>::max();

    /* We keep every states in the buffer m_states. This allows us to
       avoid having to deal with memory fragmentation and pointers */
    State m_states[max_size];

    // state 0 is reserved: the first valid state is 1
    stateptr m_state_top_ptr = 1;
    stateptr m_final_state = m_state_top_ptr;
    stateptr m_start_state = m_state_top_ptr;

    /* Returns true if a list of states contains one matching state */
    bool containsMatchState(statelist &states) {
        return std::find_if(states.begin(),
                            states.end(),
                            [this](const stateptr x) {
                                return m_states[x].c == NodeType::Match;
                            }) != states.end();
    }

    /* Add a state to a list of states.
       Skip epsilon transition states and make sure not to add
       the same state twice */
    bool addStateToStateList(statelist &states, stateptr state_id) {
        if (state_id == 0)
            return false;
        State &state = m_states[state_id];
        // If we have already seen this state during the current state, abort
        if (state.last_step_seen == m_current_step_uid)
            return state.c == NodeType::Match;

        state.last_step_seen = m_current_step_uid;
        bool is_match = false;
        if (state.c == NodeType::Split) {
            is_match |= addStateToStateList(states, state.left);
            is_match |= addStateToStateList(states, state.right);
        }
        states.push_back(state_id);
        return is_match || state.c == NodeType::Match;
    }


    inline stateptr allocateState(char c, stateptr left, stateptr right) {
        m_states[m_state_top_ptr].c = c;
        m_states[m_state_top_ptr].left = left;
        m_states[m_state_top_ptr].right = right;
        m_states[m_state_top_ptr].last_step_seen = 0;
        return m_state_top_ptr++;
    }
    inline stateptr allocateState(char c, stateptr left) {
        return allocateState(c, left, 0);
    }
    inline stateptr allocateState(char c) {
        return allocateState(c, 0, 0);
    }

    /* A fragment is nearly a nfa. It is represented by an end node
       and a list of states that still needs to be defined.
       We say that these undefined states needs to be "linked"
    */
    struct Fragment {
        stateptr state;
        std::vector<stateptr*> needs_linking;

        Fragment(stateptr s, std::vector<stateptr*> n):
            state(s), needs_linking(n)
        {}
    };

    /* Pop a fragment from fragment stack safely.
       Raise an error is something invalid occurs */
    Fragment popFragment(std::stack<Fragment> &s) {
        if (s.empty())
            throw RegexException("[Nfa] Invalid postfix expression");
        Fragment f = s.top();
        s.pop();
        return f;
    }

    /* Link every state to a given target */
    void link(std::vector<stateptr*> &needs_linking, stateptr target) {
        for (stateptr* x : needs_linking) {
            *x = target;
        }
    }


};
