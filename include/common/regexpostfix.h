#pragma once
#include <common/regexcommon.h>

/* Postfix representation of a regex */
class RegexPostfix {
public:
    /* Operators are put on non printable characters
       to save space */
    enum SpecialChar {
                      Concat = 1,
                      Pipe = 2,
                      ZeroOrOne = 3,
                      OneOrMore = 4,
                      ZeroOrMore = 5,
                      Any = 6
    };

    RegexPostfix(std::string const &infix): m_infix(infix) {
        convert();
    }

    std::string get() const {
        return m_postfix;
    }

    /* Similar to a shunting yard algorithm, but inlined as we only have one
       binary operator ('|') */
    void convert() {
        m_stack = std::stack<StackElem>();
        m_current_scope = {0, 0, -1};
        m_postfix = "";
        for (unsigned int i = 0; i < m_infix.size(); ++i) {
            if (m_infix[i] == '(') {
                /* When seeing an open paranthesis we are starting a new group:
                   We create a new scope and push the old one on the stack */
                popOneConcat();
                m_current_scope.index_start_group = m_postfix.size();
                m_stack.push(m_current_scope);
                m_current_scope = {0, 0, -1};
            } else if (!m_stack.empty() && m_infix[i] == ')') {
                /* For ')' we have to finish converting the remaining concatenations
                   and pipes to postfix, and then we retrieve the previous scope
                   from the frame */
                popAllConcat();
                popAllPipe();
                m_current_scope = m_stack.top();
                m_current_scope.n_concat++;
                m_stack.pop();
            } else if (m_infix[i] == '|') {
                if (m_current_scope.n_concat == 0)
                    throw RegexException("[Syntax error] '|' operator expression expects tokens on the left.");
                /* '|' has precedence over concatenation */
                popAllConcat();
                m_current_scope.n_pipe++;
            } else if (m_infix[i] == '\\' && i < m_infix.size()) {
                /* Escaped characters */
                popOneConcat();
                // The next character is escaped: we push it directly on the stack
                m_postfix += m_infix[++i];
            } else if (m_infix[i] == '{') {
                /* Parse and convert repetitions */
                int repetitions_min, repetitions_max;
                std::tie(i, repetitions_min, repetitions_max) = parseRepetitions(i+1);
                if (m_current_scope.index_start_group < 0) {
                        throw RegexException("[Syntax error] A {...} expression expects tokens on the left.");
                } else {
                    int pattern_size = m_postfix.size() - m_current_scope.index_start_group + 1;
                    std::string pattern = m_postfix.substr(m_current_scope.index_start_group, pattern_size);
                    translateRepetitions(pattern, repetitions_min, repetitions_max);
                }
            } else if (isUnary(m_infix[i])) {
                dealWithUnary(m_infix[i]);
            } else {
                // "." is consider as a char so it will be treated here
                popOneConcat();
                m_current_scope.n_concat++;
                m_current_scope.index_start_group = m_postfix.size();
                m_postfix += toChar(m_infix[i]);
            }
        }
        popAllConcat();
        popAllPipe();
    }

private:
    std::string m_infix;
    std::string m_postfix;


    /* Our "inlined shutting yard" uses a stack
       Each element of this stack needs to remember 3 elements:
       - [index_start_group], the location in our input string where
         our current group (something between paranthesis) started
       - [n_pipe] Number of '|' seen but not yet present in the postfix expression
       - [n_concat] Number of concatenation (for exemple ab) seen but not yet present
         in the postfix expression
    */
    struct StackElem {
        int n_concat;
        int n_pipe;
        int index_start_group;
    };
    std::stack<StackElem> m_stack;
    /* A stack element to save the current information of our scope
       The other scope are saved on the stack */
    StackElem m_current_scope;

    /* Given a character convert it in the compressed representation
       of characters (see SpecialChar) */
    char toChar(char c) {
        switch (c) {
        case '*':
            return (char)SpecialChar::ZeroOrMore;
        case '?':
            return (char)SpecialChar::ZeroOrOne;
        case '+':
            return (char)SpecialChar::OneOrMore;
        case '|':
            return (char)SpecialChar::Pipe;
        case '$':
            return (char)SpecialChar::Concat;
        case '.':
            return (char)SpecialChar::Any;
        default:
            // For non operator no compression is needed
            return c;
        }
    }

    /* Add a concat operator to postfix expression
       if it is allowed */
    void popOneConcat() {
        if (m_current_scope.n_concat > 1) {
            m_current_scope.n_concat--;
            m_postfix += toChar('$');
        }
    }

    /* Add all remaining concat operators of our stack
       on the postfix expression */
    void popAllConcat() {
        while (m_current_scope.n_concat-- > 1)
            m_postfix += toChar('$');
    }

    void popAllPipe() {
        while (m_current_scope.n_pipe-- > 0)
            m_postfix += toChar('|');
    }

    /* So when n_concat is 0 it means that we have no
       expression (such as a letter) left to be concattenated
       (for exemple for 'ab' -> n_concat = 2, for 'a' -> n_concat = 1).
       We raise an error */
    void dealWithUnary(char unary) {
        if (m_current_scope.n_concat == 0) {
            throw RegexException("[Syntax error] Regex can't have a unary op here.");
        }
        m_postfix += toChar(unary);
    }

    /* Is an operator a unary operator (one argument operator)? */
    bool isUnary(char x) {
        return (x == '?') || (x == '*') || (x == '+');
    }

    /* Parse a {n, m} pattern starting at index i in
       m_infix and return:
       - the index of the next character of m_infix
       - n
       - m
    */
    std::tuple<int, int, int> parseRepetitions(int i) {
        int repetitions_min = 0;
        int repetitions_max = std::numeric_limits<int>::max();
        bool saw_one_coma = false;

        unsigned int i_next_token = i;

        while (i_next_token < m_infix.size()) {
            i_next_token = skipEmptyChars(i_next_token, m_infix);
            char c = m_infix[i_next_token];
            if (c == '}') {
                if (!saw_one_coma)
                    repetitions_max = repetitions_min;

                if (repetitions_min < 0 || repetitions_min > repetitions_max)
                    throw RegexException("[Syntax error] An {n, m} expression must have 0 <= n <= m");
                // we do not skip the next token right now. We want to return the
                // index of the '}'
                return std::make_tuple(i_next_token, repetitions_min, repetitions_max);
            } else if (c == ',' && !saw_one_coma) {
                // switch to the next entry (if we already parsed n we are gonna parsed
                // m now)
                saw_one_coma = true;
                i_next_token++;
            } else {
                /* Parse everything until the next coma or close bracket
                   Try to convert it in a number */
                int i_end_number = skipUntil(i_next_token, m_infix, ",}");
                std::string potential_number =
                    m_infix.substr(i_next_token, i_end_number - i_next_token);
                try {
                    int number = std::stoi(potential_number);
                    if (!saw_one_coma)
                        repetitions_min = number;
                    else
                        repetitions_max = number;
                    i_next_token = i_end_number;
                } catch (std::exception const &e) {
                    break;
                }
            }
        }
        // If we reached this point that need that we enver saw a '}', thus
        // this repetition pattern is invalid
        throw RegexException("[Syntax error] Invalid syntax for a {...} expression");
        return std::make_tuple(-1, repetitions_min, repetitions_max);
    }

    void translateRepetitions(std::string pattern, int min, int max) {
        /* Convert something like pattern{min, max} to a regex without {...}
           Note that we assume pattern is already put in our postfix expression.

           We distinguish several cases:
           - min = 0, max = +oo => pattern{min, max} = pattern*
           - min = 0, _ => pattern{min, max} = pattern?pattern?. [max - 1 times] pattern ?.
           - _, max = +oo => pattern{min, max} ⁼ patternpattern.[min-2 times]pattern.pattern+.
           - _, _ => pattern{min, max} = patternpattern.[min-1 times]pattern.pattern?.[max-min times]pattern?.
           Do not forget to remember fat the pattern pattern is already present
           at once in postfix !
        */
        if (min == 0 && max == std::numeric_limits<int>::max()) {
            m_postfix += toChar('*');
        } else if (min == 0) {
            m_postfix += toChar('?');
            // Change int to unsigned int and i=1;i<max to i=0; i < max - 1 ?
            for (int i = 1; i < max; ++i) {
                m_postfix += pattern + toChar('?') + toChar('$');
            }
        } else if (max == std::numeric_limits<int>::max()) {
            for (int i = 2; i < min; ++i) {
                m_postfix += pattern + toChar('$');
            }
            m_postfix += pattern + toChar('+') + toChar('$');
        } else {
            for (int i = 1; i < min; ++i) {
                m_postfix += pattern + toChar('$');
            }
            for (int i = 0; i < max - min; ++i) {
                m_postfix += pattern + toChar('?') + toChar('$');
            }
        }
    }

};
