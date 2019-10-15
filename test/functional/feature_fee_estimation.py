#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test fee estimation code."""
from copy import deepcopy
from decimal import Decimal
import gzip
import random
import os

from test_framework.messages import (
    COIN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    satoshi_round,
)
from test_framework.wallet import MiniWallet

# Hard coded feeestimates.dat file with the expected resulting estimates
fixed_fee_estimates_expected = {
    "smart": "0.00113050 0.00113050 0.00082030 0.00082030 0.00079932 0.00067950 0.00056521 0.00047942 0.00039966 0.00033988 0.00033988 0.00028260 0.00028260 0.00024384 0.00024384 0.00023764 0.00023764 0.00020200 0.00020200 0.00020200 0.00020200 0.00014266 0.00014266 0.00014266 0.00014266",
    "short": "0.00190117 0.00161830 0.00116006 0.00096078 0.00082030 0.00082030 0.00079932 0.00067950 0.00056521 0.00047942 0.00039966 0.00028260 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1",
    "medium": "0.00161573 0.00161573 0.00113050 0.00113050 0.00082030 0.00082030 0.00058000 0.00058000 0.00047895 0.00047895 0.00029000 0.00029000 0.00023764 0.00023764 0.00020183 0.00020183 0.00017243 0.00017243 0.00012189 0.00012189 0.00011882 0.00011882 0.00011882 0.00011882 0.00011882",
    "long": "0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00014254 0.00008403",
}

fixed_fee_estimates_gz="""
1f8b0800000000000203ec9d7934d56b dbc74dc978a243233267ce542172dfc8
6c9ba724439290a27912a541834a224a 918cc589448542a42889642c45a24464
7a24f5f69cce7aff38d7b1de739ef759 cfd359ebfaacd5da6b7dbf6baff6de7e
f7675feedf6f6f474d58985a795899be 323131d57dfbf7cfdbdb4cff841c27bf
defa9cf87eab75f2d7db901991648db7 b737fb974822babfc75a74e014599013
ffc1a33f8a047ba8d67b7f3e4dcacd5b dc6a056308cbd6a699563a67c84821d7
eca88db1e4f195ec10979b7184f7f8ad 97fed3ce11e68ebb37ce6c89272502ea
c3373f9c27e603765a2d3a09e4ee2a9e ca3bbc89a4d867d09db33b91c454a8f2
e83cba48dc445842de14251153f6c7fe 676e5d22cc8a298defef2693cded02bb
f737a71025bb42599eafa924e648adbc 9a6a3a295f5673ebfaa60cd2da7eccd7
a9e232611e17487a2b97496412d78be8 2564111f668f3c7181ab2473d31edd81
575789d0d365425cb7b249bec80cb681 733984779da981f2916ba4cb2954c079
7f2e91f615fbc07af43ad1db65d25773 218f6c2ff556f429c9270f3b2f2fd831
70839866a79a152bdf22112fb58526f6 1490073dbaa39fdb0ac9276156df68eb
db64479b506146fd1d623beca27d2aaf 987845b87f0d395a42ac3ca578c4d695
9280e662f3cf4e77c95070e5be8b5665 c462ca7852865d3929fb87a2c2b5d5f7
08f3f489839e6115e4fa3ce5cd6bb2ef 93603bdd80edbd0fc8fe4e9939ce9a55
4455f24549cde9878425e578552e6b35 d99b28a1fd7adf6352951b79e1d2911a
c21958dceceff6845cdd72b5b34aa796 68ceb8676a32bf8e144e58ba6c147a4a
6a958d0eb409d7930c67af6b720acf48 f168e9c531a306e2986f6e3d16d848ba
a5e53cdb329a089db6fd98ef603339ff f688c02af356f28631b6e46cde73c27d
afbcea836a1b199b3a6e2ae9f192448e f7863e547d458a5f69daf7ffd44e74dc
670c0a8eb413de4e1dddcaee0e32b7f2 914c62d76b529319c8b668b093848ef0
f5c8f374118e451ac24a6add247cca4e bf7e9fb784758cf7d282ac7724635bcb
941cd6f76476b2f34d23df5ee2d2eb6b 59d5de4716399849f02fee27ef433657
9d671f20f25101e2c75e0e108feee209 f7f28fe461b14e43f2f541a2d3c024de
973d44d4057393ea6e0d13be0e93dad1 9a11e2a7b46611dbf0281990b8599b31
7f8c0c9cd97c2265ed273261689c5d50 304e0e9b7e34bc2132410caa5f5fdd74
ea0b9979939fcf979789def1693bd1de cc44a32b846725e630d3328379d2b2a7
596875e2ba6ab67dac74b6f02077f92e 361af1c26936cbbe29d4645fcd239768
769aea10fa657bfe54fa39eee96b9e2e 0eeaa0dd16fe488a8b96ac965b1cbc89
9b26e7cfeb7d57c743132d3cdade1afc 44ad563a8fbadc9f46f3c3d2b2a27fe1
a3c503ca6cf2fbf969c3c443b975aba7 d3421dc7ba4eab9fe985478c30496301
cab86d3370c74c90d61a4d3838ac9841 e36f3bb41cdb31937e14983b28903a8b
565d9cf6f062fb6c1ace965138aa3897 ba8a98df7c1e2e4477dd5cf158665498
de2d3d9892bf651eddb85d8bed569828 b57d98942feb244625952f3c925f244e
8ba5ba628ce649502e86b902b78024bd 2d6255c02d28458d45057a3ac5a569a7
58fda2a54be7d3ca6805db923532b46f b17d8c64822cb513b8e627dd2d473f06
b4174510059a333f6ac7820c45ba7324 94b7536a01655ef27aa3d972652a13b8
8bfd9cbc0a8dd73f57b76aaa2aadfad9 eb894c9f2a7d20909eb6f5a51a650fcf
e67afd429dee58abc5cdf96e219d6f3f 111cceb2982ef2ee5fc927ab416dfd76
295bafd4a4665c6f833892b4284b309d fe797409dd3b3ada36eeaa434de44ef2
b7d72fa571e76adb4d9409ad3f7cb722 e42ba18a3706d9049a285dc0f0d63854
a4473bcf9f4ff4c9d4a70dd2215b8cd2 0de8e9617feddbd9cb684675a3544085
2155e339bc89a9c788ae952e7b292d6c 423f94c44d39b7d2945e34fba556e2aa
197db2f93cebd1e91654c8d33bfeca01 06dd3bc8a199cb6e454d6277d53fabb3
a2cba78e282cb96c4d6dc41e1ff23a66 43a3773595b6eeb0a57e2bbc62d537d9
d106c52d53d976d8d34293f589ef8e3a d0da8d8c91e3998e94d5bd7638a9d589
fa06e7b9b4cd594e25fccc45d9d6bad0 9ca4bcbee7152ba871244fa29ec64a1a
1b7bad747a811bd5313dcf139fe64e8b e378e634eef6a0a6eb8b430b5d3da9d2
858317b3f476db9db5bf7039b3f1832e cbffca1ff94bf833f1d1df670fbfe1a5
c40f72b9ddb6222223d2749a80659a4e f86cd06b09310e714baa81dc2682b19d
df5b9e46a7179ddf35451af4fe0afdd5 16ba4b405efd60ca9df13175da1070d6
a0c1480af4161f42bd2fb95290b78a94 1698b96bd389add35a47a628c1e777c9
da63a1ac017d3dad5a744bbf32e897ca 1b273c50b402f91d5f565d0949639a93
21b0d4e5d822d00f0904f9ba8939837c bee647bdf5bd36d4f2496fd6144902fa
7b0f0ebe99a1e101f2b0a8170b5f043b d353fcd72d1b3bcc417f72aa7bf0190b
4f90abb8ce7f6fdfe4490fb38d6ea8fb 620ffa4fa7741f5dbae14f273b268672
dd4472b983405fa725b9c126c28b7296 159d28927303fdb68b05b5758e3b413e
d27024f7edc24dd4f2ca9cb092adf0ff 9dbf4cf94dc4a33090c764bb2609a46c
a3da0686bb79423680be3d56916d25eb 6190e72f9e5b71c6632f8dac6bbe95c4
b11df41f9f0f3936cc8c04795e683fa7 c081701ae5f4b30d77fd5ef8f358bdd5
84bde8e4a4afdbe097b395cd7c5114ad f29f0185ffaf10b28c800598107ef5d5
5a7d904f8b0c0f0bf3742005226abd7d fac6a0cff17e20922aea06f272bf8494
da0e1b12712a4cee5aac05e89f713b7b b05bc3fbe56b8c3fb8b8d98e3c4d2fcf
0ad43000fd3c8e0ba2c92a2b402efe55 d3ddf1842d518e953c66a300ef17c7a5
9310e6ee48d4de7d62ea9280cff3ea96 057971daf0f11c572a5bf373952dd16b
daadf9944d0ff41d7333af87aa78803c fd8aeaf3651fed094fb656c01b6d78bf
65ddc7773027b9837c8c16b6bb74d911 914f463d450246a067bb5b21e6c4e20c
f2eee0d4e0c8667b22cc3aeb8a7033fc 39156dbefe8f9e9b2e64b243a2cbf3c8
1a2d2357d01b0e696b7c7634271ed37f 3270e987afeb4cadba73cfd7c1d7cd7a
9a8af5ba5c5bf2bca54ec899c70cf41b c373ca2e77c0e75f9b1fda62ffc58ae4
583d1d6c913704fdc3dac5bb96bf828f f39e22d7a9e11e6b922530744dfc017c
dd9ea6658d4d2c87f7e3afb4e6ac3963 4dba462a4dc51926a04fe0f40bd03f69
37e9eb66f1f47cebbe6fc71d4ae53fc3 2c34fe7f995b7beb17c4680982233e5a
e59cf448b218a1ed6723bd348440ef66 28281124a806f2a98cf5adc5d512448c
45ebd0c3cbe270c577c927848543836e bf32eff50b132d1299d82e7fdc7831e8
7786aa94a9332889183e56c49ea502fa 45aefc1c4a4370c5970fe6b22ba81990
e11b367e9f7516817e94d1b4e7c42a1b 900b1c68a96e703323d5e1dc615c8e3a
a03fa5afaa451cec416e655325fecb29 735296e2dca12002df99425dd36d1543
a1612a05658b2a996dc9215b8f11b518 684a6e2b8db2a31dce939ae920537966
380f1a1f8d8f233ef28d3922276ecb94 4f050776b1d8f53415261990bff44a39
abcaa7487acf303f0e089f0f1744febc d5161c50687ecb5737c8e4a81246414d
bdc31c55d0734a7dbaaf510817f46757 9b9cd5ab08592bf2a83d545913f44bbe
2cac752d32241e01a2ebd61f50870bfb cb8bfda71bad40de6c2191d8da6f482a
66c73fd513d300fd56c170e1cdea8e20 97f8e4fb6a91bb25c9bcbaaa57a24317
f4f103ad5925254e209ff5892b6737bf 15e9ecb5c8d4d387bf8a452ccdd63a92
eb88233e0a1f858ffc59fa757f9f7c3f 33f215e4a1a7f58ff877fd4ca266bc8f
785127060eece72ccc8b2d7ae0c4bccf eaadf8893a4d72760e577faf2514ecf5
3cdfbbdeb1504c219e0b856f2fd4221e 392f3ccc3817c237189fb35ffd8bcc41
9e643c532e4b489f54eed355e86b8513 758cb686f3c74d2624a446fe9082127c
43e0bbe260c470800bd7a62dd67f96c1 b7c779f96890c28816e83d0318eb4b73
97839cd7a0bc222ddc86f41975279fb2 a0a0ffaa3d5bef42061470aa7f5ca752
852d39562e725020de10f77450f8080a ffffbd15462613fbef73f5907cb2b77d
26f93ee1c349dd76744599c7366d90cf 55f312b45a49898372b35cea1528ca48
d9fe437ecc0c90c75aa944795fa0930a 3fde8fc54328104ee23147db0e3e4d35
241798e43a5e96c189fafc1bafaeb487 0c22596ad535f16229e85da24a391da3
e1e43ce5c06d56c6264b5215fd8f297e fbe11b49eace9da10b9c5642012b8a04
4684db91ddbbf8b974caa1f0db542fdd 676f83a25ccb7b2a38673d6ee2a3f011
14febf0d497080b61c9e583dfa5a0ae4 bf9ecd7d2b46e24585b6d36259d0df9f
99d67d870b0a4d9e456aa842499fd45c df70b4d8080af8f3bcbc9f66bac0bd6f
0bc375ddacca8644c8a12ba0a705eec5 f3f4f12e2e580117e09bed45e7225219
447af71e995595f0f1cce59415921db6 21137dfaafcc96c2dee8ccfadbfe1a50
885696eb5c38836c482adf015fb73802 faa88cca253105505c78d616858fa0f0
7f10a4c1014add3b6bc45b142799fc25 48fd4cd5f7be4fe09e7a4538c36c6818
2ecccd2e3541e9db0c4972e452c5b79f a0f0c41730f3d4ec87272df5bb3666b0
33cc886e88f2c9d3779780deb243f18d 56309cc44b14d40e954efd26f4b1b56d
f639f0dcc081a53c267189f6935ea653 5662fe4cdc034eea78990e0a1f858fc2
ff9ba334c9240ff7e2233607cee57d29 fbdb960e9cb80797aa37f717c03d75f3
9d161162c74c88fa68d3f4e152b87037 ac5a9e6276009eb4dc3cb5c0658b0483
5c330bf41cb78613f5d6555b449493a0 f0b62add8e1e70c2eb3251f8287c143e
f27f0afffb240f4fae16a60425a475ab fcb6a5034f760e6c3dc87d24db124ee2
0f38521a9dcc494d204743f60228a0ea 57b58e67f2a180061a1fe56671d8102d
654e267333b8f5c2dfd4fe68da10bc1f 5e888fc247e1a3f0913f046ecd1cdcbf
bcaeb0034ef04541cbf57b9ab42715fe f6446ef7a1bd702ffed8bacab8802556
e465e549d6e874b890cc0c5c973935c2 05281ff83573e8b30db12857e417d385
62c64f5ea1f051f8287ce42f013f40b4 6443f5a1da1770cf9c25c765a8632d21
6e927d27dedc8057e3b8659e1f7ebd07 1ef0d196c6c93e69d6e4eeb21eafb178
b8e563fbe01796ad7570cfbcdf6ea2c0 eca62d7ed416858fc24750f8ff1eb4fe
b4f07324cbf63f79a347daa577987dbd 062f671cd178b93148072e08fc6e0514
3e0a1f41e1ff10684f227c9d49859f23 c0ca7ad4119e44d5d21dafb71847e1a3
f051f8080aff07058a3d6af5a155aead f07a79ce8b41daeadefa6448d0c47d96
3d3c895a551c69aaa3e180c247e1a3f0 1114fe8f09dc9a11bd1ae473721d9ce0
bf7f5da6e16fd7e1437115d28327bc47 50f8287c143e82c2ff418193fa5645b1
7943efa1f0f3f7ea8fc73d372133666f 3319ad86a2bc52baf3d9f13ff8322f14
3e0a1f858fa0f07f08f4ffb4f0bd7627 f8b8b79a92bcd973b71559c005cfb83f
c8993c88c247e1a3f01114fedf46f892 32c1476ff6c0c97ff9f8804578b31989
9dcbc6c65709bfae579f5164271ce484 c247e1a3f01114fe8f095cb84fc45bc6
82df413179f47044373d63104da154d5 67c670017630dcf775f4a3f051f8287c
0485ff6312f2e7857f3d5d90393dcd9a ec10ee3dee69081712bbcd71ff525f17
143e0a1f858fa0f07f4ce1fff93f6a7b 534c4cea89b4eda47fd4f68873bfbc9c
b42b0a1f858fc24750f87f77e1e73bd5 17f04ada4f2afc0dabf795cc915889c2
47e1a3f01114fedf5df8dfafc3779854 f839de0f445245dd50f8287c143e82c2
47e1a3f051f8287c04858fc247e1a3f0 51f8c85f60161a1f411004477c044110
04858f200882a0f011044110143e8220 0882c24710044150f8088220080a1f41
100441e123088220287c044110143e82 200882c24710044150f8088220080a1f
41100441e123088220287c0441100485 8f200882a0f011044110143e82200882
c247100441e123088220287c04411004 858f200882a0f011044190ff2e9e7c4e
9ab9f7fb749951feff1afe4c7cf40f8b 5498bfe3cab30e9492a6057d99879c5b
6782be7c5f63d0624b6590af3972a171 7a9d28e5ad0e19546a1201fd1e925cdd
19a902f2e484f4a29878599a96b030dd a54a14f4564f59de5eddb410e4bd515f
07b5321428bbb184cec65712a0dfe2a1 e95fe7a34e59c7d2db66df9302bde961
85f75dba14e4738458f7d73dd4a25143 223bfcb6c8833ea03347cd2ade04e4ff
d3de9d4753bdff7b1c970a21999aa936 a1682322b3cf1785cc4528a4a452a248
a632f42b0d683637494a9a2825a7494a 87e2644845a306a73a51a114e2b7d63d
ebf7cf799fbdd6b9bf7bd6fd75d77d3d fffcbcd776ecefd91efbe3b3779b4a8b
ef9e84028eabf5d6dd5ea9a343af5b4c 477b6a950b59bf68b562ca881d36dc83
562b3db70b66645eb9ddb4f8662fbddd d4290632277b9cb941d75a960f1a6c4d
e603a7c5be59bdde8b13f4986879e152 b7ae6321998bbf8fda315ed585b30e70
ef9fa24cffbb964e4faec5280692f535 6ea9eac97b167223c6ac99dd3ed397cc
23c3fb22ed1f879275abc2945c51bbc5 dc36f78471bbdff891b9eee87b4687b8
48b26ebb5044bcf75030f720766fe782 f5f4fbb13b67bfacc47203599fbcfa88
ef8bab6bb9c0fe85a6e73ac3c8bca440 25d4452146e07593ee0d8b6cad8ce7a0
0a76fb3f6e7133d81f97a66fe57c5fd7 58907549fdb071c3d3dc58d243b5b845
27679279f7a2a50d6edbe691f551e1a6 37beeb58b3ecf712b7932e5991f91c2e
f075b3a62b595fa3fda9fc5b8f0db354 78ffd2f39d399947bdb2ba27bcd485ac
1fb82979caf98625b3b23e50ec5d6c44 e66e15221936f9d66cbaf9f144cf4c03
32cf6c0a5a1b9aed4cd64f9eee4dd1f8 368335173df59bf6498fccdf6d4e7ff2
329e7e3fcf9717dad56eb062cf94bf5d 1dbb731a99efef9bd478419ddeaec822
cfe18692159319609810b4957e9f72ed b5d33bb7da90f584d2336d6505964cc6
f665dd7c617aff254725660e08b36382 1e1239ce93f246cb3b90b9eda31433cf
dce9ccb0faa5e63391e964be52ade6f3 17157addbe5f5d6dff4989634336cf38
ebd26646e64f131ef8b331f476879327 f0347b8cd95ec9e64cab527a3f728f1d
a92aecb227eb53b43afc77de376377bc 3b146ec81893b97746bd924c0abd7fba
aead3ff5789933db72dbbbb971a6647e d756ed55e31a4ee075db31e3568baa87
e039fa7b9384f8ffe132fcbf65b6bdf9 6efec775d58f66251b634599d442b7e6
a419a2e427e2e28cd697175c64c83aaf ec98e758595156ed324f2cf4ca302a2c
ff7df4b17c5db29ef272ce30ff2c35d6 bec939225d5b83cce77595be2ebaa6c5
5e8d6bbf10eaa0427fb21585740f7453 61f94bce46c728ea33d3295e56d1d6f4
ebea2ccbda59e8c9c8fabe42cbe5b525 facc6c44c11ef10c2d323fad7974769a
88251536aee6d755bba6b3d2339139e5 4775e833db21d5b57177a830af79cdc7
f7e49831f5a196d6bf79e993b98c78ef f6c2ea9902658a5af18a8b8ba2cf080b
ca86cb8f2dd067ce457e83b265e8d75d 6ae17bdfb1d609e2236cf1ff3f543ef8
424ad0f301740b7fd1444ed478a840f0 f73ed06e79f87d22599fe1e01ec31395
67fd0e566559f74791b94dcfca006d23 bad51c9eba6944c5250dd63024537866
299f6ea9a58e84d728ebb29bd24ed3ab b254e91346efb751b5974dc87a71d50e
a10badd399b36e7195d47d0abe869ce7 1cd75df457a35203cff2ab72864cda66
757284fa54323f95752a58da8002ecb3 a849a868b709ab1edad4fa558afe4ad1
e07fac58ee0a7da2b8c0db2eb6db8163 67e615de723847afcfc5a4ac3769c1b6
02611a6e203ec1e79e3db6f8001fe023 c11d761e3e9f377c3879c0d7294ebc53
a133925da89093922e93a53b75fb7b76 2c9e4298faeed7e7de0ae35976e2ebf6
fbfcf164defcbeb565ea4e0a81caf4f7 99fe857c36ac4231be7e2cdd51af2e53
b9d25fa2c736778a19ac37984ce6674f 6625da6da36748e1cfe2ae858d341208
bea8648097e3760ab7f3fe73e9d1cd86 ecb7abafea2583e9fde41c5557047ad3
b3999a5edd3889df4c99c43f1cba7daa 29f879aa36ae7da2f44ccefea0c6605f
170b7624daafb19a1992f9cba52bd3ae ebe04c07e0037cf417aa28d24f6ed513
210f6c07d9ac7d4bf215c9faba67c725 a52ac6b3decaa43673de68328f791633
b63f811ea1c45d5a7cd63a9ac7fc9a66 cf9d2f4481cdeeb29977793d0564d590
e0ea86c75399ca8d9ad802636d321fe9 b94dccfc9d015bd2e27fb121419dcc3d
936beb6c0ed29dfae5dd1fe277f51b31 d67ea97caea726991f9dc85ef0e529dc
1d5f1e5a473598b09c93777cf5be52f0 5b6da73da9eca03bee8e8e100fc53673
d679ddd568772a3d4259d6ec78c036de 9aace3101fe0037cf497fbfdbd4f7de6
82d6dfd65d9b98f296c2bd583bbad8f9 1735b2de15696a343564123badf1c451
7c0b8f1ea1d86d5e71bb8a1ea1e85bdb 8a37d8a931b331a90ef9f614585db999
a737f7d2337379c3578e2b36e90a04ff 49519663b486117b726081f90709fa1b
40af5fe3fac607f4a824bc4c3bceef90 314bf7f05de01444bfee6e1fbbf667c9
14eeb6da9d574c9dcd596a0e9f7fa096 9ec5876ef24a78933f8bfec6a026f2da
671fc79a85bca2ca175060f1aa2dc007 f8e8bfd947f3bf0abea9f8839c55a672
2c55f4ede490bc09e4811dbcb6619c72 2a3d22e9cb3978c5f9b5164b968858f3
ca96ce57ed8a1c29739e1ea1a8092fff 694dbf0673dfe461923280027b544561
d52631ba13df23242eee50338d0d2cac 8cad37a72feace8df9eebcb6da982db0
79fa4c994fbfaec781146d591b7a54b2 ad3251ac4bd384b96ec9f6e60da33bf5
94414ac2ab7a28dcd636722137ee3176 5a59e15cc812fafdd405b96c0aba4de1
2a947032fbd867c1bae366096576027c 808f00feff3cf6e7e04b91f55992c21a
9b478c64c52a21d7477bd117594b9e98 3e1b7f9a42782bddfbe704051dd61264
7928a39c1ecd84141fd91056437f5078 5f94c215bda70a047ff87aebc8d20e0a
7e7081dc75db8bfa6cc24e09f3cc560a ac429354da592953a6752ee0509e12dd
71ef93daf9b0c6939ec52fe5f155c3e5 19fb28ab9edff78ade4fcd33baa5e961
1492f757a35b765673ac2247dcd0ba87 7e3f817d76d5430750f0b66f28dbb8d1
d212efcb04f808e0ff5dd177bffc6b27 2f08fcdfcff0e9bb5ba6cd2d0e49e4d1
1717c307fe23af56661a93195e9814ad 49e19e5ce22a1e9d4fe1f67b946b78ea
918e40f0afc87405986ca56fec6f0d37 5ca731d08099043bf4f966d137be9f5f
9b5da41bc098f0b8cfbefc51743e5936 255b3d941ecd34160a6d7f57cc09043f
eadc4809cf7647b2fe3143f86cc1590b 81e04798594a47d851f032cb4a7e6e4b
06f8001f01fcbf2d7a34639918f86bcf 6415013bff71eca1658fc7f56193c83c
5f477bfecf2614a65743c2a5f3c5f559 71cedbb6c02d14ee3b092587e28b29dc
fd132f477e79a4c7f40b43b6969ea1c0 c6368e9fd6d54777e2ee0f8e1cc93430
6511fcda756d5fe813d0a8fbc1bd8979 164ce2ddf7cd870ae8bc4c6e8e63e26c
0aa2a171e4cadb5516eca4c58abc9a4d f43703796e79859c21852b40b8a9af34
d312fff20ae02380ff9f6ee25f06ffbf fe09ae334f20f8cb369bf1cbbb04832f
257a7e8cd220bac39d9c59cd537941df 6d327fe0e5d47d3a064c7d92ff86fa7e
0abe18bf22a5ca9eeec4bf69869c5ebd da9c15fee6da133d9bc234faf40be50b
972d59d8fc0fdbf2be53f0e70da97c93 ed4741b8993db77dc07e2b7646656cc0
55290ab791e5e3d6d02e0a17fea92dc0 4700ff8705df39392d4876e06481e08f
8d1bba453a9cbebb45365bcb56d584be f8baf7e990a695530cd8cf7d9d23ef0e
a16f2f5c70698fe5f9140a77a046b14f 86bc098bd811f3e9782e7da2d8f24be7
c4887bf445d29ea73ebb3307724c6ceb 18c39af9f47de84b37140d4a7b65c566
de8ab2e37750b8926cbf297d89a44733 de01173f986c17fcd90a226ac98132ab
f0d90a001f01fc1f367a16efb5a3ecf3 a4c753c8faef67fbca6c9cd39d615e29
74c7ed61f43266de63fa807fc7350c56 ce3362f99396dd78ab44e172d2ab7dde
3d8b02242e313b3df08b19f3d79ab0b2 d6ee4f8e5e465f902cf2a33ff0b15f53
ea6fc6593095655e5f53875028eaee8c d2aa9c3153e087e98408ddb8ebe18e0f
d301f800ff470e1fa6831042d8e22384 1002f8082184003e420821808f104208
e023841002f8082184003e420821808f 104208e0238410c047082104f0114208
fd9feffc18d92e8fee0fe6a380ffbf57 a09034f7c73533f107396ae632643dea
6472adb0b41a57755caedfe3da68324f 16dff6755d862e59ef345879f9ec423e
773ca7c0adc1408dcca71deee6358a9b 92750fafcb7a0d5bf4b9b45b4eb32aba
55c85c315dabf2e32e0bb23e68798bbc b68c3937a0d8ea8d49911699cf78ae7c
30557e26a7e75d9490fc652a991fd18b ff60b3cf85acf73ff3bd5bac3e8b1bfc
a63a69df0403323f11fe3a3261d37cb2 7eae74f7b7efdf5cb92d17e72abc58cb
91f9dec7cf2bf7372e22eb978db23fd7 f0bdb845bce87b85614e649eab553f27
4d6d31594f141fa6a3dfb598936adab8 57a2d79dccd7df3f91f934359813f498
68e0d9eff8da144ae693448da5d2c72e e5ea77b5dfaf7ee14be6191e697196df
d693f537b79b4acd72d67286fdd287e5 8f0791b9da3061f92d429bc97ad972f7
66d99fa239e9349fddfcbcd564de179b f4a9fc541259b74ff263fb0f6ce48c87
3eaa741f184de6e31e967caf78bc87ac 0f78142f5d1799c4f5aa57dd8a34d848
e6c736b6d97a37ef1178dd6c37c8040d 5c92ca4115ecf67fdce2e81f7215d1ea
16af5b47ffa2b7c53d9bf2b2e8b9cc51 4da4cf239c7e64e37587b142bf3c5d40
d643ef9504ffc2b9b2fc8a88a63a7d27 32b7f2d67931b8d097ac7f905f5c71ac
db8d1d35723b18f182feb594bd9d010b 26aaf990f5b9ac3fcd6e851b1b7af616
2fc881debf8e805c739b4c4f96e2a1c2 97b4a75f375145debf5e6821593f7e78
f2188b356eac51c26c928835bd3efefc dc3bddf71691f57333fd6463677b308f
a0ab1ab72fd1dbed3d31786bec747abb 88cb9e6d2963dc99fd4f5a4f9d95e867
49ef961b5fb926713e597fa4e523d2e0 e8c16cf4f507478ca61f9119789f3f2e
328e5eb77fb5f3f3890e2705faffa337 34fe41e9632726ad2959ed5b40ff0a4d
bca2efdd40237adda25235d2f564e732 d52d37d24ff3e847562e2dfd20967e8a
deffb7358546198a73d8a0256245374b e81f4f68dbe676626d24fd3e5dfb0aef
1e1de3caf4adaaafecc9a7d72dac69d6 e4d78af476fb9e5b953dcc9ec3c276d5
f75dd2a0d7adcf61d40ea9a1ee02afdb 872e9f71d7efbb31a0f2bf933ac4ffb7
32218fd0ad33c3a58d7be887c5ce6db4 105fd86bc18605a55a4c896264aec673
57ca3dec06f1213ec447d8e2638b0ff0 013ec047001fe0037c800ff011c007f8
001fe0037c04f0013ec007f8001fe023 800ff0013ec007f808e0037c800ff001
3ec007f8001fe0037c800ff0013ec007 f8001fe0037c800ff0013ec007f8001f
e0037c808f003ec007f8001fe023800f f0013ec007f808e0037c800ff0013e02
f8001fe0037c800ff011c007f8001fe0 037c04f0013ec007f8001fe0037c800f
f0013ec007f8001fe0037c800ff0013e c007f8001f017c800ff0013ec047001f
e0037c800ff011c007f8001fe0037c04 f0013ec007f8001fe023800ff0013ec0
07f808e0037c800ff0013ec007f8001f e0037c800ff0013ec007f8001fe0037c
800ff0013ec007f8001fe0037c808f00 3ec007f8001fe023800ff0013ec007f8
08e0037c800ff0013e02f8001fe0037c 800ff011c007f8001fe0037c04f0013e
c007f8001fe0037c800ff0013ec007f8 001fe0037c800ff0013ec007f8001fe0
037c800ff0013ec047001fe0037c800f f011c007f8001fe0037cf4e7a9437c84
10c2161f218410c047082104f0114208 017c841042001f218410c047082104f0
114208017c841042001f2184003e4208 21808f104208e023841002f808218400
3e420821808f104208e023841002f808 2184003e420821808f1042001f218410
c047082104f0114208017c841042001f 218410c047082104f0114208017c8410
42001f2184003e420821808f104208e0 23841002f8082184003e420821808f10
4208e023841002f8082184003e420821 808f1042001f218410c047082104f011
4208fdc8fd1334899223b1c80300"""

def small_txpuzzle_randfee(
    wallet, from_node, conflist, unconflist, amount, min_fee, fee_increment
):
    """Create and send a transaction with a random fee using MiniWallet.

    The function takes a list of confirmed outputs and unconfirmed outputs
    and attempts to use the confirmed list first for its inputs.
    It adds the newly created outputs to the unconfirmed list.
    Returns (raw transaction, fee)."""

    # It's best to exponentially distribute our random fees
    # because the buckets are exponentially spaced.
    # Exponentially distributed from 1-128 * fee_increment
    rand_fee = float(fee_increment) * (1.1892 ** random.randint(0, 28))
    # Total fee ranges from min_fee to min_fee + 127*fee_increment
    fee = min_fee - fee_increment + satoshi_round(rand_fee)
    utxos_to_spend = []
    total_in = Decimal("0.00000000")
    while total_in <= (amount + fee) and len(conflist) > 0:
        t = conflist.pop(0)
        total_in += t["value"]
        utxos_to_spend.append(t)
    while total_in <= (amount + fee) and len(unconflist) > 0:
        t = unconflist.pop(0)
        total_in += t["value"]
        utxos_to_spend.append(t)
    if total_in <= amount + fee:
        raise RuntimeError(f"Insufficient funds: need {amount + fee}, have {total_in}")
    tx = wallet.create_self_transfer_multi(
        utxos_to_spend=utxos_to_spend,
        fee_per_output=0,
    )["tx"]
    tx.vout[0].nValue = int((total_in - amount - fee) * COIN)
    tx.vout.append(deepcopy(tx.vout[0]))
    tx.vout[1].nValue = int(amount * COIN)

    txid = from_node.sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)
    unconflist.append({"txid": txid, "vout": 0, "value": total_in - amount - fee})
    unconflist.append({"txid": txid, "vout": 1, "value": amount})

    return (tx.serialize().hex(), fee)


def check_raw_estimates(node, fees_seen):
    """Call estimaterawfee and verify that the estimates meet certain invariants."""

    delta = 1.0e-6  # account for rounding error
    for i in range(1, 26):
        for _, e in node.estimaterawfee(i).items():
            feerate = float(e["feerate"])
            assert_greater_than(feerate, 0)

            if feerate + delta < min(fees_seen) or feerate - delta > max(fees_seen):
                raise AssertionError(
                    f"Estimated fee ({feerate}) out of range ({min(fees_seen)},{max(fees_seen)})"
                )


def check_smart_estimates(node, fees_seen):
    """Call estimatesmartfee and verify that the estimates meet certain invariants."""

    delta = 1.0e-6  # account for rounding error
    last_feerate = float(max(fees_seen))
    all_smart_estimates = [node.estimatesmartfee(i) for i in range(1, 26)]
    mempoolMinFee = node.getmempoolinfo()["mempoolminfee"]
    minRelaytxFee = node.getmempoolinfo()["minrelaytxfee"]
    for i, e in enumerate(all_smart_estimates):  # estimate is for i+1
        feerate = float(e["feerate"])
        assert_greater_than(feerate, 0)
        assert_greater_than_or_equal(feerate, float(mempoolMinFee))
        assert_greater_than_or_equal(feerate, float(minRelaytxFee))

        if feerate + delta < min(fees_seen) or feerate - delta > max(fees_seen):
            raise AssertionError(
                f"Estimated fee ({feerate}) out of range ({min(fees_seen)},{max(fees_seen)})"
            )
        if feerate - delta > last_feerate:
            raise AssertionError(
                f"Estimated fee ({feerate}) larger than last fee ({last_feerate}) for lower number of confirms"
            )
        last_feerate = feerate

        if i == 0:
            assert_equal(e["blocks"], 2)
        else:
            assert_greater_than_or_equal(i + 1, e["blocks"])


def check_estimates(node, fees_seen):
    check_raw_estimates(node, fees_seen)
    check_smart_estimates(node, fees_seen)


def send_tx(wallet, node, utxo, feerate):
    """Broadcast a 1in-1out transaction with a specific input and feerate (sat/vb)."""
    return wallet.send_self_transfer(
        from_node=node,
        utxo_to_spend=utxo,
        fee_rate=Decimal(feerate * 1000) / COIN,
    )['txid']


class EstimateFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        # Force fSendTrickle to true (via whitelist.noban)
        self.extra_args = [
            ["-whitelist=noban@127.0.0.1"],
            ["-whitelist=noban@127.0.0.1", "-blockmaxweight=68000"],
            ["-whitelist=noban@127.0.0.1", "-blockmaxweight=32000"],
        ]

    def setup_network(self):
        """
        We'll setup the network to have 3 nodes that all mine with different parameters.
        But first we need to use one node to create a lot of outputs
        which we will use to generate our transactions.
        """
        self.add_nodes(3, extra_args=self.extra_args)
        # Use node0 to mine blocks for input splitting
        # Node1 mines small blocks but that are bigger than the expected transaction rate.
        # NOTE: the CreateNewBlock code starts counting block weight at 4,000 weight,
        # (68k weight is room enough for 120 or so transactions)
        # Node2 is a stingy miner, that
        # produces too small blocks (room for only 55 or so transactions)

    def transact_and_mine(self, numblocks, mining_node):
        min_fee = Decimal("0.00001")
        # We will now mine numblocks blocks generating on average 100 transactions between each block
        # We shuffle our confirmed txout set before each set of transactions
        # small_txpuzzle_randfee will use the transactions that have inputs already in the chain when possible
        # resorting to tx's that depend on the mempool when those run out
        for _ in range(numblocks):
            random.shuffle(self.confutxo)
            for _ in range(random.randrange(100 - 50, 100 + 50)):
                from_index = random.randint(1, 2)
                (txhex, fee) = small_txpuzzle_randfee(
                    self.wallet,
                    self.nodes[from_index],
                    self.confutxo,
                    self.memutxo,
                    Decimal("0.005"),
                    min_fee,
                    min_fee,
                )
                tx_kbytes = (len(txhex) // 2) / 1000.0
                self.fees_per_kb.append(float(fee) / tx_kbytes)
            self.sync_mempools(wait=0.1)
            mined = mining_node.getblock(self.generate(mining_node, 1)[0], True)["tx"]
            # update which txouts are confirmed
            newmem = []
            for utx in self.memutxo:
                if utx["txid"] in mined:
                    self.confutxo.append(utx)
                else:
                    newmem.append(utx)
            self.memutxo = newmem

    def check_fee_estimates_file(self):
        # stop node 2, update estimates file, restart
        self.stop_node(2)
        estfile = os.path.join(self.nodes[2].datadir, 'regtest', 'fee_estimates.dat')
        f = open(estfile, 'wb')
        f.write(gzip.decompress(bytes.fromhex(fixed_fee_estimates_gz.replace('\n','').replace(' ',''))))
        f.close()
        self.start_node(2)

        # collect estimates for 1-26 blocks
        all_raw_estimates = [self.nodes[2].estimaterawfee(i) for i in range(1, 26)]
        all_smart_estimates = [self.nodes[2].estimatesmartfee(i) for i in range(1, 26)]

        fixed_fee_estimates_actual = {
            "smart": [x['feerate'] for x in all_smart_estimates]
        }

        for y in ['short', 'medium', 'long']:
            fixed_fee_estimates_actual[y] = [x.get(y,{'feerate':-1})['feerate'] for x in all_raw_estimates]

        delta = 1.0e-6  # account for rounding error
        for t in fixed_fee_estimates_actual:
            for i, (act, expected) in enumerate(zip(fixed_fee_estimates_actual[t], fixed_fee_estimates_expected[t].split(" "))):
                expected = Decimal(expected)
                if abs(act - expected) > delta:
                    raise AssertionError("Estimated %s %d-block fee (%f) mismatch against expected estimate (%f)" % (t, i+1, act, expected))

    def initial_split(self, node):
        """Split two coinbase UTxOs into many small coins"""
        self.confutxo = self.wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=[self.wallet.get_utxo() for _ in range(2)],
            num_outputs=2048)['new_utxos']
        while len(node.getrawmempool()) > 0:
            self.generate(node, 1, sync_fun=self.no_op)

    def sanity_check_estimates_range(self):
        """Populate estimation buckets, assert estimates are in a sane range and
        are strictly increasing as the target decreases."""
        self.fees_per_kb = []
        self.memutxo = []
        self.log.info("Will output estimates for 1/2/3/6/15/25 blocks")

        for _ in range(2):
            self.log.info(
                "Creating transactions and mining them with a block size that can't keep up"
            )
            # Create transactions and mine 10 small blocks with node 2, but create txs faster than we can mine
            self.transact_and_mine(10, self.nodes[2])
            check_estimates(self.nodes[1], self.fees_per_kb)

            self.log.info(
                "Creating transactions and mining them at a block size that is just big enough"
            )
            # Generate transactions while mining 10 more blocks, this time with node1
            # which mines blocks with capacity just above the rate that transactions are being created
            self.transact_and_mine(10, self.nodes[1])
            check_estimates(self.nodes[1], self.fees_per_kb)

        # Finish by mining a normal-sized block:
        while len(self.nodes[1].getrawmempool()) > 0:
            self.generate(self.nodes[1], 1)

        self.log.info("Final estimates after emptying mempools")
        check_estimates(self.nodes[1], self.fees_per_kb)

    def test_feerate_mempoolminfee(self):
        high_val = 3 * self.nodes[1].estimatesmartfee(1)["feerate"]
        self.restart_node(1, extra_args=[f"-minrelaytxfee={high_val}"])
        check_estimates(self.nodes[1], self.fees_per_kb)
        self.restart_node(1)

    def sanity_check_rbf_estimates(self, utxos):
        """During 5 blocks, broadcast low fee transactions. Only 10% of them get
        confirmed and the remaining ones get RBF'd with a high fee transaction at
        the next block.
        The block policy estimator should return the high feerate.
        """
        # The broadcaster and block producer
        node = self.nodes[0]
        miner = self.nodes[1]
        # In sat/vb
        low_feerate = 1
        high_feerate = 10
        # Cache the utxos of which to replace the spender after it failed to get
        # confirmed
        utxos_to_respend = []
        txids_to_replace = []

        assert_greater_than_or_equal(len(utxos), 250)
        for _ in range(5):
            # Broadcast 45 low fee transactions that will need to be RBF'd
            for _ in range(45):
                u = utxos.pop(0)
                txid = send_tx(self.wallet, node, u, low_feerate)
                utxos_to_respend.append(u)
                txids_to_replace.append(txid)
            # Broadcast 5 low fee transaction which don't need to
            for _ in range(5):
                send_tx(self.wallet, node, utxos.pop(0), low_feerate)
            # Mine the transactions on another node
            self.sync_mempools(wait=0.1, nodes=[node, miner])
            for txid in txids_to_replace:
                miner.prioritisetransaction(txid=txid, fee_delta=-COIN)
            self.generate(miner, 1)
            # RBF the low-fee transactions
            while len(utxos_to_respend) > 0:
                u = utxos_to_respend.pop(0)
                send_tx(self.wallet, node, u, high_feerate)

        # Mine the last replacement txs
        self.sync_mempools(wait=0.1, nodes=[node, miner])
        self.generate(miner, 1)

        # Only 10% of the transactions were really confirmed with a low feerate,
        # the rest needed to be RBF'd. We must return the 90% conf rate feerate.
        high_feerate_kvb = Decimal(high_feerate) / COIN * 10 ** 3
        est_feerate = node.estimatesmartfee(2)["feerate"]
        assert_equal(est_feerate, high_feerate_kvb)

    def run_test(self):
        self.log.info("This test is time consuming, please be patient")
        self.log.info("Splitting inputs so we can generate tx's")

        # Split two coinbases into many small utxos
        self.start_node(0)
        self.wallet = MiniWallet(self.nodes[0])
        self.wallet.rescan_utxos()
        self.initial_split(self.nodes[0])
        self.log.info("Finished splitting")

        # Now we can connect the other nodes, didn't want to connect them earlier
        # so the estimates would not be affected by the splitting transactions
        self.start_node(1)
        self.start_node(2)
        self.connect_nodes(1, 0)
        self.connect_nodes(0, 2)
        self.connect_nodes(2, 1)
        self.sync_all()

        self.log.info("Testing estimates with single transactions.")
        self.sanity_check_estimates_range()

        # check that the effective feerate is greater than or equal to the mempoolminfee even for high mempoolminfee
        self.log.info(
            "Test fee rate estimation after restarting node with high MempoolMinFee"
        )
        self.test_feerate_mempoolminfee()

        self.log.info("Restarting node with fresh estimation")
        self.stop_node(0)
        fee_dat = os.path.join(self.nodes[0].datadir, self.chain, "fee_estimates.dat")
        os.remove(fee_dat)
        self.start_node(0)
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)

        self.log.info("Testing estimates with RBF.")
        self.sanity_check_rbf_estimates(self.confutxo + self.memutxo)

        self.log.info("Testing that fee estimation is disabled in blocksonly.")
        self.restart_node(0, ["-blocksonly"])
        assert_raises_rpc_error(
            -32603, "Fee estimation disabled", self.nodes[0].estimatesmartfee, 2
        )

        # Check loading fee estimates is okay
        self.check_fee_estimates_file()

if __name__ == "__main__":
    EstimateFeeTest().main()
