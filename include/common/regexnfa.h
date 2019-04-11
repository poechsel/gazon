#pragma once
#include <algorithm>
#include <common/regexpostfix.h>
#include <map>

template <typename T>
class RegexNfa {
public:
    RegexNfa() {
        // TODO: do something to enlarge the buffer if needed, or just raise an exception
    }

    enum NodeType {
                      Any = 1,
                      Split = 2,
                      Match = 3
    };

    T getNStates() {
        return m_state_top_ptr;
    }

    void from(RegexPostfix const &postfix) {
        /* See https://swtch.com/~rsc/regexp/regexp1.html for details */
        std::stack<Fragment> fragments;

        for (char c : postfix.get()) {
            switch(c) {
            case (char)RegexPostfix::SpecialChar::Any: {
                T s = allocateState(NodeType::Any);
                fragments.push({s, {&m_states[s].left}});
                break;
            }
            case (char)RegexPostfix::SpecialChar::Concat: {
                auto e2 = popFragment(fragments);
                auto e1 = popFragment(fragments);
                link(e1.needs_linking, e2.state);
                fragments.push({e1.state, e2.needs_linking});
                break;
            }
            case (char)RegexPostfix::SpecialChar::Pipe: {
                auto e2 = popFragment(fragments);
                auto e1 = popFragment(fragments);
                T s = allocateState(NodeType::Split, e1.state, e2.state);
                e1.needs_linking.insert(e1.needs_linking.end(),
                                        e2.needs_linking.begin(), e2.needs_linking.end());
                fragments.push({s, e1.needs_linking});
                break;
            }
            case (char)RegexPostfix::SpecialChar::ZeroOrOne: {
                auto e = popFragment(fragments);
                T s = allocateState(NodeType::Split, e.state);
                e.needs_linking.push_back(&m_states[s].right);
                fragments.push({s, e.needs_linking});
                break;
            }
            case (char)RegexPostfix::SpecialChar::ZeroOrMore: {
                auto e = popFragment(fragments);
                T s = allocateState(NodeType::Split, e.state);
                link(e.needs_linking, s);
                fragments.push({s, {&m_states[s].right}});
                break;
            }
            case (char)RegexPostfix::SpecialChar::OneOrMore: {
                auto e = popFragment(fragments);
                T s = allocateState(NodeType::Split, e.state);
                link(e.needs_linking, s);
                fragments.push({e.state, {&m_states[s].right}});
                break;
            }
            default: {
                T s = allocateState(c);
                fragments.push({s, {&m_states[s].left}});
                break;
            }
            }
        }
        m_final_state = allocateState(NodeType::Match);
        auto e = popFragment(fragments);
        m_start_state = e.state;
        link(e.needs_linking, m_final_state);
        if (!fragments.empty())
            throw RegexException("[Nfa] Invalid postfix expression");
    }

    std::vector<T> getStartStates() {
        m_current_step_uid++;
        std::vector<T> current_states;
        addState(current_states, m_start_state);
        return current_states;
    }

    bool match(std::string const s) {
        std::vector<T> current_states = getStartStates();
        bool isMatchState = false;
        for (auto c : s) {
            isMatchState = false;
            current_states = step(current_states, c, isMatchState);
        }
        return isMatchState;
    }

    bool asMatch(std::string const s) {
        std::vector<T> current_states = getStartStates();
        bool isMatchState;
        for (auto c : s) {
            isMatchState = false;
            current_states = step(current_states, c, isMatchState);
            if (isMatchState)
                return true;
        }
        return false;
    }

    std::vector<T> step(std::vector<T> const& states, char c, bool &isMatch) {
        m_current_step_uid++;
        std::vector<T> out;
        for (T state_id : states) {
            State &state = m_states[state_id];
            std::string f;
            f += state.c;
            if (state.c == NodeType::Any || state.c == c) {
                isMatch |= addState(out, state.left);
            }
        }
        return out;
    }


private:
    struct State {
        char c;
        T left;
        T right;
        int last_step_seen;
    };

    // We don't mind overflowing this bit
    int m_current_step_uid = 0;

    static constexpr unsigned int max_size = std::numeric_limits<T>::max();
    State m_states[max_size];
    // 0 is for the "null node"
    T m_state_top_ptr = 1;
    T m_final_state = m_state_top_ptr;
    T m_start_state = m_state_top_ptr;

    struct Fragment {
        T state;
        std::vector<T*> needs_linking;

        Fragment(T s, std::vector<T*> n):
            state(s), needs_linking(n)
        {}
    };

    void debug() {
        std::cout<<"[start at]: "<<m_start_state<<"\n";
        std::cout<<"[final at]: "<<m_final_state<<"\n";
        for (int i = 1; i < m_state_top_ptr; ++i) {
            std::cout<<i<<":  ";
            if (m_states[i].c == NodeType::Any) {
                std::cout<<"any";
            } else if (m_states[i].c == NodeType::Split) {
                std::cout<<"split";
            } else if (m_states[i].c == NodeType::Match) {
                std::cout<<"match";
            } else {
                std::cout<<m_states[i].c;
            }

            std::cout<<"  left: "<<m_states[i].left;
            if (m_states[i].right)
            std::cout<<"  right: "<<m_states[i].right;
            std::cout<<"\n";
        }
    }


    bool containsMatchState(std::vector<T> &states) {
        return std::find_if(states.begin(),
                            states.end(),
                            [this](const T x) {
                                return m_states[x].c == NodeType::Match;
                            }) != states.end();
    }

    void link(std::vector<T*> &needs_linking, T target) {
        for (T* x : needs_linking) {
            *x = target;
        }
    }

    bool addState(std::vector<T> &states, int state_id) {
        if (state_id == 0)
            return false;
        State &state = m_states[state_id];
        // If we have already seen this step, abort
        if (state.last_step_seen == m_current_step_uid)
            return state.c == NodeType::Match;

        state.last_step_seen = m_current_step_uid;
        bool is_match = false;
        if (state.c == NodeType::Split) {
            is_match |= addState(states, state.left);
            is_match |= addState(states, state.right);
        }
        states.push_back(state_id);
        return is_match || state.c == NodeType::Match;
    }



    Fragment popFragment(std::stack<Fragment> &s) {
        if (s.empty())
            throw RegexException("[Nfa] Invalid postfix expression");
        Fragment f = s.top();
        s.pop();
        return f;
    }

    inline T allocateState(char c, T left, T right) {
        m_states[m_state_top_ptr].c = c;
        m_states[m_state_top_ptr].left = left;
        m_states[m_state_top_ptr].right = right;
        m_states[m_state_top_ptr].last_step_seen = 0;
        return m_state_top_ptr++;
    }
    inline T allocateState(char c, T left) {
        return allocateState(c, left, 0);
    }
    inline T allocateState(char c) {
        return allocateState(c, 0, 0);
    }
};
