// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>

#include <vector>
#include <prevector.h>

#include <reverse_iterator.h>
#include <serialize.h>
#include <streams.h>

namespace {

template<unsigned int N, typename T>
class prevector_tester {
    typedef std::vector<T> realtype;
    realtype real_vector;
    realtype real_vector_alt;

    typedef prevector<N, T> pretype;
    pretype pre_vector;
    pretype pre_vector_alt;

    typedef typename pretype::size_type Size;

public:
    void test() const {
        const pretype& const_pre_vector = pre_vector;
        assert(real_vector.size() == pre_vector.size());
        assert(real_vector.empty() == pre_vector.empty());
        for (Size s = 0; s < real_vector.size(); s++) {
             assert(real_vector[s] == pre_vector[s]);
             assert(&(pre_vector[s]) == &(pre_vector.begin()[s]));
             assert(&(pre_vector[s]) == &*(pre_vector.begin() + s));
             assert(&(pre_vector[s]) == &*((pre_vector.end() + s) - real_vector.size()));
        }
        // assert(realtype(pre_vector) == real_vector);
        assert(pretype(real_vector.begin(), real_vector.end()) == pre_vector);
        assert(pretype(pre_vector.begin(), pre_vector.end()) == pre_vector);
        size_t pos = 0;
        for (const T& v : pre_vector) {
             assert(v == real_vector[pos]);
             ++pos;
        }
        for (const T& v : reverse_iterate(pre_vector)) {
             --pos;
             assert(v == real_vector[pos]);
        }
        for (const T& v : const_pre_vector) {
             assert(v == real_vector[pos]);
             ++pos;
        }
        for (const T& v : reverse_iterate(const_pre_vector)) {
             --pos;
             assert(v == real_vector[pos]);
        }
        CDataStream ss1(SER_DISK, 0);
        CDataStream ss2(SER_DISK, 0);
        ss1 << real_vector;
        ss2 << pre_vector;
        assert(ss1.size() == ss2.size());
        for (Size s = 0; s < ss1.size(); s++) {
            assert(ss1[s] == ss2[s]);
        }
    }

    void resize(Size s) {
        real_vector.resize(s);
        assert(real_vector.size() == s);
        pre_vector.resize(s);
        assert(pre_vector.size() == s);
    }

    void reserve(Size s) {
        real_vector.reserve(s);
        assert(real_vector.capacity() >= s);
        pre_vector.reserve(s);
        assert(pre_vector.capacity() >= s);
    }

    void insert(Size position, const T& value) {
        real_vector.insert(real_vector.begin() + position, value);
        pre_vector.insert(pre_vector.begin() + position, value);
    }

    void insert(Size position, Size count, const T& value) {
        real_vector.insert(real_vector.begin() + position, count, value);
        pre_vector.insert(pre_vector.begin() + position, count, value);
    }

    template<typename I>
    void insert_range(Size position, I first, I last) {
        real_vector.insert(real_vector.begin() + position, first, last);
        pre_vector.insert(pre_vector.begin() + position, first, last);
    }

    void erase(Size position) {
        real_vector.erase(real_vector.begin() + position);
        pre_vector.erase(pre_vector.begin() + position);
    }

    void erase(Size first, Size last) {
        real_vector.erase(real_vector.begin() + first, real_vector.begin() + last);
        pre_vector.erase(pre_vector.begin() + first, pre_vector.begin() + last);
    }

    void update(Size pos, const T& value) {
        real_vector[pos] = value;
        pre_vector[pos] = value;
    }

    void push_back(const T& value) {
        real_vector.push_back(value);
        pre_vector.push_back(value);
    }

    void pop_back() {
        real_vector.pop_back();
        pre_vector.pop_back();
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
    }

    void swap() {
        real_vector.swap(real_vector_alt);
        pre_vector.swap(pre_vector_alt);
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
    }
};

}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider prov(buffer.data(), buffer.size());
    prevector_tester<8, int> test;

    while (prov.remaining_bytes()) {
        switch (prov.ConsumeIntegralInRange<int>(0, 13 + 3 * (test.size() > 0))) {
        case 0:
            test.insert(prov.ConsumeIntegralInRange<size_t>(0, test.size()), prov.ConsumeIntegral<int>());
            break;
        case 1:
            test.resize(std::max(0, std::min(30, (int)test.size() + prov.ConsumeIntegralInRange<int>(0, 4) - 2)));
            break;
        case 2:
            test.insert(prov.ConsumeIntegralInRange<size_t>(0, test.size()), 1 + prov.ConsumeBool(), prov.ConsumeIntegral<int>());
            break;
        case 3: {
            int del = prov.ConsumeIntegralInRange<int>(0, test.size());
            int beg = prov.ConsumeIntegralInRange<int>(0, test.size() - del);
            test.erase(beg, beg + del);
            break;
        }
        case 4:
            test.push_back(prov.ConsumeIntegral<int>());
            break;
        case 5: {
            int values[4];
            int num = 1 + prov.ConsumeIntegralInRange<int>(0, 3);
            for (int k = 0; k < num; ++k) {
                values[k] = prov.ConsumeIntegral<int>();
            }
            test.insert_range(prov.ConsumeIntegralInRange<size_t>(0, test.size()), values, values + num);
            break;
        }
        case 6: {
            int num = 1 + prov.ConsumeIntegralInRange<int>(0, 15);
            std::vector<int> values(num);
            for (auto& v : values) {
                v = prov.ConsumeIntegral<int>();
            }
            test.resize_uninitialized(values);
            break;
        }
        case 7:
            test.reserve(prov.ConsumeIntegralInRange<size_t>(0, 32767));
            break;
        case 8:
            test.shrink_to_fit();
            break;
        case 9:
            test.clear();
            break;
        case 10:
            test.assign(prov.ConsumeIntegralInRange<size_t>(0, 32767), prov.ConsumeIntegral<int>());
            break;
        case 11:
            test.swap();
            break;
        case 12:
            test.copy();
            break;
        case 13:
            test.move();
            break;
        case 14:
            test.update(prov.ConsumeIntegralInRange<size_t>(0, test.size() - 1), prov.ConsumeIntegral<int>());
            break;
        case 15:
            test.erase(prov.ConsumeIntegralInRange<size_t>(0, test.size() - 1));
            break;
        case 16:
            test.pop_back();
            break;
        }
    }

    test.test();
}
