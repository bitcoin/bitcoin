// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>

#include <test/fuzz/fuzz.h>

class ConditionStack_d0e8f4d5d8ddaccb37f98b7989fb944081e41ab8
{
private:
    std::vector<bool> m_flags;

public:
    bool empty() { return m_flags.empty(); }
    bool all_true() { return !std::count(m_flags.begin(), m_flags.end(), false); }
    void push_back(bool f) { m_flags.push_back(f); }
    void pop_back() { m_flags.pop_back(); }
    void toggle_top() { m_flags.back() = !m_flags.back(); }
};


class ConditionStack_e6e622e5a0e22c2ac1b50b96af818e412d67ac54
{
private:
    //! A constant for m_first_false_pos to indicate there are no falses.
    static constexpr uint32_t NO_FALSE = std::numeric_limits<uint32_t>::max();

    //! The size of the implied stack.
    uint32_t m_stack_size = 0;
    //! The position of the first false value on the implied stack, or NO_FALSE if all true.
    uint32_t m_first_false_pos = NO_FALSE;

public:
    bool empty() { return m_stack_size == 0; }
    bool all_true() { return m_first_false_pos == NO_FALSE; }
    void push_back(bool f)
    {
        if (m_first_false_pos == NO_FALSE && !f) {
            // The stack consists of all true values, and a false is added.
            // The first false value will appear at the current size.
            m_first_false_pos = m_stack_size;
        }
        ++m_stack_size;
    }
    void pop_back()
    {
        assert(m_stack_size > 0);
        --m_stack_size;
        if (m_first_false_pos == m_stack_size) {
            // When popping off the first false value, everything becomes true.
            m_first_false_pos = NO_FALSE;
        }
    }
    void toggle_top()
    {
        assert(m_stack_size > 0);
        if (m_first_false_pos == NO_FALSE) {
            // The current stack is all true values; the first false will be the top.
            m_first_false_pos = m_stack_size - 1;
        } else if (m_first_false_pos == m_stack_size - 1) {
            // The top is the first false value; toggling it will make everything true.
            m_first_false_pos = NO_FALSE;
        } else {
            // There is a false value, but not on top. No action is needed as toggling
            // anything but the first false value is unobservable.
        }
    }
};

enum class Action {
    PUSH,
    POP,
    TOGGLE,
};

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    ConditionStack_d0e8f4d5d8ddaccb37f98b7989fb944081e41ab8 cs_1;
    ConditionStack_e6e622e5a0e22c2ac1b50b96af818e412d67ac54 cs_2;
    while (fuzzed_data_provider.remaining_bytes()) {
        const Action act = fuzzed_data_provider.PickValueInArray({Action::PUSH, Action::POP, Action::TOGGLE});
        switch (act) {
        case Action::PUSH: {
            const bool condition{fuzzed_data_provider.ConsumeBool()};
            cs_1.push_back(condition);
            cs_2.push_back(condition);
            break;
        }
        case Action::POP: {
            if (!cs_1.empty()) {
                cs_1.pop_back();
                cs_2.pop_back();
            }
            break;
        }
        case Action::TOGGLE: {
            if (!cs_1.empty()) {
                cs_1.toggle_top();
                cs_2.toggle_top();
            }
            break;
        }
        }

        assert(cs_1.empty() == cs_2.empty());
        assert(cs_1.all_true() == cs_2.all_true());
    }
}
