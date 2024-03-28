// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <prevector.h>
#include <vector>

#include <reverse_iterator.h>
#include <serialize.h>
#include <streams.h>

#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(prevector_tests, TestingSetup)

template<unsigned int N, typename T>
class prevector_tester {
    typedef std::vector<T> realtype;
    realtype real_vector;
    realtype real_vector_alt;

    typedef prevector<N, T> pretype;
    pretype pre_vector;
    pretype pre_vector_alt;

    typedef typename pretype::size_type Size;
    bool passed = true;
    FastRandomContext rand_cache;
    uint256 rand_seed;


    template <typename A, typename B>
        void local_check_equal(A a, B b)
        {
            local_check(a == b);
        }
    void local_check(bool b)
    {
        passed &= b;
    }
    void test() {
        const pretype& const_pre_vector = pre_vector;
        local_check_equal(real_vector.size(), pre_vector.size());
        local_check_equal(real_vector.empty(), pre_vector.empty());
        for (Size s = 0; s < real_vector.size(); s++) {
             local_check(real_vector[s] == pre_vector[s]);
             local_check(&(pre_vector[s]) == &(pre_vector.begin()[s]));
             local_check(&(pre_vector[s]) == &*(pre_vector.begin() + s));
             local_check(&(pre_vector[s]) == &*((pre_vector.end() + s) - real_vector.size()));
        }
        // local_check(realtype(pre_vector) == real_vector);
        local_check(pretype(real_vector.begin(), real_vector.end()) == pre_vector);
        local_check(pretype(pre_vector.begin(), pre_vector.end()) == pre_vector);
        size_t pos = 0;
        for (const T& v : pre_vector) {
             local_check(v == real_vector[pos++]);
        }
        for (const T& v : reverse_iterate(pre_vector)) {
             local_check(v == real_vector[--pos]);
        }
        for (const T& v : const_pre_vector) {
             local_check(v == real_vector[pos++]);
        }
        for (const T& v : reverse_iterate(const_pre_vector)) {
             local_check(v == real_vector[--pos]);
        }
        DataStream ss1{};
        DataStream ss2{};
        ss1 << real_vector;
        ss2 << pre_vector;
        local_check_equal(ss1.size(), ss2.size());
        for (Size s = 0; s < ss1.size(); s++) {
            local_check_equal(ss1[s], ss2[s]);
        }
    }

public:
    void resize(Size s) {
        real_vector.resize(s);
        local_check_equal(real_vector.size(), s);
        pre_vector.resize(s);
        local_check_equal(pre_vector.size(), s);
        test();
    }

    void reserve(Size s) {
        real_vector.reserve(s);
        local_check(real_vector.capacity() >= s);
        pre_vector.reserve(s);
        local_check(pre_vector.capacity() >= s);
        test();
    }

    void insert(Size position, const T& value) {
        real_vector.insert(real_vector.begin() + position, value);
        pre_vector.insert(pre_vector.begin() + position, value);
        test();
    }

    void insert(Size position, Size count, const T& value) {
        real_vector.insert(real_vector.begin() + position, count, value);
        pre_vector.insert(pre_vector.begin() + position, count, value);
        test();
    }

    template<typename I>
    void insert_range(Size position, I first, I last) {
        real_vector.insert(real_vector.begin() + position, first, last);
        pre_vector.insert(pre_vector.begin() + position, first, last);
        test();
    }

    void erase(Size position) {
        real_vector.erase(real_vector.begin() + position);
        pre_vector.erase(pre_vector.begin() + position);
        test();
    }

    void erase(Size first, Size last) {
        real_vector.erase(real_vector.begin() + first, real_vector.begin() + last);
        pre_vector.erase(pre_vector.begin() + first, pre_vector.begin() + last);
        test();
    }

    void update(Size pos, const T& value) {
        real_vector[pos] = value;
        pre_vector[pos] = value;
        test();
    }

    void push_back(const T& value) {
        real_vector.push_back(value);
        pre_vector.push_back(value);
        test();
    }

    void pop_back() {
        real_vector.pop_back();
        pre_vector.pop_back();
        test();
    }

    void clear() {
        real_vector.clear();
        pre_vector.clear();
    }

    void assign(Size n, const T& value) {
        real_vector.assign(n, value);
        pre_vector.assign(n, value);
    }

    Size size() const {
        return real_vector.size();
    }

    Size capacity() const {
        return pre_vector.capacity();
    }

    void shrink_to_fit() {
        pre_vector.shrink_to_fit();
        test();
    }

    void swap() noexcept
    {
        real_vector.swap(real_vector_alt);
        pre_vector.swap(pre_vector_alt);
        test();
    }

    void move() {
        real_vector = std::move(real_vector_alt);
        real_vector_alt.clear();
        pre_vector = std::move(pre_vector_alt);
        pre_vector_alt.clear();
    }

    void copy() {
        real_vector = real_vector_alt;
        pre_vector = pre_vector_alt;
    }

    void resize_uninitialized(realtype values) {
        size_t r = values.size();
        size_t s = real_vector.size() / 2;
        if (real_vector.capacity() < s + r) {
            real_vector.reserve(s + r);
        }
        real_vector.resize(s);
        pre_vector.resize_uninitialized(s);
        for (auto v : values) {
            real_vector.push_back(v);
        }
        auto p = pre_vector.size();
        pre_vector.resize_uninitialized(p + r);
        for (auto v : values) {
            pre_vector[p] = v;
            ++p;
        }
        test();
    }

    ~prevector_tester() {
        BOOST_CHECK_MESSAGE(passed, "insecure_rand: " + rand_seed.ToString());
    }

    prevector_tester() {
        SeedRandomForTest();
        rand_seed = InsecureRand256();
        rand_cache = FastRandomContext(rand_seed);
    }
};

BOOST_AUTO_TEST_CASE(PrevectorTestInt)
{
    for (int j = 0; j < 64; j++) {
        prevector_tester<8, int> test;
        for (int i = 0; i < 2048; i++) {
            if (InsecureRandBits(2) == 0) {
                test.insert(InsecureRandRange(test.size() + 1), int(InsecureRand32()));
            }
            if (test.size() > 0 && InsecureRandBits(2) == 1) {
                test.erase(InsecureRandRange(test.size()));
            }
            if (InsecureRandBits(3) == 2) {
                int new_size = std::max(0, std::min(30, (int)test.size() + (int)InsecureRandRange(5) - 2));
                test.resize(new_size);
            }
            if (InsecureRandBits(3) == 3) {
                test.insert(InsecureRandRange(test.size() + 1), 1 + InsecureRandBool(), int(InsecureRand32()));
            }
            if (InsecureRandBits(3) == 4) {
                int del = std::min<int>(test.size(), 1 + (InsecureRandBool()));
                int beg = InsecureRandRange(test.size() + 1 - del);
                test.erase(beg, beg + del);
            }
            if (InsecureRandBits(4) == 5) {
                test.push_back(int(InsecureRand32()));
            }
            if (test.size() > 0 && InsecureRandBits(4) == 6) {
                test.pop_back();
            }
            if (InsecureRandBits(5) == 7) {
                int values[4];
                int num = 1 + (InsecureRandBits(2));
                for (int k = 0; k < num; k++) {
                    values[k] = int(InsecureRand32());
                }
                test.insert_range(InsecureRandRange(test.size() + 1), values, values + num);
            }
            if (InsecureRandBits(5) == 8) {
                int del = std::min<int>(test.size(), 1 + (InsecureRandBits(2)));
                int beg = InsecureRandRange(test.size() + 1 - del);
                test.erase(beg, beg + del);
            }
            if (InsecureRandBits(5) == 9) {
                test.reserve(InsecureRandBits(5));
            }
            if (InsecureRandBits(6) == 10) {
                test.shrink_to_fit();
            }
            if (test.size() > 0) {
                test.update(InsecureRandRange(test.size()), int(InsecureRand32()));
            }
            if (InsecureRandBits(10) == 11) {
                test.clear();
            }
            if (InsecureRandBits(9) == 12) {
                test.assign(InsecureRandBits(5), int(InsecureRand32()));
            }
            if (InsecureRandBits(3) == 3) {
                test.swap();
            }
            if (InsecureRandBits(4) == 8) {
                test.copy();
            }
            if (InsecureRandBits(5) == 18) {
                test.move();
            }
            if (InsecureRandBits(5) == 19) {
                unsigned int num = 1 + (InsecureRandBits(4));
                std::vector<int> values(num);
                for (int& v : values) {
                    v = int(InsecureRand32());
                }
                test.resize_uninitialized(values);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
