// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory.h>
#include <blsct/wallet/verification.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <wallet/receive.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blsct_keyman_tests)

// TODO: FIX THIS
// BOOST_FIXTURE_TEST_CASE(wallet_test, TestingSetup)
// {
//     wallet::DatabaseOptions options;
//     options.create_flags |= wallet::WALLET_FLAG_BLSCT;
//
//     std::shared_ptr<wallet::CWallet> wallet(new wallet::CWallet(m_node.chain.get(), "", wallet::CreateMockWalletDatabase(options)));
//
//     LOCK(wallet->cs_wallet);
//     auto blsct_km = wallet->GetOrCreateBLSCTKeyMan();
//     blsct_km->SetHDSeed(MclScalar(uint256(1)));
//     BOOST_CHECK(blsct_km->NewSubAddressPool());
//     BOOST_CHECK(blsct_km->NewSubAddressPool(-1));
//
//     std::vector<std::string> expectedAddresses = {
//         "nv14h85k6mf4l5fu3j4v0nuuswjwrz5entvzcw9jl3s8uknsndu0pfzaze4992n36uq7hpcy8yeuu854p0gmhq4m2u0tf5znazc527cxy4j7c39qxlc89wg4nca8pazkecx0p6wmu3pwrma3ercgrk8s7k475xxa5pl2w",
//         "nv1kq8zphgry92d02j7sm460c8xv88avuxcqlxrl7unxva9c4uawuvskx3s3pd6g3nychcq0ksy0tlpmgyt35384dnqdtudafa00yrjpcsffef404xur6cegkm98llf5xptkj6napgqk6g9dpa0x24qe4cgaq3v9scy0m",
//         "nv1s48u8dtxseguw6s7ecewph2szrxwy3fzx47rzdtppgzrnxrp0p0peetjx5h2f6gpwy3ar65tmw4p39z30pzt0t6san07th0694pffc0f6dghnskfujfanzwjdzep8fn0ezdeg7ejmvulj8nymrzkw8wdvqmhvl6gt5",
//         "nv1k34crux0s5urxtcndupcd37ehkakkz6da8n5ghmx388vfynhqa4k9zmrp8qmyw485ujvpkjwcasqhq5rqpxrkvhm0tg3ap3er8eycgwu5ew5xq5u84vzxsaqgc37ud67g5j9jvynlqacx78zl6l2flw82gvv2w5wzw",
//         "nv13qq8el3522u4jxd4e8y54du9d5fqlqlcmz8n90k8hc6e72dqky99ajgfarmd3puzx9zz9hazr99zrggharvuh9ulg9ugnu6nf5hfvq9mw03nv2g9xz9v2vnvn6uumrwxcv93ae54kuzjmz49g4mx0u2pzq2dmup8dn",
//         "nv1kh6n54xfhq0nmsr8rrqsff8xtegr8hvsdsvn2sdtk3w25w9fkescwqeqlnasm9ngcr895ycxx4ave2m5crya7hgyydhsa66ct995lrvywpgseu8cq4yjwcjm7dkh367pg3dhtxnwsfsct7my5tzu0c8jwsnddq2xw3",
//         "nv15gxjtgw289m82any2fn75gdh09cyte4c6qlzrms7wr4a4vyqdd8epl2qncrhspdflru3kcc4kdpzrrqtcvrq3qzxdjrh3l2lqr9v5jnjw22ut4axj9czcajj8pfyy0mm99n0q8088z99uame7ckrk8k3yvzc6em4d6",
//         "nv1j08knwnjcuukjl88vyt06c2h7unqjurflvtqaa9ljw08mz6swp2je7zg962u5qke9dc3cnhz3rkfdg0uhyw3zw6jk2akd08krzxqms74lcm9paapjygl3kglru3gaumy682qysl2hy6cgujqs9ugfxvqzcpmrge5pg",
//         "nv15vn8346nl5ttuu28w7dhwetq5vlu8tv3dgdqdhks769ye9gd9ssaszk5unwtejp6vftw82936k20m93sc4z9z29zz4f2rneexfw770ducywzxt3wp6vc7c3lhgxn2jxxufv74hwppcxd3prcn2yf2qgk6stntwl9lg",
//         "nv1kag0sqeuzz64stxmc5ztrafqvyx7lv4k09leasauyku5eg6zdsh23nyauzwrszyqysj02ecqmzkdrdym02w7u5y6ed7ptwe5adqyqufnqfj5hqve2et935gw8p8jculfnr66qpk8u86f35zaxs053920gs84wlan8z"};
//
//     for (size_t i = 0; i < 10; i++) {
//         auto recvAddress = blsct::SubAddress(std::get<blsct::DoublePublicKey>(blsct_km->GetNewDestination(0).value()));
//         BOOST_CHECK(recvAddress.GetString() == expectedAddresses[i]);
//     }
// }

BOOST_AUTO_TEST_SUITE_END()
