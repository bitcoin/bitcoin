// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <blsct/arith/range_proof/range_proof.h>
// #include <blsct/arith/range_proof/range_proof_orig.h>

#include <boost/test/unit_test.hpp>
#include <util/strencodings.h>

BOOST_FIXTURE_TEST_SUITE(range_proof_tests, MclTestingSetup)

static G1Point GenNonce()
{
    std::string nonce_str("nonce");
    G1Point nonce = G1Point::HashAndMap(std::vector<unsigned char> { nonce_str.begin(), nonce_str.end() });
    return nonce;
}

static TokenId GenTokenId()
{
    TokenId token_id(uint256(123));
    return token_id;
}

static std::pair<std::string, std::vector<unsigned char>> GenMessage()
{
    std::string msg_str("spagetti meatballs");
    std::vector<unsigned char> message { msg_str.begin(), msg_str.end() };
    return std::pair(msg_str, message);
}

BOOST_AUTO_TEST_CASE(test_range_proof_prove_verify_one_value)
{
    auto nonce = GenNonce();
    auto msg = GenMessage();
    auto token_id = GenTokenId();

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    RangeProof rp;
    auto p = rp.Prove(vs, nonce, msg.second, token_id);

    auto is_valid = rp.Verify(std::vector<Proof> { p }, token_id);
    BOOST_CHECK(is_valid);
}

BOOST_AUTO_TEST_CASE(test_range_proof_recovery_one_value)
{
    auto nonce = GenNonce();
    auto msg = GenMessage();
    auto token_id = GenTokenId();

    Scalar one(1);
    std::vector<Scalar> vs_vec;
    vs_vec.push_back(one);

    Scalars vs;
    vs.Add(one);

    RangeProof rp;
    auto proof = rp.Prove(vs, nonce, msg.second, token_id);

    size_t index = 0;
    auto req = AmountRecoveryRequest::of(proof, index, nonce);
    auto reqs = std::vector<AmountRecoveryRequest> { req };
    auto result = rp.RecoverAmounts(reqs, token_id);

    BOOST_CHECK(result.is_completed);
    auto xs = result.amounts;
    BOOST_CHECK(xs.size() == 1);
    BOOST_CHECK(xs[0].gamma == nonce.GetHashWithSalt(100));
    BOOST_CHECK(xs[0].amount == 1);
    BOOST_CHECK(xs[0].message == msg.first);
}

struct TestCase
{
    std::string name;
    Scalars values;
    bool is_batch;  // if true, prove function is called for each value in values
    bool should_complete_recovery;
    size_t num_amounts;
    bool verify_result;
};

static std::vector<TestCase> BuildTestCases()
{
    RangeProof rp;

    Scalar one(1);
    Scalar two(2);
    Scalar lower_bound(0);
    Scalar upper_bound = (one << 64) - one;  // int64_t max
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

    std::vector<TestCase> test_cases;

    // test single valid value
    for (auto value: valid_inputs.m_vec) {
        Scalars values;
        values.Add(value);

        TestCase x;
        x.name = "single valid value";
        x.values = values;
        x.is_batch = false;
        x.verify_result = true;
        x.should_complete_recovery = true;
        x.num_amounts = 1;
        test_cases.push_back(x);
    }

    // test single invalid value
    for (auto value: invalid_inputs.m_vec) {
        Scalars values;
        values.Add(value);

        TestCase x;
        x.name = "single invalid value";
        x.values = values;
        x.is_batch = false;
        x.should_complete_recovery = true;
        x.num_amounts = 0;
        x.verify_result = false;
        test_cases.push_back(x);
    }

    // test valid values
    for (auto is_batch: std::vector<bool> { true, false }) {
        auto num_amounts = is_batch ? valid_inputs.Size() : 1;
        TestCase x;
        x.name = "valid values";
        x.values = valid_inputs;
        x.is_batch = is_batch;
        x.should_complete_recovery = true;
        x.num_amounts = num_amounts;
        x.verify_result = true;
        test_cases.push_back(x);
    }

    // test invalid values
    for (auto is_batch: std::vector<bool> { true, false }) {
        //auto num_amounts = is_batch ? invalid_inputs.Size() : 1;
        TestCase x;
        x.name = "invalid values";
        x.values = invalid_inputs;
        x.is_batch = is_batch;
        x.should_complete_recovery = true;
        x.num_amounts = 0;
        x.verify_result = false;
        test_cases.push_back(x);
    }

    // // test single valid values w/ # of values not power of 2
    // {
    //     Scalars values;
    //     for (size_t i=0; i<3; ++i) values.Add(valid_inputs[i]);

    //     TestCase x;
    //     x.name = "# of values not power of 2";
    //     x.values = values;
    //     x.is_batch = false;
        // x.should_complete_recovery = true;
        // x.num_amounts = 0;
    //     x.verify_result = true;
    //     test_cases.push_back(x);
    // }

    // test valid input values of maximum number
    {
        Scalars values;
        for (size_t i=0; i<Config::m_max_input_values; ++i) {
            values.Add(Scalar(i + 1));
        }
        TestCase x;
        x.name = "max # of input values";
        x.values = values;
        x.is_batch = false;
        x.should_complete_recovery = true;
        x.num_amounts = 1;
        x.verify_result = true;
        test_cases.push_back(x);
    }

    // test valid and invalid values mixed
    {
        Scalars values;
        for (auto& s: valid_inputs.m_vec) values.Add(s);
        for (auto& s: invalid_inputs.m_vec) values.Add(s);

        TestCase x;
        x.name = "mix of valid and invalid values";
        x.values = values;
        x.is_batch = false;
        x.should_complete_recovery = true;
        x.num_amounts = 1;
        x.verify_result = false;
        test_cases.push_back(x);
    }

    return test_cases;
}

static void RunTestCase(
    RangeProof& rp,
    TestCase& test_case
) {
    printf("===> Testing '%s'\n", test_case.name.c_str());

    auto msg = GenMessage();
    auto token_id = GenTokenId();
    auto nonce = GenNonce();

    std::vector<Proof> proofs;

    // calculate proofs
    if (test_case.is_batch) {
        for (auto value: test_case.values.m_vec) {
            Scalars single_value_vec;
            single_value_vec.Add(value);
            auto proof = rp.Prove(single_value_vec, nonce, msg.second, token_id);
            proofs.push_back(proof);
        }
    } else {
        auto proof = rp.Prove(test_case.values, nonce, msg.second, token_id);
        proofs.push_back(proof);
    }

    // verify proofs
    auto verify_result = rp.Verify(proofs, token_id);
    if (verify_result != test_case.verify_result) {
        printf("=====> VERIFICATION FAILED\n");
    }
    BOOST_CHECK(verify_result == test_case.verify_result);

    // recover value, gamma and message
    std::vector<AmountRecoveryRequest> reqs;

    for (size_t i=0; i<proofs.size(); ++i) {
        reqs.push_back(AmountRecoveryRequest::of(proofs[i], i, nonce));
    }
    auto recovery_result = rp.RecoverAmounts(reqs, token_id);
    BOOST_CHECK(recovery_result.is_completed == test_case.should_complete_recovery);

    if (recovery_result.is_completed) {
        auto amounts = recovery_result.amounts;
        printf("amounts=%ld, num_amounts=%ld\n", amounts.size(), test_case.num_amounts);
        BOOST_CHECK(amounts.size() == test_case.num_amounts);

        for (size_t i=0; i<amounts.size(); ++i) {
            auto x = amounts[i];
            auto gamma = nonce.GetHashWithSalt(100 + i);

            BOOST_CHECK(((uint64_t) x.amount) == test_case.values[i].GetUint64());
            BOOST_CHECK(x.gamma == gamma);

            std::vector<unsigned char> x_msg(x.message.begin(), x.message.end());
            printf("===> act msg: %s, exp msg: %s\n", x.message.c_str(), msg.first.c_str());
            BOOST_CHECK(x_msg == msg.second);
        }
    }
}


BOOST_AUTO_TEST_CASE(test_range_proof_prove_verify_recovery_original_code)
{
    auto test_cases = BuildTestCases();
    RangeProof rp;
    for (auto test_case: test_cases) {
        RunTestCase(rp, test_case);
    }
}

BOOST_AUTO_TEST_CASE(test_range_proof_get_max_input_value)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_get_first_power_of_2_greater_or_eq_to)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_validate_proofs_by_sizes)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_too_long_message)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_no_input_value)
{
}

BOOST_AUTO_TEST_CASE(test_range_proof_too_many_input_values)
{
}

BOOST_AUTO_TEST_SUITE_END()
