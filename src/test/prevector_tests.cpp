// Copyright (c) 2015-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include "prevector.h"

#include "serialize.h"
#include "streams.h"

#include "test/test_bitcoin.h"
#include "test/test_random.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(PrevectorTests, TestingSetup)

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
        BOOST_FOREACH(const T& v, pre_vector) {
             local_check(v == real_vector[pos++]);
        }
        BOOST_REVERSE_FOREACH(const T& v, pre_vector) {
             local_check(v == real_vector[--pos]);
        }
        BOOST_FOREACH(const T& v, const_pre_vector) {
             local_check(v == real_vector[pos++]);
        }
        BOOST_REVERSE_FOREACH(const T& v, const_pre_vector) {
             local_check(v == real_vector[--pos]);
        }
        CDataStream ss1(SER_DISK, 0);
        CDataStream ss2(SER_DISK, 0);
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

    Size size() {
        return real_vector.size();
    }

    Size capacity() {
        return pre_vector.capacity();
    }

    void shrink_to_fit() {
        pre_vector.shrink_to_fit();
        test();
    }

    void swap() {
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

    ~prevector_tester() {
        BOOST_CHECK_MESSAGE(passed, "insecure_rand_Rz: "
                << rand_cache.Rz
                << ", insecure_rand_Rw: "
                << rand_cache.Rw);
    }
    prevector_tester() {
        seed_insecure_rand();
        rand_cache = insecure_rand_ctx;
    }
};

BOOST_AUTO_TEST_CASE(PrevectorTestInt)
{
    for (int j = 0; j < 64; j++) {
        prevector_tester<8, int> test;
        for (int i = 0; i < 2048; i++) {
            int r = insecure_rand();
            if ((r % 4) == 0) {
                test.insert(insecure_rand() % (test.size() + 1), insecure_rand());
            }
            if (test.size() > 0 && ((r >> 2) % 4) == 1) {
                test.erase(insecure_rand() % test.size());
            }
            if (((r >> 4) % 8) == 2) {
                int new_size = std::max<int>(0, std::min<int>(30, test.size() + (insecure_rand() % 5) - 2));
                test.resize(new_size);
            }
            if (((r >> 7) % 8) == 3) {
                test.insert(insecure_rand() % (test.size() + 1), 1 + (insecure_rand() % 2), insecure_rand());
            }
            if (((r >> 10) % 8) == 4) {
                int del = std::min<int>(test.size(), 1 + (insecure_rand() % 2));
                int beg = insecure_rand() % (test.size() + 1 - del);
                test.erase(beg, beg + del);
            }
            if (((r >> 13) % 16) == 5) {
                test.push_back(insecure_rand());
            }
            if (test.size() > 0 && ((r >> 17) % 16) == 6) {
                test.pop_back();
            }
            if (((r >> 21) % 32) == 7) {
                int values[4];
                int num = 1 + (insecure_rand() % 4);
                for (int k = 0; k < num; k++) {
                    values[k] = insecure_rand();
                }
                test.insert_range(insecure_rand() % (test.size() + 1), values, values + num);
            }
            if (((r >> 26) % 32) == 8) {
                int del = std::min<int>(test.size(), 1 + (insecure_rand() % 4));
                int beg = insecure_rand() % (test.size() + 1 - del);
                test.erase(beg, beg + del);
            }
            r = insecure_rand();
            if (r % 32 == 9) {
                test.reserve(insecure_rand() % 32);
            }
            if ((r >> 5) % 64 == 10) {
                test.shrink_to_fit();
            }
            if (test.size() > 0) {
                test.update(insecure_rand() % test.size(), insecure_rand());
            }
            if (((r >> 11) % 1024) == 11) {
                test.clear();
            }
            if (((r >> 21) % 512) == 12) {
                test.assign(insecure_rand() % 32, insecure_rand());
            }
            if (((r >> 15) % 8) == 3) {
                test.swap();
            }
            if (((r >> 15) % 16) == 8) {
                test.copy();
            }
            if (((r >> 15) % 32) == 18) {
                test.move();
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
