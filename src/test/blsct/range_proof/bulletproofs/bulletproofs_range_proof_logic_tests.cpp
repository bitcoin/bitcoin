// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/range_proof/bulletproofs/range_proof_logic.h>
#include <blsct/range_proof/common.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/imp_inner_prod_arg.h>
#include <test/util/setup_common.h>

#include <tinyformat.h>
#include <boost/test/unit_test.hpp>
#include <util/strencodings.h>

BOOST_FIXTURE_TEST_SUITE(bulletproofs_range_proof_logic_tests, BasicTestingSetup)

using T = Mcl;
using Point = T::Point;
using Scalar = T::Scalar;
using Scalars = Elements<Scalar>;
using MsgPair = std::pair<std::string, std::vector<unsigned char>>;

struct TestCase
{
    std::string name;
    Scalars values;
    bool is_batched;
    bool should_complete_recovery;
    size_t num_amounts;
    bool verify_result;
    MsgPair msg;
    Scalar min_value;
};

static MclG1Point GenNonce()
{
    std::string nonce_str("nonce");
    MclG1Point nonce = MclG1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });
    return nonce;
}

static TokenId GenTokenId()
{
    TokenId token_id(uint256(123));
    return token_id;
}

static MsgPair GenMsgPair(std::string s = "spaghetti meatballs")
{
    std::vector<unsigned char> message { s.begin(), s.end() };
    return std::pair(s, message);
}

BOOST_AUTO_TEST_CASE(test_range_proof_prove_verify_one_value)
{
    auto nonce = GenNonce();
    auto msg = GenMsgPair();
    auto token_id = GenTokenId();

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    bulletproofs::RangeProofLogic<T> rp;
    auto p = rp.Prove(vs, nonce, msg.second, token_id);
    std::vector<bulletproofs::RangeProofWithSeed<T>> proofs;
    bulletproofs::RangeProofWithSeed<T> proof{p, token_id};
    proofs.emplace_back(proof);
    auto is_valid = rp.Verify(proofs);
    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(test_range_proof_recovery_one_value)
{
    auto nonce = GenNonce();
    auto msg = GenMsgPair();
    auto token_id = GenTokenId();

    Scalar one(1);
    Scalars vs;
    vs.Add(one);

    bulletproofs::RangeProofLogic<T> rp;
    auto proof = rp.Prove(vs, nonce, msg.second, token_id);
    bulletproofs::RangeProofWithSeed<T> proofWithSeed = {proof, token_id};

    auto req = bulletproofs::AmountRecoveryRequest<T>::of(proofWithSeed, nonce);
    auto reqs = std::vector<bulletproofs::AmountRecoveryRequest<T>> { req };
    auto result = rp.RecoverAmounts(reqs);

    BOOST_CHECK(result.is_completed);
    auto xs = result.amounts;
    BOOST_CHECK(xs.size() == 1);
    BOOST_CHECK(xs[0].gamma == nonce.GetHashWithSalt(100));
    BOOST_CHECK(xs[0].amount == 1);
    BOOST_CHECK(xs[0].message == msg.first);
}

static std::vector<TestCase> BuildTestCases()
{
    bulletproofs::RangeProofLogic<T> rp;
    std::vector<TestCase> test_cases;

    for (auto& lower_bound : {Scalar(0), Scalar(100)}) {
        Scalar one(1);
        Scalar two(2);
        Scalar upper_bound = (one << 64) - one; // int64_t max
        // [LB, LB+1, UB-1, UB]
        Scalars valid_inputs;
        valid_inputs.Add(lower_bound);
        valid_inputs.Add(lower_bound + one);
        valid_inputs.Add(upper_bound - one);
        valid_inputs.Add(upper_bound);

        // [-1, UB+1, UB+2, UB*2]
        Scalars invalid_inputs;
        invalid_inputs.Add(one.Negate());
        invalid_inputs.Add(upper_bound + one);
        invalid_inputs.Add(upper_bound + one + one);
        invalid_inputs.Add(upper_bound << 1);

        // test single valid value
        for (auto value : valid_inputs.m_vec) {
            Scalars values;
            values.Add(value);

            TestCase x;
            x.name = strprintf("valid input value %s min_value=%s", value.GetString(), lower_bound.GetString()).c_str();
            x.values = values;
            x.is_batched = false;
            x.should_complete_recovery = true;
            x.num_amounts = 1;
            x.msg = GenMsgPair();
            x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);
            x.min_value = lower_bound;
            test_cases.push_back(x);
        }

        // test single invalid value
        for (auto value : invalid_inputs.m_vec) {
            Scalars values;
            values.Add(value);

            TestCase x;
            x.name = strprintf("invalid input value %s min_value=%s", value.GetString(), lower_bound.GetString()).c_str();
            x.values = values;
            x.is_batched = false;
            x.should_complete_recovery = true;
            x.num_amounts = 0;
            x.msg = GenMsgPair();
            x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);
            x.min_value = lower_bound;
            test_cases.push_back(x);
        }

        // test batched valid values
        {
            TestCase x;
            x.name = "batched valid values";
            x.values = valid_inputs;
            x.is_batched = true;
            x.should_complete_recovery = true;
            x.num_amounts = 0;
            x.msg = GenMsgPair();
            x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);
            x.min_value = lower_bound;
            test_cases.push_back(x);
        }

        // test batched invalid values
        {
            TestCase x;
            x.name = "batched invalid values";
            x.values = invalid_inputs;
            x.is_batched = true;
            x.should_complete_recovery = true;
            x.num_amounts = 0;
            x.msg = GenMsgPair();
            x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);
            x.min_value = lower_bound;
            test_cases.push_back(x);
        }

        // test with messages of various length
        {
            Scalars values;
            values.Add(Scalar(1));

            std::vector<size_t> msg_sizes{1ul, 23ul, 24ul, range_proof::Setup::max_message_size};
            for (auto msg_size : msg_sizes) {
                TestCase x;
                x.name = strprintf("with message of length %d min_value=%s", msg_size, lower_bound.GetString()).c_str();
                x.values = values;
                x.is_batched = true;
                x.should_complete_recovery = true;
                x.num_amounts = 1;
                x.msg = GenMsgPair(std::string(msg_size, 'x'));
                x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);
                x.min_value = lower_bound;
                test_cases.push_back(x);
            }
        }

        // test # of input values from 1 to max
        {
            for (size_t n = 1; n <= range_proof::Setup::max_input_values; ++n) {
                Scalars values;
                for (size_t i = 0; i < n; ++i) {
                    values.Add(Scalar(i + 1));
                }
                TestCase x;
                x.name = strprintf("%d valid input values min_value=%s", n, lower_bound.GetString()).c_str();
                x.values = values;
                x.is_batched = true;
                x.should_complete_recovery = true;
                x.num_amounts = n == 1 ? 1 : 0; // recovery should be performed only when n=1
                x.msg = GenMsgPair();
                x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);
                x.min_value = lower_bound;
                test_cases.push_back(x);
            }
        }

        // test valid and invalid values mixed
        {
            Scalars values;
            for (auto& s : valid_inputs.m_vec)
                values.Add(s);
            for (auto& s : invalid_inputs.m_vec)
                values.Add(s);

            TestCase x;
            x.name = "mix of valid and invalid values";
            x.values = values;
            x.is_batched = true;
            x.should_complete_recovery = true;
            x.num_amounts = 0;
            x.msg = GenMsgPair();
            x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);
            x.min_value = lower_bound;
            test_cases.push_back(x);
        }

        {
            // string of maximum message size 54
            const std::string s("Pneumonoultramicroscopicsilicovolcanoconiosis123456789");
            assert(s.size() == range_proof::Setup::max_message_size);
            Scalars values;
            values.Add(one);

            for (size_t i = 0; i <= s.size(); ++i) { // try message of size 0 to 54
                auto msg = s.substr(0, i);

                TestCase x;
                x.name = strprintf("message size %ld min_value=%s", i, lower_bound.GetString()).c_str();
                x.values = values;
                x.is_batched = false;
                x.should_complete_recovery = true;
                x.num_amounts = 1;
                x.msg = GenMsgPair(msg);
                x.verify_result = x.values >= lower_bound && x.values <= (upper_bound + lower_bound);

                x.min_value = lower_bound;
                test_cases.push_back(x);
            }
        }
    }

    return test_cases;
}

static void RunTestCase(
    bulletproofs::RangeProofLogic<T>& rp,
    TestCase& test_case
) {
    auto token_id = GenTokenId();
    auto nonce = GenNonce();

    std::vector<bulletproofs::RangeProofWithSeed<T>> proofs;

    // calculate proofs
    if (test_case.is_batched) {
        auto proof = rp.Prove(test_case.values, nonce, test_case.msg.second, token_id, test_case.min_value);
        bulletproofs::RangeProofWithSeed<T> p{proof, token_id, test_case.min_value};
        proofs.emplace_back(p);
    } else {
        for (auto value: test_case.values.m_vec) {
            Scalars single_value_vec;
            single_value_vec.Add(value);

            auto proof = rp.Prove(single_value_vec, nonce, test_case.msg.second, token_id, test_case.min_value);
            bulletproofs::RangeProofWithSeed<T> p{proof, token_id, test_case.min_value};
            proofs.emplace_back(p);
        }
    }

    // verify proofs
    auto verify_result = rp.Verify(proofs);
    BOOST_CHECK(verify_result == test_case.verify_result);

    // recover value, gamma and message
    std::vector<bulletproofs::AmountRecoveryRequest<T>> reqs;

    for (size_t i=0; i<proofs.size(); ++i) {
        reqs.push_back(bulletproofs::AmountRecoveryRequest<T>::of(proofs[i], nonce));
    }
    auto recovery_result = rp.RecoverAmounts(reqs);
    BOOST_CHECK(recovery_result.is_completed == test_case.should_complete_recovery);

    if (recovery_result.is_completed) {
        auto amounts = recovery_result.amounts;

        BOOST_CHECK(amounts.size() == test_case.num_amounts);

        for (size_t i=0; i<amounts.size(); ++i) {
            auto x = amounts[i];
            auto gamma = nonce.GetHashWithSalt(100 + i);
            BOOST_CHECK(((uint64_t)x.amount) == test_case.values[i].GetUint64());
            BOOST_CHECK(x.gamma == gamma);

            std::vector<unsigned char> x_msg(x.message.begin(), x.message.end());
            BOOST_CHECK(x_msg == test_case.msg.second);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_range_proof_prove_verify_recovery)
{
    auto test_cases = BuildTestCases();
    bulletproofs::RangeProofLogic<T> rp;
    for (auto test_case: test_cases) {
        RunTestCase(rp, test_case);
    }
}

BOOST_AUTO_TEST_CASE(test_range_proof_message_size)
{
    bulletproofs::RangeProofLogic<T> rp;

    Scalars values;
    values.Add(Scalar(1));
    MclG1Point nonce = MclG1Point::GetBasePoint();
    TokenId token_id;

    {
        // empty msg
        std::vector<unsigned char> msg;
        BOOST_CHECK_NO_THROW(rp.Prove(values, nonce, msg, token_id));
    }
    {
        // msg of valid size
        std::string s(range_proof::Setup::max_message_size, 'x');
        std::vector<unsigned char> msg(s.begin(), s.end());
        BOOST_CHECK_NO_THROW(rp.Prove(values, nonce, msg, token_id));
    }
    {
        // msg of exceeded size
        std::string s(range_proof::Setup::max_message_size + 1, 'x');
        std::vector<unsigned char> msg(s.begin(), s.end());
        BOOST_CHECK_THROW(rp.Prove(values, nonce, msg, token_id), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_range_proof_number_of_input_values)
{
    bulletproofs::RangeProofLogic<T> rp;
    MclG1Point nonce = MclG1Point::GetBasePoint();
    std::vector<unsigned char> msg;
    TokenId token_id;

    {
        // should throw if there is no input value
        Scalars values;
        BOOST_CHECK_THROW(rp.Prove(values, nonce, msg, token_id), std::runtime_error);
    }
    {
        // should not throw if number of input values is within the valid range
        Scalars values;
        values.Add(Scalar(1));
        BOOST_CHECK_NO_THROW(rp.Prove(values, nonce, msg, token_id));
    }
    {
        // should throw if number of input values is outsize the valid range
        Scalars values;
        for (size_t i=0; i<range_proof::Setup::max_input_values + 1; ++i) {
            values.Add(Scalar(1));
        }
        BOOST_CHECK_THROW(rp.Prove(values, nonce, msg, token_id), std::runtime_error);
    }
}

BOOST_AUTO_TEST_SUITE_END()
