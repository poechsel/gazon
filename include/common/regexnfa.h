#pragma once
#include <algorithm>
#include <common/regexpostfix.h>

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

    bool match(std::string const s) {
        m_current_step_uid++;
        std::vector<T> current_states;
        addState(current_states, m_start_state);
        for (auto c : s) {
            current_states = step(current_states, c);
            m_current_step_uid++;
        }
        return containsMatchState(current_states);
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

    void addState(std::vector<T> &states, int state_id) {
        if (state_id == 0)
            return;
        State &state = m_states[state_id];
        // If we have already seen this step, abort
        if (state.last_step_seen == m_current_step_uid)
            return;
        state.last_step_seen = m_current_step_uid;
        if (state.c == NodeType::Split) {
            addState(states, state.left);
            addState(states, state.right);
        }
        states.push_back(state_id);
    }


    std::vector<T> step(std::vector<T> const& states, char c) {
        std::vector<T> out;
        for (T state_id : states) {
            State &state = m_states[state_id];
            if (state.c == NodeType::Any || state.c == c) {
                addState(out, state.left);
            }
        }
        return out;
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
