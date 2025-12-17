/* This file has been automatically generated. */
[ADD_16] =
{ .tag = JET
, .jet = simplicity_add_16
, .cmr = {{0x26ae0994u, 0xce8b771au, 0xf7ad2851u, 0xb83b49a5u, 0x95053658u, 0x9f67bd85u, 0x59470460u, 0x29751c0du}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw16
, .cost = 108 /* milli weight units */
}
,[ADD_32] =
{ .tag = JET
, .jet = simplicity_add_32
, .cmr = {{0x3d767446u, 0x6ed69e1du, 0xbedcd480u, 0x57a9e628u, 0x8c222532u, 0xfbc50480u, 0x49928cfbu, 0x77f829d9u}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw32
, .cost = 117 /* milli weight units */
}
,[ADD_64] =
{ .tag = JET
, .jet = simplicity_add_64
, .cmr = {{0x9b56e61eu, 0xefe2805cu, 0xa87396bdu, 0xfb03f5e1u, 0xb1385f7au, 0xc4bff768u, 0x4026a07cu, 0xf97fb6f6u}}
, .sourceIx = ty_w128
, .targetIx = ty_pbw64
, .cost = 109 /* milli weight units */
}
,[ADD_8] =
{ .tag = JET
, .jet = simplicity_add_8
, .cmr = {{0xd7328c09u, 0x14ee999eu, 0xfa0a6cb2u, 0x6eb40912u, 0xc215c062u, 0xe58a981au, 0xe6b2e4a8u, 0x0474a1dau}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw8
, .cost = 112 /* milli weight units */
}
,[ALL_16] =
{ .tag = JET
, .jet = simplicity_all_16
, .cmr = {{0x16f0c930u, 0x7eb8f4c1u, 0xfdd1bafau, 0xef287924u, 0x2958498eu, 0x8f5b2e0du, 0x29f06553u, 0xdc06a0bdu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 62 /* milli weight units */
}
,[ALL_32] =
{ .tag = JET
, .jet = simplicity_all_32
, .cmr = {{0x0eb8b40du, 0x29021747u, 0xeec451d4u, 0xe663586au, 0x436c2db0u, 0x932675dau, 0xf2166123u, 0xbfe452a6u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 65 /* milli weight units */
}
,[ALL_64] =
{ .tag = JET
, .jet = simplicity_all_64
, .cmr = {{0xa65c82d5u, 0x3d382ee2u, 0x9aa88b77u, 0x18a97fbbu, 0xce6475ecu, 0x32c4b4cdu, 0x6908fde4u, 0x5d81b624u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 79 /* milli weight units */
}
,[ALL_8] =
{ .tag = JET
, .jet = simplicity_all_8
, .cmr = {{0x1d3ec7fbu, 0x6a07847cu, 0x92b8a998u, 0xe1f6b478u, 0x319d050au, 0x387642f4u, 0x032d2f7du, 0x2e027fcdu}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 76 /* milli weight units */
}
,[AND_1] =
{ .tag = JET
, .jet = simplicity_and_1
, .cmr = {{0xb773cefau, 0x418957feu, 0xa7dfb49cu, 0x6c43b3dbu, 0xfa35fa3du, 0x80de8cfdu, 0x4d70c08du, 0x945f5fbau}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 79 /* milli weight units */
}
,[AND_16] =
{ .tag = JET
, .jet = simplicity_and_16
, .cmr = {{0x57dd730bu, 0x1c8ddff1u, 0x3cae2769u, 0x562be0abu, 0xc6ca3bc8u, 0x02da0abbu, 0xb7fc138cu, 0xa463da59u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 88 /* milli weight units */
}
,[AND_32] =
{ .tag = JET
, .jet = simplicity_and_32
, .cmr = {{0x753e332du, 0xdfa096f0u, 0x8399ffaau, 0x7ec4da40u, 0x35bcbaa1u, 0x42e6e38du, 0x4cb607ceu, 0x1f0b051du}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 94 /* milli weight units */
}
,[AND_64] =
{ .tag = JET
, .jet = simplicity_and_64
, .cmr = {{0xf1ad5e6cu, 0x63ee5c89u, 0x0b0f2e71u, 0x1561b905u, 0x316487acu, 0x4044dd23u, 0x0cf6a736u, 0xf81bd4f3u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 93 /* milli weight units */
}
,[AND_8] =
{ .tag = JET
, .jet = simplicity_and_8
, .cmr = {{0xac828b72u, 0x4c5f5340u, 0xb51e76e7u, 0xb6e8b23au, 0xeab7533fu, 0xd8c091aeu, 0x2a515530u, 0xae7ab200u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 91 /* milli weight units */
}
,[ANNEX_HASH] =
{ .tag = JET
, .jet = simplicity_annex_hash
, .cmr = {{0xeba1b7fbu, 0x2ea8d57au, 0xf3b6273eu, 0x5182fb99u, 0xdb1708b2u, 0xd0afb9c6u, 0xad0358ccu, 0x3f647dc8u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256mw256
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 241 /* milli weight units */
}
,[ASSET_AMOUNT_HASH] =
{ .tag = JET
, .jet = simplicity_asset_amount_hash
, .cmr = {{0xa864d1f1u, 0xf83950b6u, 0x3196fe88u, 0xbaec2e42u, 0x28953230u, 0xbe45c88cu, 0x4e0b9917u, 0x8b338f07u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256pspbw256w256spbw256w64
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 308 /* milli weight units */
}
,[BIP_0340_VERIFY] =
{ .tag = JET
, .jet = simplicity_bip_0340_verify
, .cmr = {{0xc9c45a8au, 0xec865914u, 0x3bfe2af6u, 0xead48d4eu, 0x0542453au, 0xcae84b9bu, 0xbb97656bu, 0x670bdfddu}}
, .sourceIx = ty_w1Ki
, .targetIx = ty_u
, .cost = 49087 /* milli weight units */
}
,[BUILD_TAPBRANCH] =
{ .tag = JET
, .jet = simplicity_build_tapbranch
, .cmr = {{0xcbecf9bcu, 0xe172c50fu, 0x585951dfu, 0xf0e0523du, 0xb109e570u, 0x25ec37deu, 0x2c3a74d4u, 0xa673f225u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 2563 /* milli weight units */
}
,[BUILD_TAPLEAF_SIMPLICITY] =
{ .tag = JET
, .jet = simplicity_build_tapleaf_simplicity
, .cmr = {{0x736e9fceu, 0xfb5b873eu, 0x09f2681eu, 0x74e49f5au, 0x1056f90au, 0xf52ab3ecu, 0x5b197727u, 0xd39834beu}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 1843 /* milli weight units */
}
,[BUILD_TAPTWEAK] =
{ .tag = JET
, .jet = simplicity_build_taptweak
, .cmr = {{0x38741f80u, 0xa2bf10f8u, 0xf8723077u, 0xc6741cbeu, 0xae2dcac8u, 0x57901b81u, 0x3725806fu, 0x21898ee3u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 92813 /* milli weight units */
}
,[CALCULATE_ASSET] =
{ .tag = JET
, .jet = simplicity_calculate_asset
, .cmr = {{0x676f988eu, 0x547251e7u, 0xa09eede1u, 0x920a9c5du, 0x35e75759u, 0xe6d8d49bu, 0x580ceb3eu, 0x146bcf7eu}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 807 /* milli weight units */
}
,[CALCULATE_CONFIDENTIAL_TOKEN] =
{ .tag = JET
, .jet = simplicity_calculate_confidential_token
, .cmr = {{0x8e402905u, 0x60743398u, 0x40c0e700u, 0x9b66f382u, 0xbb98d7ddu, 0xe719dd2cu, 0x9f2fd3acu, 0xd9325ac6u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 707 /* milli weight units */
}
,[CALCULATE_EXPLICIT_TOKEN] =
{ .tag = JET
, .jet = simplicity_calculate_explicit_token
, .cmr = {{0x87522802u, 0x0b456c99u, 0x8aad3ac5u, 0xc4836f71u, 0x9445dcbeu, 0x6bd274e8u, 0xca145d82u, 0x797e1b92u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 771 /* milli weight units */
}
,[CALCULATE_ISSUANCE_ENTROPY] =
{ .tag = JET
, .jet = simplicity_calculate_issuance_entropy
, .cmr = {{0xa6e7c434u, 0xaee5a733u, 0x0c1fc246u, 0xf71dcf05u, 0x1ce54bdeu, 0xe7b8e639u, 0x57be18c2u, 0xd751c653u}}
, .sourceIx = ty_ppw256w32w256
, .targetIx = ty_w256
, .cost = 2095 /* milli weight units */
}
,[CH_1] =
{ .tag = JET
, .jet = simplicity_ch_1
, .cmr = {{0xb841b857u, 0xa4aa50eau, 0xca27a26fu, 0x7442fcbfu, 0xe954677au, 0xe6d455f6u, 0x05654989u, 0xe35aeb13u}}
, .sourceIx = ty_pbw2
, .targetIx = ty_b
, .cost = 78 /* milli weight units */
}
,[CH_16] =
{ .tag = JET
, .jet = simplicity_ch_16
, .cmr = {{0x9cff11a0u, 0x9b6041e5u, 0xf2639ae4u, 0xc065a18fu, 0xc675db2fu, 0xbd985408u, 0xe28f027au, 0x99110e11u}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 94 /* milli weight units */
}
,[CH_32] =
{ .tag = JET
, .jet = simplicity_ch_32
, .cmr = {{0x071cef80u, 0x39c79f71u, 0x31cd6a5fu, 0xe493dc26u, 0x8f9db58fu, 0x7b20a855u, 0x55e297bdu, 0xd216cf40u}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 91 /* milli weight units */
}
,[CH_64] =
{ .tag = JET
, .jet = simplicity_ch_64
, .cmr = {{0xd555d219u, 0x63b0192fu, 0xc97214b6u, 0x3dc1c3afu, 0x758b2911u, 0x58f0e1a3u, 0xbcfdea67u, 0x9c666da6u}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 91 /* milli weight units */
}
,[CH_8] =
{ .tag = JET
, .jet = simplicity_ch_8
, .cmr = {{0x353f63b0u, 0xf8cb54f5u, 0xae6575afu, 0x8ca2242cu, 0xeee9f27au, 0x84186eb8u, 0x0e620d5eu, 0x2e8548ecu}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 77 /* milli weight units */
}
,[CHECK_LOCK_DISTANCE] =
{ .tag = JET
, .jet = simplicity_check_lock_distance
, .cmr = {{0x7f78c7a7u, 0x7a25ada2u, 0x23267d23u, 0x9a5922f7u, 0x64b8ac0cu, 0x2fcef68eu, 0xb93c0d92u, 0xda4af515u}}
, .sourceIx = ty_w16
, .targetIx = ty_u
, .cost = 105 /* milli weight units */
}
,[CHECK_LOCK_DURATION] =
{ .tag = JET
, .jet = simplicity_check_lock_duration
, .cmr = {{0x73dac8e2u, 0x5d87eaf3u, 0x82c2a772u, 0x06ad38b9u, 0x384361e7u, 0xd0dc87c0u, 0xfa7af7eau, 0x524597b7u}}
, .sourceIx = ty_w16
, .targetIx = ty_u
, .cost = 102 /* milli weight units */
}
,[CHECK_LOCK_HEIGHT] =
{ .tag = JET
, .jet = simplicity_check_lock_height
, .cmr = {{0x9e7898d0u, 0x37627134u, 0xd2bd70c7u, 0xfca9cba4u, 0x5eaf267du, 0x4d09ad50u, 0xa9ef717au, 0x8f2749dbu}}
, .sourceIx = ty_w32
, .targetIx = ty_u
, .cost = 77 /* milli weight units */
}
,[CHECK_LOCK_TIME] =
{ .tag = JET
, .jet = simplicity_check_lock_time
, .cmr = {{0x68673d12u, 0xe2732faau, 0x1d39e213u, 0x6b1406afu, 0xa098a84cu, 0x96e8d605u, 0x02a2dd61u, 0xc59570bbu}}
, .sourceIx = ty_w32
, .targetIx = ty_u
, .cost = 93 /* milli weight units */
}
,[CHECK_SIG_VERIFY] =
{ .tag = JET
, .jet = simplicity_check_sig_verify
, .cmr = {{0xb5801554u, 0x6d285266u, 0x5dd21bf1u, 0x12662670u, 0x20fa5e27u, 0x5001dd46u, 0x18fa4156u, 0x25952e68u}}
, .sourceIx = ty_ppw256w512w512
, .targetIx = ty_u
, .cost = 50000 /* milli weight units */
}
,[COMPLEMENT_1] =
{ .tag = JET
, .jet = simplicity_complement_1
, .cmr = {{0xed74eeb8u, 0x3a00c713u, 0xcc14f33eu, 0xfe553383u, 0xcd0411ccu, 0x3020fd89u, 0x27931667u, 0x5d910e66u}}
, .sourceIx = ty_b
, .targetIx = ty_b
, .cost = 79 /* milli weight units */
}
,[COMPLEMENT_16] =
{ .tag = JET
, .jet = simplicity_complement_16
, .cmr = {{0x61fdd904u, 0xa4aeb7ebu, 0x7684af61u, 0x8e25aae9u, 0x07cd1db0u, 0xf62d9703u, 0xc5b854e1u, 0x663cac9fu}}
, .sourceIx = ty_w16
, .targetIx = ty_w16
, .cost = 75 /* milli weight units */
}
,[COMPLEMENT_32] =
{ .tag = JET
, .jet = simplicity_complement_32
, .cmr = {{0xfeb02cc3u, 0x6e195b46u, 0x2ae504a9u, 0x12dadfe6u, 0x6ad47f23u, 0xa0cb3baeu, 0xa21d31aau, 0xa0ce101du}}
, .sourceIx = ty_w32
, .targetIx = ty_w32
, .cost = 93 /* milli weight units */
}
,[COMPLEMENT_64] =
{ .tag = JET
, .jet = simplicity_complement_64
, .cmr = {{0x45072d5au, 0xa0e5c37cu, 0x9e521dccu, 0x92e8f39au, 0x5f75e7d9u, 0x28670acau, 0xb79cd8c8u, 0xb5b59e1au}}
, .sourceIx = ty_w64
, .targetIx = ty_w64
, .cost = 88 /* milli weight units */
}
,[COMPLEMENT_8] =
{ .tag = JET
, .jet = simplicity_complement_8
, .cmr = {{0x6916b28fu, 0xb574d9c9u, 0x08a3f33fu, 0x74bf06f7u, 0xed937254u, 0x247f9efcu, 0x2603d717u, 0x1dd497beu}}
, .sourceIx = ty_w8
, .targetIx = ty_w8
, .cost = 80 /* milli weight units */
}
,[CURRENT_AMOUNT] =
{ .tag = JET
, .jet = simplicity_current_amount
, .cmr = {{0x9e7bc96du, 0x06c1f4a4u, 0xd2bd4333u, 0xd55643cbu, 0xa6863d3au, 0x338e31dcu, 0x0d179558u, 0x4156ea69u}}
, .sourceIx = ty_u
, .targetIx = ty_pspbw256w256spbw256w64
, .cost = 225 /* milli weight units */
}
,[CURRENT_ANNEX_HASH] =
{ .tag = JET
, .jet = simplicity_current_annex_hash
, .cmr = {{0x5f68bdafu, 0xd417febbu, 0x10dee16bu, 0x1d47c21fu, 0xa97efc91u, 0x9157c7f7u, 0xbc54a61du, 0xc4f9f7d6u}}
, .sourceIx = ty_u
, .targetIx = ty_mw256
, .cost = 79 /* milli weight units */
}
,[CURRENT_ASSET] =
{ .tag = JET
, .jet = simplicity_current_asset
, .cmr = {{0x03c84459u, 0xa5ecdfdbu, 0x46e3d38eu, 0xcb79ff22u, 0x243fdcf7u, 0x0d80da1bu, 0xa45699e7u, 0x008eecfeu}}
, .sourceIx = ty_u
, .targetIx = ty_spbw256w256
, .cost = 171 /* milli weight units */
}
,[CURRENT_INDEX] =
{ .tag = JET
, .jet = simplicity_current_index
, .cmr = {{0x15e1051fu, 0xf23f851cu, 0x19131f0du, 0xe6edc488u, 0x2376a257u, 0x90dbd910u, 0x2824aa22u, 0xa889ae84u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 88 /* milli weight units */
}
,[CURRENT_ISSUANCE_ASSET_AMOUNT] =
{ .tag = JET
, .jet = simplicity_current_issuance_asset_amount
, .cmr = {{0xdc410ec6u, 0x9946f08bu, 0x9c4a40cfu, 0x38819cbbu, 0x014e98fcu, 0xe0a9b196u, 0x51455747u, 0x7fe1c2a7u}}
, .sourceIx = ty_u
, .targetIx = ty_mspbw256w64
, .cost = 165 /* milli weight units */
}
,[CURRENT_ISSUANCE_ASSET_PROOF] =
{ .tag = JET
, .jet = simplicity_current_issuance_asset_proof
, .cmr = {{0xe5c1c6f7u, 0xea029614u, 0x8e76f8b1u, 0x79d41519u, 0x8273dcc8u, 0xe1fd891fu, 0x83ef2bdfu, 0xb0692936u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 140 /* milli weight units */
}
,[CURRENT_ISSUANCE_TOKEN_AMOUNT] =
{ .tag = JET
, .jet = simplicity_current_issuance_token_amount
, .cmr = {{0xe3ea9c61u, 0x70955f27u, 0xb01b120cu, 0xd63a3608u, 0x50cc700fu, 0x4725193bu, 0xdff88e01u, 0xdfb04eadu}}
, .sourceIx = ty_u
, .targetIx = ty_mspbw256w64
, .cost = 188 /* milli weight units */
}
,[CURRENT_ISSUANCE_TOKEN_PROOF] =
{ .tag = JET
, .jet = simplicity_current_issuance_token_proof
, .cmr = {{0x92e7d9ddu, 0xbf756e3eu, 0x622a44d4u, 0xd00e8423u, 0x6bcce42fu, 0x16f3200eu, 0x8b89c03cu, 0x441e458au}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 144 /* milli weight units */
}
,[CURRENT_NEW_ISSUANCE_CONTRACT] =
{ .tag = JET
, .jet = simplicity_current_new_issuance_contract
, .cmr = {{0x23781567u, 0x5e9e9cf4u, 0xd8548560u, 0xc761e3bau, 0x7f64dcf2u, 0x5c99521bu, 0xe9d857e4u, 0xea41d868u}}
, .sourceIx = ty_u
, .targetIx = ty_mw256
, .cost = 145 /* milli weight units */
}
,[CURRENT_PEGIN] =
{ .tag = JET
, .jet = simplicity_current_pegin
, .cmr = {{0x369ad5f6u, 0xd760481cu, 0xb353a9bau, 0xcf900ff1u, 0x62b76040u, 0xc07ad0a8u, 0x3e52fcceu, 0xf1295790u}}
, .sourceIx = ty_u
, .targetIx = ty_mw256
, .cost = 147 /* milli weight units */
}
,[CURRENT_PREV_OUTPOINT] =
{ .tag = JET
, .jet = simplicity_current_prev_outpoint
, .cmr = {{0xa1ec909cu, 0x81e508e8u, 0x6e1361bau, 0x82e5fb47u, 0xa2c76bffu, 0xba7321a6u, 0xfc13cb12u, 0xaf7f697cu}}
, .sourceIx = ty_u
, .targetIx = ty_pw256w32
, .cost = 156 /* milli weight units */
}
,[CURRENT_REISSUANCE_BLINDING] =
{ .tag = JET
, .jet = simplicity_current_reissuance_blinding
, .cmr = {{0xb71550c8u, 0xf62e7aacu, 0xff13e8d3u, 0xc0ae054eu, 0xf036409du, 0x76c06579u, 0xc026ad68u, 0xa1b561f8u}}
, .sourceIx = ty_u
, .targetIx = ty_mw256
, .cost = 94 /* milli weight units */
}
,[CURRENT_REISSUANCE_ENTROPY] =
{ .tag = JET
, .jet = simplicity_current_reissuance_entropy
, .cmr = {{0x8b271532u, 0x7a0ac6b0u, 0xd18a2e51u, 0xfbda46c4u, 0xb118ee06u, 0x37d114aeu, 0x3e58a23bu, 0x45d67ff8u}}
, .sourceIx = ty_u
, .targetIx = ty_mw256
, .cost = 85 /* milli weight units */
}
,[CURRENT_SCRIPT_HASH] =
{ .tag = JET
, .jet = simplicity_current_script_hash
, .cmr = {{0xbfaf8584u, 0x43cec833u, 0x7e37839du, 0xc4113502u, 0x7184580cu, 0x892157ceu, 0x730418c0u, 0x8d5ed838u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 134 /* milli weight units */
}
,[CURRENT_SCRIPT_SIG_HASH] =
{ .tag = JET
, .jet = simplicity_current_script_sig_hash
, .cmr = {{0x8baa9df8u, 0x54fe33eeu, 0xea66195cu, 0x97ce18edu, 0x701e107eu, 0xea7e02fbu, 0x2603b2c1u, 0x21575066u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 139 /* milli weight units */
}
,[CURRENT_SEQUENCE] =
{ .tag = JET
, .jet = simplicity_current_sequence
, .cmr = {{0x0f4224f9u, 0xaa8644f5u, 0xc0f7910fu, 0x1739041au, 0xec8e7af7u, 0x7b5f38cfu, 0xdca0fdf4u, 0x8267a6c3u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 89 /* milli weight units */
}
,[DECOMPRESS] =
{ .tag = JET
, .jet = simplicity_decompress
, .cmr = {{0x13973317u, 0xd587418eu, 0xf3063631u, 0xa6edb0acu, 0xfa1cbe49u, 0x83d7574bu, 0x1b305f96u, 0x61c048cbu}}
, .sourceIx = ty_pbw256
, .targetIx = ty_mw512
, .cost = 10861 /* milli weight units */
}
,[DECREMENT_16] =
{ .tag = JET
, .jet = simplicity_decrement_16
, .cmr = {{0xe34db118u, 0x79272b32u, 0x7a3bd034u, 0xc0f61ef6u, 0x0a2be96fu, 0xdfe0b2d5u, 0x7ffe39ceu, 0x714c78fbu}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw16
, .cost = 85 /* milli weight units */
}
,[DECREMENT_32] =
{ .tag = JET
, .jet = simplicity_decrement_32
, .cmr = {{0x019ead5au, 0x7305606du, 0xc950fb55u, 0x476d09c1u, 0x7d66f570u, 0xdab510b9u, 0x0d2a27e2u, 0x266599cfu}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw32
, .cost = 91 /* milli weight units */
}
,[DECREMENT_64] =
{ .tag = JET
, .jet = simplicity_decrement_64
, .cmr = {{0x34752cf4u, 0xe1d0a431u, 0xf017a68bu, 0xebfab741u, 0xbbc88affu, 0xb57cc0b3u, 0x025ccfddu, 0x67622f35u}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw64
, .cost = 89 /* milli weight units */
}
,[DECREMENT_8] =
{ .tag = JET
, .jet = simplicity_decrement_8
, .cmr = {{0x2892ceb3u, 0xb6ec5325u, 0xd0c1b9f5u, 0x20425e4bu, 0x05c2e1f4u, 0x37e0b3f5u, 0x81f41b9du, 0x0f7dff4du}}
, .sourceIx = ty_w8
, .targetIx = ty_pbw8
, .cost = 79 /* milli weight units */
}
,[DIV_MOD_128_64] =
{ .tag = JET
, .jet = simplicity_div_mod_128_64
, .cmr = {{0x2296b70fu, 0x600e8a21u, 0x4ad070b2u, 0x194a677du, 0x3051bc1cu, 0x49018397u, 0x5f2a1d3eu, 0x0cade378u}}
, .sourceIx = ty_pw128w64
, .targetIx = ty_w128
, .cost = 208 /* milli weight units */
}
,[DIV_MOD_16] =
{ .tag = JET
, .jet = simplicity_div_mod_16
, .cmr = {{0x648fab86u, 0x4374846au, 0xbf4f9d9du, 0xefe27561u, 0x4d33f482u, 0x9c36a47eu, 0xcb53d7bfu, 0xb605485fu}}
, .sourceIx = ty_w32
, .targetIx = ty_w32
, .cost = 118 /* milli weight units */
}
,[DIV_MOD_32] =
{ .tag = JET
, .jet = simplicity_div_mod_32
, .cmr = {{0xbd3d4d55u, 0x2d7b347bu, 0xd8a44e3eu, 0xe224c846u, 0xbe230ff6u, 0xe2044ddbu, 0x97f48e27u, 0xd20c4225u}}
, .sourceIx = ty_w64
, .targetIx = ty_w64
, .cost = 115 /* milli weight units */
}
,[DIV_MOD_64] =
{ .tag = JET
, .jet = simplicity_div_mod_64
, .cmr = {{0xfa6bad6au, 0x95e2aba4u, 0x305bfe91u, 0xcc47acc3u, 0xd99b92e6u, 0x75e69d3bu, 0x37bb0913u, 0x3d390d0fu}}
, .sourceIx = ty_w128
, .targetIx = ty_w128
, .cost = 86 /* milli weight units */
}
,[DIV_MOD_8] =
{ .tag = JET
, .jet = simplicity_div_mod_8
, .cmr = {{0x48cd501bu, 0xb2aa2acau, 0xe014fe20u, 0x8bb9941du, 0x07a9bffeu, 0x1ad6cd3du, 0x36fc6b08u, 0x60f6eba7u}}
, .sourceIx = ty_w16
, .targetIx = ty_w16
, .cost = 128 /* milli weight units */
}
,[DIVIDE_16] =
{ .tag = JET
, .jet = simplicity_divide_16
, .cmr = {{0x470b01a5u, 0x7c4f9d8fu, 0x997fcde0u, 0x06191611u, 0xdda4c98bu, 0xa2a5f1dau, 0x134ae4c2u, 0x2d52e920u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 98 /* milli weight units */
}
,[DIVIDE_32] =
{ .tag = JET
, .jet = simplicity_divide_32
, .cmr = {{0xab03acd8u, 0x93610c3cu, 0x6582e7f7u, 0xfbe5e756u, 0x2574a7b2u, 0x6646f1c2u, 0xfdc6e76eu, 0x445a77a1u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 100 /* milli weight units */
}
,[DIVIDE_64] =
{ .tag = JET
, .jet = simplicity_divide_64
, .cmr = {{0xebfc56fbu, 0xb8a47e73u, 0xffabb7eau, 0x228ac784u, 0x37be820eu, 0xddfa4781u, 0x4ccebd26u, 0x1bd8cfffu}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 101 /* milli weight units */
}
,[DIVIDE_8] =
{ .tag = JET
, .jet = simplicity_divide_8
, .cmr = {{0x2ccfbc7cu, 0x02bf4d53u, 0x0493bb22u, 0x867a951du, 0x8ae91312u, 0x66875972u, 0x84e9bbb3u, 0xe1e7e349u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 108 /* milli weight units */
}
,[DIVIDES_16] =
{ .tag = JET
, .jet = simplicity_divides_16
, .cmr = {{0x5fc3ac38u, 0x4d5f4540u, 0x4156971au, 0x768d93bcu, 0x064bc17cu, 0x15a37c27u, 0x019ddeefu, 0x17046dd4u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 93 /* milli weight units */
}
,[DIVIDES_32] =
{ .tag = JET
, .jet = simplicity_divides_32
, .cmr = {{0xcc45b405u, 0x246438f7u, 0x65740b4fu, 0xb0a34dc8u, 0x1b347801u, 0x98863b0fu, 0xb186446au, 0xdfbb09deu}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 87 /* milli weight units */
}
,[DIVIDES_64] =
{ .tag = JET
, .jet = simplicity_divides_64
, .cmr = {{0xdc473bfdu, 0xec30ab98u, 0xd48cd088u, 0x84ef4fffu, 0xef3d4b16u, 0xad5c3711u, 0x2a2035b9u, 0x9bb77458u}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 91 /* milli weight units */
}
,[DIVIDES_8] =
{ .tag = JET
, .jet = simplicity_divides_8
, .cmr = {{0x0b5502acu, 0x4f21f230u, 0xa09ccfafu, 0xfaac77a7u, 0xc41b2bf3u, 0x0b146848u, 0x1e4dfb98u, 0xb6187a0du}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 98 /* milli weight units */
}
,[EQ_1] =
{ .tag = JET
, .jet = simplicity_eq_1
, .cmr = {{0x607f6b8fu, 0x5d25b80eu, 0x05a2bf79u, 0xd62e8707u, 0x99522cc3u, 0xe39ce962u, 0x57455293u, 0xf9b2b2edu}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 74 /* milli weight units */
}
,[EQ_16] =
{ .tag = JET
, .jet = simplicity_eq_16
, .cmr = {{0xc996e42bu, 0x979abc53u, 0x0cc27163u, 0x6671e920u, 0x54876a1eu, 0xcaed1433u, 0xfd619a25u, 0xfe6d03adu}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 84 /* milli weight units */
}
,[EQ_256] =
{ .tag = JET
, .jet = simplicity_eq_256
, .cmr = {{0x778d1506u, 0xc735d277u, 0x6b950facu, 0xefc159b6u, 0x78dec038u, 0x28cf0273u, 0xeeea64a9u, 0xda98c12cu}}
, .sourceIx = ty_w512
, .targetIx = ty_b
, .cost = 225 /* milli weight units */
}
,[EQ_32] =
{ .tag = JET
, .jet = simplicity_eq_32
, .cmr = {{0x66d38903u, 0xe73b1a13u, 0x20c68a4au, 0x3970d71fu, 0x94ba9e2bu, 0x15168399u, 0x43fb15e4u, 0x4ebf57fbu}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 88 /* milli weight units */
}
,[EQ_64] =
{ .tag = JET
, .jet = simplicity_eq_64
, .cmr = {{0xd6a666b4u, 0xe0f9f575u, 0x508dbf3bu, 0x31ceea68u, 0x393c7db2u, 0xe98bc592u, 0xfdd26faeu, 0x837a0b87u}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 100 /* milli weight units */
}
,[EQ_8] =
{ .tag = JET
, .jet = simplicity_eq_8
, .cmr = {{0x99787ba2u, 0x672dd0ebu, 0x4d7d2ea9u, 0x9449de8fu, 0x798e7cb1u, 0x81a5e166u, 0xe1a53f98u, 0x02b62064u}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 95 /* milli weight units */
}
,[FE_ADD] =
{ .tag = JET
, .jet = simplicity_fe_add
, .cmr = {{0xb0593e18u, 0x7ee7333cu, 0x47a05467u, 0xdf66d582u, 0x0a6f5befu, 0x914a4b76u, 0xe5d16331u, 0x4b5ef20eu}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 755 /* milli weight units */
}
,[FE_INVERT] =
{ .tag = JET
, .jet = simplicity_fe_invert
, .cmr = {{0x343e9c90u, 0xf1285060u, 0x56b548d2u, 0xed5e223cu, 0x81f5b06au, 0x1ed86b7cu, 0xd9354057u, 0xaa595102u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 3175 /* milli weight units */
}
,[FE_IS_ODD] =
{ .tag = JET
, .jet = simplicity_fe_is_odd
, .cmr = {{0xdcf0375du, 0x20818a99u, 0xf723f812u, 0x3cbd051au, 0x3878a428u, 0x24b3740fu, 0x6821a5fau, 0x123f14c7u}}
, .sourceIx = ty_w256
, .targetIx = ty_b
, .cost = 290 /* milli weight units */
}
,[FE_IS_ZERO] =
{ .tag = JET
, .jet = simplicity_fe_is_zero
, .cmr = {{0x28ff4169u, 0x9a881aafu, 0xb7a976c0u, 0xc576353fu, 0x7fe54463u, 0xb6aa754cu, 0xf2c6329au, 0xf2650e3bu}}
, .sourceIx = ty_w256
, .targetIx = ty_b
, .cost = 268 /* milli weight units */
}
,[FE_MULTIPLY] =
{ .tag = JET
, .jet = simplicity_fe_multiply
, .cmr = {{0x5669929bu, 0x5f31fa3du, 0x02c5839du, 0xd06354cdu, 0x171635f3u, 0xa0727f32u, 0x2abfc994u, 0xba6290deu}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 808 /* milli weight units */
}
,[FE_MULTIPLY_BETA] =
{ .tag = JET
, .jet = simplicity_fe_multiply_beta
, .cmr = {{0x7a781345u, 0x0d82e935u, 0x690f433eu, 0x65df707au, 0x4dd17534u, 0xa00ddd40u, 0xdd85e3e3u, 0xf78402c3u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 579 /* milli weight units */
}
,[FE_NEGATE] =
{ .tag = JET
, .jet = simplicity_fe_negate
, .cmr = {{0x3b0d7b5cu, 0x2e6c3aebu, 0x5e00085bu, 0x9d30585au, 0xff054e32u, 0x5a998361u, 0x113bfd23u, 0x28c008f6u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 531 /* milli weight units */
}
,[FE_NORMALIZE] =
{ .tag = JET
, .jet = simplicity_fe_normalize
, .cmr = {{0xc51beffau, 0x215e9cdeu, 0x8e933bb9u, 0x4680bae0u, 0x12c4daabu, 0x3d04b6cbu, 0xf0733fd7u, 0x35733538u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 521 /* milli weight units */
}
,[FE_SQUARE] =
{ .tag = JET
, .jet = simplicity_fe_square
, .cmr = {{0x5a6e7b2eu, 0xac73f4e4u, 0x4dfa28fbu, 0x86bb117bu, 0x65606f28u, 0x74d565c9u, 0x799c63e0u, 0xfe692b1au}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 556 /* milli weight units */
}
,[FE_SQUARE_ROOT] =
{ .tag = JET
, .jet = simplicity_fe_square_root
, .cmr = {{0xe00142eau, 0x03094a30u, 0x4ac82bc1u, 0xe2d2dc71u, 0xfb064ed0u, 0x82856735u, 0xb14ff2c7u, 0xfaf036f0u}}
, .sourceIx = ty_w256
, .targetIx = ty_mw256
, .cost = 10275 /* milli weight units */
}
,[FULL_ADD_16] =
{ .tag = JET
, .jet = simplicity_full_add_16
, .cmr = {{0xfc9e5df8u, 0x3bfdb902u, 0x8c87d139u, 0xf8583903u, 0xcb2a0704u, 0x2a73e534u, 0x81deb52fu, 0xf1f1f884u}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw16
, .cost = 121 /* milli weight units */
}
,[FULL_ADD_32] =
{ .tag = JET
, .jet = simplicity_full_add_32
, .cmr = {{0xa7d98d50u, 0xd045cb90u, 0x6b195e65u, 0x11879495u, 0xc8510959u, 0x49a9c01eu, 0x6039a84bu, 0x2a5ec909u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw32
, .cost = 119 /* milli weight units */
}
,[FULL_ADD_64] =
{ .tag = JET
, .jet = simplicity_full_add_64
, .cmr = {{0x7aecc8c9u, 0x053bb2fbu, 0x170c1c97u, 0x2fd40025u, 0x64e152a0u, 0x6d9f4580u, 0x75e38c7au, 0x0698a7f4u}}
, .sourceIx = ty_pbw128
, .targetIx = ty_pbw64
, .cost = 121 /* milli weight units */
}
,[FULL_ADD_8] =
{ .tag = JET
, .jet = simplicity_full_add_8
, .cmr = {{0xed3ba5b7u, 0x9ea45b18u, 0x7a2d43e8u, 0xed802de1u, 0xed442659u, 0x6cbe32e7u, 0x57c85119u, 0x15ffa5cfu}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw8
, .cost = 127 /* milli weight units */
}
,[FULL_DECREMENT_16] =
{ .tag = JET
, .jet = simplicity_full_decrement_16
, .cmr = {{0xd4c2eddau, 0x872c0550u, 0x6f792cf5u, 0x46a89d4du, 0x7cffcb1eu, 0x17f5da61u, 0x03100e7eu, 0x73a7737du}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw16
, .cost = 92 /* milli weight units */
}
,[FULL_DECREMENT_32] =
{ .tag = JET
, .jet = simplicity_full_decrement_32
, .cmr = {{0x7cc2304du, 0x17431210u, 0x2e9b7363u, 0x45c77f77u, 0x1d1f6a9cu, 0x9e1d1cd8u, 0xdb8cb461u, 0x3980c8c2u}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw32
, .cost = 107 /* milli weight units */
}
,[FULL_DECREMENT_64] =
{ .tag = JET
, .jet = simplicity_full_decrement_64
, .cmr = {{0x15c16345u, 0x4bcd7544u, 0x30da5579u, 0xbbcaad26u, 0xe57e95c7u, 0x72224b7bu, 0x83c705f7u, 0xdeb64aa6u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw64
, .cost = 81 /* milli weight units */
}
,[FULL_DECREMENT_8] =
{ .tag = JET
, .jet = simplicity_full_decrement_8
, .cmr = {{0x7c5e94a9u, 0x98028182u, 0x1737b1ceu, 0x73bfda4cu, 0x79ef649bu, 0x3d05cc1cu, 0x00c4a8b6u, 0x4b949bbeu}}
, .sourceIx = ty_pbw8
, .targetIx = ty_pbw8
, .cost = 91 /* milli weight units */
}
,[FULL_INCREMENT_16] =
{ .tag = JET
, .jet = simplicity_full_increment_16
, .cmr = {{0x81380adau, 0xa3a547f1u, 0xbc4bbb64u, 0x6bda9d9fu, 0xb7bd4dc1u, 0xb3a9f3ddu, 0x220b56a4u, 0x7c2798fbu}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw16
, .cost = 89 /* milli weight units */
}
,[FULL_INCREMENT_32] =
{ .tag = JET
, .jet = simplicity_full_increment_32
, .cmr = {{0xa760a844u, 0x9a2ab5deu, 0xdb4ee51bu, 0xf5c25a8fu, 0x06af0666u, 0xdf7fc419u, 0xb498b909u, 0x76d698cbu}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw32
, .cost = 104 /* milli weight units */
}
,[FULL_INCREMENT_64] =
{ .tag = JET
, .jet = simplicity_full_increment_64
, .cmr = {{0xc6af30ddu, 0x286d6e21u, 0xc38860edu, 0x1e2f212au, 0x21b2fd1eu, 0xdeadb5e0u, 0xfce2e3fdu, 0x75b7f3c2u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw64
, .cost = 99 /* milli weight units */
}
,[FULL_INCREMENT_8] =
{ .tag = JET
, .jet = simplicity_full_increment_8
, .cmr = {{0xd304ea28u, 0xa95d496du, 0x14b4f2fbu, 0x5c860372u, 0xecf247beu, 0xfde3ea3bu, 0x2ad67bceu, 0x99039dbcu}}
, .sourceIx = ty_pbw8
, .targetIx = ty_pbw8
, .cost = 72 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_1
, .cmr = {{0x14dcc346u, 0x6fa828a3u, 0xf0740451u, 0xb8037d7au, 0xd603eadcu, 0x80aaeadcu, 0x664434acu, 0x2ad7fd9cu}}
, .sourceIx = ty_pw16b
, .targetIx = ty_pbw16
, .cost = 83 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_2
, .cmr = {{0xafb7e928u, 0xb052c228u, 0x7921662cu, 0xd8ab122fu, 0xe074efd2u, 0x51a5c9cfu, 0xbcaa369du, 0x06337392u}}
, .sourceIx = ty_pw16w2
, .targetIx = ty_pw2w16
, .cost = 83 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_4
, .cmr = {{0x166f348cu, 0x59e26f89u, 0xa83a991fu, 0x67e5dbf7u, 0x10cfae3du, 0x6d969382u, 0x82bb44c1u, 0xafa7109bu}}
, .sourceIx = ty_pw16w4
, .targetIx = ty_pw4w16
, .cost = 89 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_8] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_8
, .cmr = {{0xc0cd015du, 0xe8ac4fccu, 0xd8db89f4u, 0xe5142fdeu, 0x279755b5u, 0x42a24f57u, 0xa2a3c7c1u, 0xf50d1db5u}}
, .sourceIx = ty_pw16w8
, .targetIx = ty_pw8w16
, .cost = 65 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_1
, .cmr = {{0xce33b5d0u, 0xc58d2d0bu, 0x9b5a9944u, 0xd3dabda0u, 0x23cd4464u, 0x7be67cf4u, 0x082830bbu, 0x205f8fbbu}}
, .sourceIx = ty_pw32b
, .targetIx = ty_pbw32
, .cost = 84 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_16] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_16
, .cmr = {{0x1cb36e6fu, 0x99308515u, 0xd4b71190u, 0x9c574b21u, 0x24c1ff42u, 0x2d8d7d94u, 0x82e25d87u, 0x88b3b957u}}
, .sourceIx = ty_pw32w16
, .targetIx = ty_pw16w32
, .cost = 81 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_2
, .cmr = {{0x3faea9b5u, 0x73fc069du, 0x8f430facu, 0xa897b687u, 0x1ea09573u, 0xc715094bu, 0x1f1be081u, 0x8488a716u}}
, .sourceIx = ty_pw32w2
, .targetIx = ty_pw2w32
, .cost = 67 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_4
, .cmr = {{0xcdbb0d23u, 0x31059011u, 0x3c934fe6u, 0x6004d2a1u, 0x1da9cbf8u, 0x873d00deu, 0xe7f02296u, 0xff0a2f12u}}
, .sourceIx = ty_pw32w4
, .targetIx = ty_pw4w32
, .cost = 84 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_8] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_8
, .cmr = {{0xccd924e1u, 0xa6184942u, 0x0ff62ed8u, 0xb245a3aau, 0x18c98c41u, 0xf9c5a3c0u, 0xb885863cu, 0x449b7d14u}}
, .sourceIx = ty_pw32w8
, .targetIx = ty_pw8w32
, .cost = 91 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_1
, .cmr = {{0xd463ccdcu, 0x7fd14e5eu, 0x894162b2u, 0xae714128u, 0xa10dc920u, 0x00b54c84u, 0x3b649ccbu, 0x775626e5u}}
, .sourceIx = ty_pw64b
, .targetIx = ty_pbw64
, .cost = 99 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_16] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_16
, .cmr = {{0x882dce21u, 0x2a0e61f8u, 0xf94cb5e3u, 0x2e00a528u, 0x7cf64f20u, 0xc21fca84u, 0xf1e3df7fu, 0x4a6291cdu}}
, .sourceIx = ty_pw64w16
, .targetIx = ty_pw16w64
, .cost = 90 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_2
, .cmr = {{0x48c89b19u, 0x1a51b6abu, 0x034c80eau, 0xff348238u, 0xd93fb31cu, 0x1e92e7f2u, 0xae49317eu, 0x0e33f82du}}
, .sourceIx = ty_pw64w2
, .targetIx = ty_pw2w64
, .cost = 94 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_32] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_32
, .cmr = {{0x39759073u, 0x33e12730u, 0x6255b7f8u, 0x8939e285u, 0x7f42ae1bu, 0xf0c66240u, 0xa8224c8du, 0xa38bb1beu}}
, .sourceIx = ty_pw64w32
, .targetIx = ty_pw32w64
, .cost = 86 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_4
, .cmr = {{0x293132ebu, 0x15ddf417u, 0x74b0005au, 0x3b5c5095u, 0x9fa8982bu, 0x759e8328u, 0x27c74fa8u, 0x2850666cu}}
, .sourceIx = ty_pw64w4
, .targetIx = ty_pw4w64
, .cost = 94 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_8] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_8
, .cmr = {{0xe6abded8u, 0xbe585eb0u, 0xb6d46e0cu, 0x5eb28a74u, 0x5f4e5c56u, 0xfd6521f8u, 0xf396cb21u, 0xa758f74cu}}
, .sourceIx = ty_pw64w8
, .targetIx = ty_pw8w64
, .cost = 86 /* milli weight units */
}
,[FULL_LEFT_SHIFT_8_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_8_1
, .cmr = {{0x733fed08u, 0x47a2ffacu, 0x9aabf50au, 0x2feb5059u, 0x8984f16du, 0x8b732468u, 0xb3d315c0u, 0x1ea4299bu}}
, .sourceIx = ty_pw8b
, .targetIx = ty_pbw8
, .cost = 96 /* milli weight units */
}
,[FULL_LEFT_SHIFT_8_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_8_2
, .cmr = {{0xb4474d0bu, 0xa1cf4fa2u, 0xd64cd4feu, 0x67bdc92cu, 0xb89efa70u, 0xcb99af77u, 0x91bf7ef6u, 0xe909d2c7u}}
, .sourceIx = ty_pw8w2
, .targetIx = ty_pw2w8
, .cost = 96 /* milli weight units */
}
,[FULL_LEFT_SHIFT_8_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_8_4
, .cmr = {{0x8eb522b9u, 0x970474adu, 0xbb7ab0deu, 0x37c4e7a0u, 0x56a1cb21u, 0x2e4103e4u, 0xa8cbbbb6u, 0x3d975606u}}
, .sourceIx = ty_pw8w4
, .targetIx = ty_pw4w8
, .cost = 85 /* milli weight units */
}
,[FULL_MULTIPLY_16] =
{ .tag = JET
, .jet = simplicity_full_multiply_16
, .cmr = {{0x88470cbfu, 0x9b4dec37u, 0xea05d7b6u, 0x30f2f112u, 0x547567d3u, 0x4f33d96eu, 0x5f611bd9u, 0xda97abb5u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 112 /* milli weight units */
}
,[FULL_MULTIPLY_32] =
{ .tag = JET
, .jet = simplicity_full_multiply_32
, .cmr = {{0x28040600u, 0xa66e1a0cu, 0x52258520u, 0x488b94c8u, 0x20c6cf86u, 0xca27ae39u, 0x034dddcau, 0xb904d1d5u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 96 /* milli weight units */
}
,[FULL_MULTIPLY_64] =
{ .tag = JET
, .jet = simplicity_full_multiply_64
, .cmr = {{0x53014f35u, 0xa8df2091u, 0xaf3ef9b8u, 0xd16b38b9u, 0xbc9661bfu, 0xdbc95733u, 0x3fba2a94u, 0x8c1e8c25u}}
, .sourceIx = ty_w256
, .targetIx = ty_w128
, .cost = 127 /* milli weight units */
}
,[FULL_MULTIPLY_8] =
{ .tag = JET
, .jet = simplicity_full_multiply_8
, .cmr = {{0xd3d24554u, 0xc466dd60u, 0x37545247u, 0x36a71eb2u, 0x35def9b5u, 0x06965e32u, 0xd56826e1u, 0x9fbad6c1u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 109 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_1
, .cmr = {{0xb379e296u, 0xe9a98fb3u, 0xb5662b8bu, 0xa04e3cc1u, 0xa43c7442u, 0x9e931233u, 0xfdd7fc8fu, 0xe6b7a2e0u}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pw16b
, .cost = 80 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_2
, .cmr = {{0xaeb8c608u, 0x06a47920u, 0x7758e390u, 0x83b4a9a7u, 0xa14da4eeu, 0x9bc1097fu, 0xc5cb4b75u, 0x540d7578u}}
, .sourceIx = ty_pw2w16
, .targetIx = ty_pw16w2
, .cost = 79 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_4] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_4
, .cmr = {{0x60b7f084u, 0x75cc0cceu, 0x64dca12du, 0x9f6a919cu, 0x30618110u, 0xeda14065u, 0x929c004eu, 0x7fc1b0fbu}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_pw16w4
, .cost = 88 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_8] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_8
, .cmr = {{0xf79dba3eu, 0x0af3d6a5u, 0x59a9e9dfu, 0xfea710afu, 0x623fe6e6u, 0x644b8979u, 0x95d71b8au, 0x4167ddb0u}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_pw16w8
, .cost = 57 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_1
, .cmr = {{0xad0d5c75u, 0xea684371u, 0x91770d7fu, 0xdf804bbcu, 0x9d573d5fu, 0x10199823u, 0xd809c9c4u, 0x6cd275adu}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pw32b
, .cost = 74 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_16] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_16
, .cmr = {{0x455299fdu, 0x6f42ab49u, 0xdbb709e6u, 0x5a3b5366u, 0x250bdc54u, 0x5d6229e8u, 0xe236056du, 0xdd1977fdu}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_pw32w16
, .cost = 64 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_2
, .cmr = {{0x44384b15u, 0x06d443d2u, 0xf8a2882bu, 0x4563d793u, 0x1a7ebce6u, 0x4acf0d02u, 0xee59ec69u, 0xd3065239u}}
, .sourceIx = ty_pw2w32
, .targetIx = ty_pw32w2
, .cost = 63 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_4] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_4
, .cmr = {{0x2e9a8ab5u, 0xa1817bd0u, 0xb8a46626u, 0x994917a0u, 0xde1a745eu, 0x99520ce6u, 0xebcc67d4u, 0x636551b7u}}
, .sourceIx = ty_pw4w32
, .targetIx = ty_pw32w4
, .cost = 71 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_8] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_8
, .cmr = {{0xaf47d4f9u, 0x6e7d8026u, 0xd44e6ecau, 0x1b807f73u, 0x344ce2eau, 0xf700b2c8u, 0x2b4bb002u, 0x61a86f94u}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_pw32w8
, .cost = 84 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_1
, .cmr = {{0x03afb547u, 0xc30913f1u, 0x6f3e370du, 0x7f9ca029u, 0x0b615b42u, 0x85051bb9u, 0x3c3c1a9bu, 0x72ee8de4u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pw64b
, .cost = 99 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_16] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_16
, .cmr = {{0x1fb056fcu, 0xb690cee3u, 0xcff72c7du, 0xecda806du, 0x2146c492u, 0xae731a6bu, 0x94b8bb4fu, 0x1599b0ccu}}
, .sourceIx = ty_pw16w64
, .targetIx = ty_pw64w16
, .cost = 86 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_2
, .cmr = {{0x0673bff2u, 0x1e375e5du, 0xbcaf3804u, 0x664825ddu, 0x674844d2u, 0xfdb784a4u, 0xfefbc925u, 0xcf6b27adu}}
, .sourceIx = ty_pw2w64
, .targetIx = ty_pw64w2
, .cost = 86 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_32] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_32
, .cmr = {{0x356f7dd4u, 0x6ba33f84u, 0xb06672fdu, 0xe9a2972eu, 0x80f3ea96u, 0x5ae8bc0bu, 0xff67aa2fu, 0x69f10b56u}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_pw64w32
, .cost = 73 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_4] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_4
, .cmr = {{0x4c25f601u, 0x1fd3d1acu, 0x18e11eb4u, 0x3061fad6u, 0x9f3ce39fu, 0x7a99cedeu, 0x50cc85bfu, 0x88bfba82u}}
, .sourceIx = ty_pw4w64
, .targetIx = ty_pw64w4
, .cost = 93 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_8] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_8
, .cmr = {{0xa51df944u, 0x8602fa81u, 0x001aa1b5u, 0xb13be88du, 0x4b2f4d0fu, 0x60740801u, 0xcef99100u, 0x2fe37d6du}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_pw64w8
, .cost = 99 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_8_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_8_1
, .cmr = {{0xd9d4b16du, 0x37e4eb5cu, 0xc5150426u, 0xe3e86cf6u, 0x0abbdfa1u, 0xd0ecb415u, 0x82965e80u, 0x00cbd291u}}
, .sourceIx = ty_pbw8
, .targetIx = ty_pw8b
, .cost = 88 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_8_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_8_2
, .cmr = {{0x079aa166u, 0x17198ad5u, 0xdf2c98a6u, 0x3af76c1bu, 0x3e120fd2u, 0x106b225fu, 0x63fd06acu, 0x571d04a4u}}
, .sourceIx = ty_pw2w8
, .targetIx = ty_pw8w2
, .cost = 86 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_8_4] =
{ .tag = JET
, .jet = simplicity_full_right_shift_8_4
, .cmr = {{0x9d9d3f63u, 0x8a846386u, 0xa21e715fu, 0x39461686u, 0x4a2ef798u, 0x4a88cd95u, 0x50556629u, 0x7be7e06cu}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_pw8w4
, .cost = 89 /* milli weight units */
}
,[FULL_SUBTRACT_16] =
{ .tag = JET
, .jet = simplicity_full_subtract_16
, .cmr = {{0x1fc88e23u, 0x29f4aaf1u, 0x2b30513fu, 0x7a21cf5du, 0x8de24b60u, 0x0a19a217u, 0x41281b4du, 0x61aac633u}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw16
, .cost = 121 /* milli weight units */
}
,[FULL_SUBTRACT_32] =
{ .tag = JET
, .jet = simplicity_full_subtract_32
, .cmr = {{0x782705fbu, 0x42e36a7eu, 0xf831200cu, 0x617738d3u, 0x1e13b1d0u, 0xe7ceed69u, 0x3f133388u, 0x35b30acbu}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw32
, .cost = 116 /* milli weight units */
}
,[FULL_SUBTRACT_64] =
{ .tag = JET
, .jet = simplicity_full_subtract_64
, .cmr = {{0xb2856a91u, 0x80231beeu, 0x3cb89230u, 0xf75c292au, 0xf3e75239u, 0xdbeb3965u, 0x48441e6bu, 0x5a27e813u}}
, .sourceIx = ty_pbw128
, .targetIx = ty_pbw64
, .cost = 98 /* milli weight units */
}
,[FULL_SUBTRACT_8] =
{ .tag = JET
, .jet = simplicity_full_subtract_8
, .cmr = {{0x6885e141u, 0xae234c1eu, 0x2a7e4f23u, 0x52989390u, 0x36969c95u, 0x0f2cefd4u, 0x59b498acu, 0x3dd89220u}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw8
, .cost = 126 /* milli weight units */
}
,[GE_IS_ON_CURVE] =
{ .tag = JET
, .jet = simplicity_ge_is_on_curve
, .cmr = {{0x69f0e7a0u, 0xc5fff870u, 0x84ed6925u, 0xf8db762eu, 0x419e057bu, 0x96834dceu, 0x9699b0b0u, 0x09423059u}}
, .sourceIx = ty_w512
, .targetIx = ty_b
, .cost = 642 /* milli weight units */
}
,[GE_NEGATE] =
{ .tag = JET
, .jet = simplicity_ge_negate
, .cmr = {{0x1ed0ced8u, 0xdd2558e3u, 0x485f6fc3u, 0x2d69a240u, 0x5ecaee31u, 0x2dc4dc65u, 0xe0fd3477u, 0x73f5983du}}
, .sourceIx = ty_w512
, .targetIx = ty_w512
, .cost = 945 /* milli weight units */
}
,[GEJ_ADD] =
{ .tag = JET
, .jet = simplicity_gej_add
, .cmr = {{0x5a1c3103u, 0x49e8ff5cu, 0x5a61ac3eu, 0x10123f74u, 0xe87faba1u, 0x4c78bc83u, 0xf9e34136u, 0x87ecf28bu}}
, .sourceIx = ty_ppw512w256pw512w256
, .targetIx = ty_pw512w256
, .cost = 2897 /* milli weight units */
}
,[GEJ_DOUBLE] =
{ .tag = JET
, .jet = simplicity_gej_double
, .cmr = {{0x1edd0582u, 0xe2fcad99u, 0xb12d506du, 0x29b50a63u, 0x017f6769u, 0x28be5113u, 0x69006e07u, 0xcb80d982u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_pw512w256
, .cost = 1764 /* milli weight units */
}
,[GEJ_EQUIV] =
{ .tag = JET
, .jet = simplicity_gej_equiv
, .cmr = {{0x02747105u, 0x9487a12cu, 0xa207f094u, 0x0594d6cdu, 0x87fc930au, 0x8b5b3143u, 0x4a16a2d6u, 0x7f1d8dd4u}}
, .sourceIx = ty_ppw512w256pw512w256
, .targetIx = ty_b
, .cost = 2220 /* milli weight units */
}
,[GEJ_GE_ADD] =
{ .tag = JET
, .jet = simplicity_gej_ge_add
, .cmr = {{0x1ea710d5u, 0x6eafee32u, 0x5d2607ddu, 0xb45ff017u, 0x0adec2e0u, 0xee9bcc68u, 0xe4b93e1du, 0xe6ad3568u}}
, .sourceIx = ty_ppw512w256w512
, .targetIx = ty_pw512w256
, .cost = 2477 /* milli weight units */
}
,[GEJ_GE_ADD_EX] =
{ .tag = JET
, .jet = simplicity_gej_ge_add_ex
, .cmr = {{0x78f0871bu, 0x8173abdeu, 0x71871126u, 0x3b3ac1d9u, 0x22337ed5u, 0xed138d29u, 0x4962d65cu, 0xe559bd92u}}
, .sourceIx = ty_ppw512w256w512
, .targetIx = ty_pw256pw512w256
, .cost = 2719 /* milli weight units */
}
,[GEJ_GE_EQUIV] =
{ .tag = JET
, .jet = simplicity_gej_ge_equiv
, .cmr = {{0xba899a00u, 0x6216d1c9u, 0x3bd5ecbeu, 0x0080d907u, 0x8a500a72u, 0x9bbd396au, 0x004af51du, 0x4ff7d93au}}
, .sourceIx = ty_ppw512w256w512
, .targetIx = ty_b
, .cost = 1765 /* milli weight units */
}
,[GEJ_INFINITY] =
{ .tag = JET
, .jet = simplicity_gej_infinity
, .cmr = {{0x88a952dbu, 0x3816e942u, 0x59a67537u, 0xfa8fca1au, 0x35a907a8u, 0x6f51ede4u, 0x51fd32ecu, 0x253d9c62u}}
, .sourceIx = ty_u
, .targetIx = ty_pw512w256
, .cost = 716 /* milli weight units */
}
,[GEJ_IS_INFINITY] =
{ .tag = JET
, .jet = simplicity_gej_is_infinity
, .cmr = {{0x2980a735u, 0x414e4321u, 0xafeffefau, 0x8837edb0u, 0xa3309a33u, 0x7d59b7bdu, 0xea921c13u, 0x056b0428u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_b
, .cost = 666 /* milli weight units */
}
,[GEJ_IS_ON_CURVE] =
{ .tag = JET
, .jet = simplicity_gej_is_on_curve
, .cmr = {{0x0187e1e5u, 0xef7634a5u, 0xf016124du, 0x4feb5a93u, 0xdde6aa78u, 0x176cda48u, 0xb165a9aau, 0x8e0449f2u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_b
, .cost = 1016 /* milli weight units */
}
,[GEJ_NEGATE] =
{ .tag = JET
, .jet = simplicity_gej_negate
, .cmr = {{0xb32c74cau, 0xb2c7500bu, 0x73f8ec05u, 0x60fe23fcu, 0x4c21aa66u, 0x596d7f2au, 0xcf496788u, 0x6b76d856u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_pw512w256
, .cost = 1381 /* milli weight units */
}
,[GEJ_NORMALIZE] =
{ .tag = JET
, .jet = simplicity_gej_normalize
, .cmr = {{0x5de0976au, 0xe7f38b36u, 0xf0022814u, 0x966db2bau, 0xed5c4767u, 0x14944d74u, 0x1a8979c4u, 0xbcf8be25u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_mw512
, .cost = 4099 /* milli weight units */
}
,[GEJ_RESCALE] =
{ .tag = JET
, .jet = simplicity_gej_rescale
, .cmr = {{0xdcfc72a7u, 0x68d5be77u, 0x0f8db278u, 0xaeafd18eu, 0x27704c64u, 0xf8b40fa6u, 0xfe54ca94u, 0x727a076eu}}
, .sourceIx = ty_ppw512w256w256
, .targetIx = ty_pw512w256
, .cost = 1908 /* milli weight units */
}
,[GEJ_X_EQUIV] =
{ .tag = JET
, .jet = simplicity_gej_x_equiv
, .cmr = {{0x52cc2147u, 0x09c0d9fcu, 0xa9db1d09u, 0xcc807c75u, 0xcf5a6313u, 0xca540a77u, 0x2d4ea992u, 0x1f37e624u}}
, .sourceIx = ty_pw256pw512w256
, .targetIx = ty_b
, .cost = 1047 /* milli weight units */
}
,[GEJ_Y_IS_ODD] =
{ .tag = JET
, .jet = simplicity_gej_y_is_odd
, .cmr = {{0xfe0106afu, 0xb9d9e24fu, 0xd4dbe545u, 0x11fe272fu, 0x4dcb307au, 0x0ea56d59u, 0x1ceb93abu, 0x4bf88745u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_b
, .cost = 3651 /* milli weight units */
}
,[GENERATE] =
{ .tag = JET
, .jet = simplicity_generate
, .cmr = {{0xdf44e17du, 0x2a559dd0u, 0xa7034954u, 0xab333777u, 0x78b151f1u, 0xcd1e4f9fu, 0xd31b361du, 0x34a8d973u}}
, .sourceIx = ty_w256
, .targetIx = ty_pw512w256
, .cost = 50071 /* milli weight units */
}
,[GENESIS_BLOCK_HASH] =
{ .tag = JET
, .jet = simplicity_genesis_block_hash
, .cmr = {{0x0060b60du, 0x02b13360u, 0x72846acfu, 0xf44dd0f4u, 0x346c74a3u, 0x5e7b5642u, 0x3f9519c4u, 0x42d15dafu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 148 /* milli weight units */
}
,[HASH_TO_CURVE] =
{ .tag = JET
, .jet = simplicity_hash_to_curve
, .cmr = {{0x76f7ca1du, 0xb944ee31u, 0x5ed362feu, 0xe0673c58u, 0x94f8853bu, 0x44607090u, 0x1b857901u, 0xf9499d9bu}}
, .sourceIx = ty_w256
, .targetIx = ty_w512
, .cost = 68094 /* milli weight units */
}
,[HIGH_1] =
{ .tag = JET
, .jet = simplicity_high_1
, .cmr = {{0xc32d877eu, 0x670d6c03u, 0x7cb33533u, 0x289e19a7u, 0x24c368aau, 0x7551daa6u, 0xd2dacccdu, 0x8c95f4d0u}}
, .sourceIx = ty_u
, .targetIx = ty_b
, .cost = 57 /* milli weight units */
}
,[HIGH_16] =
{ .tag = JET
, .jet = simplicity_high_16
, .cmr = {{0x4164ab6eu, 0x2ff8eef6u, 0x3c06c080u, 0xf1dec697u, 0x0b4c5c31u, 0xc02305abu, 0xccd8ed2cu, 0x5e1c45ceu}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 66 /* milli weight units */
}
,[HIGH_32] =
{ .tag = JET
, .jet = simplicity_high_32
, .cmr = {{0xd3a7ce9cu, 0xd5d5fb67u, 0x9a98ef57u, 0xb8632277u, 0x0cb66fb6u, 0xf0616e16u, 0x34cfa84cu, 0x8f6809c6u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 58 /* milli weight units */
}
,[HIGH_64] =
{ .tag = JET
, .jet = simplicity_high_64
, .cmr = {{0x4af91fafu, 0x8e39f4dau, 0x7c28a879u, 0x6594a922u, 0x8213d732u, 0x3eea2ca6u, 0x30752ce4u, 0xc57f16e1u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 68 /* milli weight units */
}
,[HIGH_8] =
{ .tag = JET
, .jet = simplicity_high_8
, .cmr = {{0xcbd78d50u, 0xaf779985u, 0x5adc4903u, 0xdbbefc13u, 0x45d51484u, 0xf03d3c75u, 0x5caaa5cau, 0xa97d4a14u}}
, .sourceIx = ty_u
, .targetIx = ty_w8
, .cost = 59 /* milli weight units */
}
,[INCREMENT_16] =
{ .tag = JET
, .jet = simplicity_increment_16
, .cmr = {{0xdf274888u, 0xce4cebddu, 0x5708b38du, 0xc3dbb19cu, 0xc2f0364bu, 0x2463e99cu, 0xf5aab4f8u, 0xa23ea58au}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw16
, .cost = 69 /* milli weight units */
}
,[INCREMENT_32] =
{ .tag = JET
, .jet = simplicity_increment_32
, .cmr = {{0x54f757aeu, 0xa76bc7a3u, 0x9fc43d19u, 0xb8dd563au, 0x6807df02u, 0x77a56fcbu, 0x501089ceu, 0x7d06774cu}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw32
, .cost = 92 /* milli weight units */
}
,[INCREMENT_64] =
{ .tag = JET
, .jet = simplicity_increment_64
, .cmr = {{0x79ed5f77u, 0x99fb09dau, 0x510429a2u, 0x0128bed0u, 0x91d85876u, 0x47071285u, 0xcdec3a0cu, 0x95709e5bu}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw64
, .cost = 87 /* milli weight units */
}
,[INCREMENT_8] =
{ .tag = JET
, .jet = simplicity_increment_8
, .cmr = {{0x0c717e84u, 0xdf67823fu, 0x5741b3d5u, 0x5dbeb472u, 0x9c2bd62fu, 0x5d1def3cu, 0xabccdd6cu, 0xb8dcb56cu}}
, .sourceIx = ty_w8
, .targetIx = ty_pbw8
, .cost = 85 /* milli weight units */
}
,[INPUT_AMOUNT] =
{ .tag = JET
, .jet = simplicity_input_amount
, .cmr = {{0xdd65ac32u, 0xc87d996bu, 0x407200feu, 0xefc479a9u, 0x119c1f28u, 0xcaa488b0u, 0xaac3acf5u, 0x9378d6bau}}
, .sourceIx = ty_w32
, .targetIx = ty_mpspbw256w256spbw256w64
, .cost = 285 /* milli weight units */
}
,[INPUT_AMOUNTS_HASH] =
{ .tag = JET
, .jet = simplicity_input_amounts_hash
, .cmr = {{0x2b4df47eu, 0x758ffe88u, 0xe9cd5d39u, 0x6df3d7f9u, 0xec2fbd5au, 0x5395a6d8u, 0xc1ab7044u, 0x00c64f70u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 140 /* milli weight units */
}
,[INPUT_ANNEX_HASH] =
{ .tag = JET
, .jet = simplicity_input_annex_hash
, .cmr = {{0x0fd9e4c6u, 0x316b3728u, 0x6e885443u, 0x0e7d7418u, 0x21dc8954u, 0x4a3a59f0u, 0x3dfa40e5u, 0x32b8107fu}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 90 /* milli weight units */
}
,[INPUT_ANNEXES_HASH] =
{ .tag = JET
, .jet = simplicity_input_annexes_hash
, .cmr = {{0xded1522eu, 0xfa5a2b37u, 0x6173e440u, 0xde227ad0u, 0xc92be2edu, 0xce7c23b5u, 0x1480e50au, 0x77fc564bu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 155 /* milli weight units */
}
,[INPUT_ASSET] =
{ .tag = JET
, .jet = simplicity_input_asset
, .cmr = {{0x872b3653u, 0xd65ae7f9u, 0x166cbf70u, 0xd4fd3140u, 0x4798884du, 0x611b647fu, 0xb9f4a307u, 0xc48011b8u}}
, .sourceIx = ty_w32
, .targetIx = ty_mspbw256w256
, .cost = 162 /* milli weight units */
}
,[INPUT_HASH] =
{ .tag = JET
, .jet = simplicity_input_hash
, .cmr = {{0x3309bb46u, 0xb3158d23u, 0x124f8cedu, 0xaaa1ed3bu, 0x09a8aefeu, 0x81212e11u, 0x335523b6u, 0xb207c544u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 965 /* milli weight units */
}
,[INPUT_OUTPOINTS_HASH] =
{ .tag = JET
, .jet = simplicity_input_outpoints_hash
, .cmr = {{0x140a2baeu, 0xad083baau, 0xc806128au, 0xb649e52bu, 0xbd3c6456u, 0x700cbcfcu, 0x6dbb2959u, 0x173a4df2u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 142 /* milli weight units */
}
,[INPUT_PEGIN] =
{ .tag = JET
, .jet = simplicity_input_pegin
, .cmr = {{0xdb376f4au, 0xe84ec051u, 0xa494d766u, 0x8039727bu, 0x53353be5u, 0xae045be7u, 0x6c4b04dau, 0xbd5bbfbbu}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 151 /* milli weight units */
}
,[INPUT_PREV_OUTPOINT] =
{ .tag = JET
, .jet = simplicity_input_prev_outpoint
, .cmr = {{0x91f4d6ecu, 0x484f60cau, 0xe9297858u, 0xcac47dadu, 0x50db07dcu, 0x1f937ac6u, 0x144c9cb4u, 0xa9ef79d2u}}
, .sourceIx = ty_w32
, .targetIx = ty_mpw256w32
, .cost = 160 /* milli weight units */
}
,[INPUT_SCRIPT_HASH] =
{ .tag = JET
, .jet = simplicity_input_script_hash
, .cmr = {{0xc316df21u, 0x778e6241u, 0x05ca5990u, 0x4b9208e2u, 0xd423228bu, 0x3eb1cf68u, 0xb8eca47bu, 0xbc7b2ff3u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 147 /* milli weight units */
}
,[INPUT_SCRIPT_SIG_HASH] =
{ .tag = JET
, .jet = simplicity_input_script_sig_hash
, .cmr = {{0x941f2b4fu, 0x3b7f2097u, 0xd6ea00e9u, 0x7c1e2ba0u, 0x1cb26487u, 0xc5d2f472u, 0x3738b37cu, 0x732c05c4u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 153 /* milli weight units */
}
,[INPUT_SCRIPT_SIGS_HASH] =
{ .tag = JET
, .jet = simplicity_input_script_sigs_hash
, .cmr = {{0x6c748142u, 0x0124b664u, 0x5d32ff9bu, 0x454a1c92u, 0xc6718641u, 0x0e23cf60u, 0x9915df90u, 0x72120fc6u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 138 /* milli weight units */
}
,[INPUT_SCRIPTS_HASH] =
{ .tag = JET
, .jet = simplicity_input_scripts_hash
, .cmr = {{0xdc13519eu, 0x753e5875u, 0x3c709885u, 0x41ad8a0bu, 0xb5888d08u, 0x46e35e4du, 0x4273f656u, 0x85322ae9u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 137 /* milli weight units */
}
,[INPUT_SEQUENCE] =
{ .tag = JET
, .jet = simplicity_input_sequence
, .cmr = {{0x3c6e4a3cu, 0x21ced524u, 0x736c545cu, 0x20e4e2deu, 0xf7678f08u, 0x4cd12bcdu, 0x6506a520u, 0x0b5c1738u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw32
, .cost = 99 /* milli weight units */
}
,[INPUT_SEQUENCES_HASH] =
{ .tag = JET
, .jet = simplicity_input_sequences_hash
, .cmr = {{0xc888c873u, 0x302b92b0u, 0x0f206852u, 0xc9eaef00u, 0x0b40109au, 0x53dd686au, 0x53f11db5u, 0x074d9937u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 142 /* milli weight units */
}
,[INPUT_UTXO_HASH] =
{ .tag = JET
, .jet = simplicity_input_utxo_hash
, .cmr = {{0x8503d052u, 0x6ba95a8fu, 0xba8ca135u, 0x1c7991c2u, 0xa386b72fu, 0xa81431eau, 0xdbab62ccu, 0x1f5dc518u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 1996 /* milli weight units */
}
,[INPUT_UTXOS_HASH] =
{ .tag = JET
, .jet = simplicity_input_utxos_hash
, .cmr = {{0x9c48ab31u, 0xc050ed06u, 0x5aed5168u, 0x66c16c1bu, 0x96c195eau, 0x8e18b024u, 0xbfc0b817u, 0x5849f04eu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 140 /* milli weight units */
}
,[INPUTS_HASH] =
{ .tag = JET
, .jet = simplicity_inputs_hash
, .cmr = {{0x523cd71cu, 0x5789882fu, 0x8375a5a5u, 0x4560def8u, 0x463d1c51u, 0xe56b7e11u, 0x2693e77bu, 0x844f4740u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 154 /* milli weight units */
}
,[INTERNAL_KEY] =
{ .tag = JET
, .jet = simplicity_internal_key
, .cmr = {{0x9b742e47u, 0x1b0e591au, 0x972b0d44u, 0x21f9c965u, 0x9397f5dau, 0xba2c14dau, 0xc62c4a33u, 0x9cc3e7ecu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 152 /* milli weight units */
}
,[IS_ONE_16] =
{ .tag = JET
, .jet = simplicity_is_one_16
, .cmr = {{0x8435879cu, 0xcb864419u, 0x8dcb9a0cu, 0xd73546d7u, 0x01fdd5a4u, 0xc44323f5u, 0x63971599u, 0xc37d16fbu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 82 /* milli weight units */
}
,[IS_ONE_32] =
{ .tag = JET
, .jet = simplicity_is_one_32
, .cmr = {{0xddfbd9f0u, 0xa2e67c07u, 0xdedb89e8u, 0x96b6c4f7u, 0xd45c5147u, 0xeed0614eu, 0x4ce7d087u, 0x69aff82du}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 65 /* milli weight units */
}
,[IS_ONE_64] =
{ .tag = JET
, .jet = simplicity_is_one_64
, .cmr = {{0x35c52554u, 0x8e48eea0u, 0xf77b3bf9u, 0x7ab67a1fu, 0xfe8fb094u, 0xede3325eu, 0x4064b165u, 0x9c6d0765u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 83 /* milli weight units */
}
,[IS_ONE_8] =
{ .tag = JET
, .jet = simplicity_is_one_8
, .cmr = {{0x0aba9e57u, 0x6e64d280u, 0x4c8ac468u, 0x2bbba539u, 0x0ebc31a6u, 0xe3e2650fu, 0x9219235du, 0xf4a6ecbbu}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 91 /* milli weight units */
}
,[IS_ZERO_16] =
{ .tag = JET
, .jet = simplicity_is_zero_16
, .cmr = {{0xa25abd9cu, 0xd2a4070cu, 0x742ef8deu, 0xb0682922u, 0x46032b96u, 0xa517223bu, 0x128cfc12u, 0xd215c5bau}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 75 /* milli weight units */
}
,[IS_ZERO_32] =
{ .tag = JET
, .jet = simplicity_is_zero_32
, .cmr = {{0x612a480cu, 0xed6a79dau, 0x6119546eu, 0x056b8df9u, 0xfa95d112u, 0x4b96d601u, 0xe1d3ea91u, 0x8cc56069u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 85 /* milli weight units */
}
,[IS_ZERO_64] =
{ .tag = JET
, .jet = simplicity_is_zero_64
, .cmr = {{0x18e8e177u, 0x6ba080ccu, 0xd3e1d60cu, 0xb7534145u, 0x36bf70dfu, 0x185f72c9u, 0xe070796fu, 0x4c63cc71u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 80 /* milli weight units */
}
,[IS_ZERO_8] =
{ .tag = JET
, .jet = simplicity_is_zero_8
, .cmr = {{0xb4baa509u, 0x38108426u, 0x740d82cfu, 0x1211e0edu, 0x126de3b7u, 0x6b8d259cu, 0x50ad4b8fu, 0xcab10ab6u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 77 /* milli weight units */
}
,[ISSUANCE] =
{ .tag = JET
, .jet = simplicity_issuance
, .cmr = {{0x5c646312u, 0xc169c680u, 0x27979e1bu, 0xc326c5dcu, 0x95e5c516u, 0x8d00d98eu, 0x9d504a7cu, 0xde21d768u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmb
, .cost = 91 /* milli weight units */
}
,[ISSUANCE_ASSET] =
{ .tag = JET
, .jet = simplicity_issuance_asset
, .cmr = {{0x690bf918u, 0xe1527756u, 0xcfbbf51cu, 0x83136214u, 0x3756d52du, 0x04e1294fu, 0x1264950cu, 0x6267e5c9u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 151 /* milli weight units */
}
,[ISSUANCE_ASSET_AMOUNT] =
{ .tag = JET
, .jet = simplicity_issuance_asset_amount
, .cmr = {{0x83605146u, 0xff26a49cu, 0xa16f1c4au, 0x6e0ce0b7u, 0xc214df12u, 0xc377f522u, 0x523deb6du, 0x5d0ee44du}}
, .sourceIx = ty_w32
, .targetIx = ty_mmspbw256w64
, .cost = 162 /* milli weight units */
}
,[ISSUANCE_ASSET_AMOUNTS_HASH] =
{ .tag = JET
, .jet = simplicity_issuance_asset_amounts_hash
, .cmr = {{0x70f11c3bu, 0x3b5d1657u, 0xd3b7c7eeu, 0xec850c30u, 0xa32ea96au, 0xa36426fau, 0x30ebf549u, 0x9851a880u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 139 /* milli weight units */
}
,[ISSUANCE_ASSET_PROOF] =
{ .tag = JET
, .jet = simplicity_issuance_asset_proof
, .cmr = {{0x0b061f9fu, 0x670c05dcu, 0x39f43831u, 0x3c1379d9u, 0xbf63f44fu, 0x9876b956u, 0x693b41b7u, 0xf94c6c9du}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 150 /* milli weight units */
}
,[ISSUANCE_BLINDING_ENTROPY_HASH] =
{ .tag = JET
, .jet = simplicity_issuance_blinding_entropy_hash
, .cmr = {{0xbfb6a934u, 0x44e2a0b3u, 0xfb7c13d1u, 0x589a787bu, 0x66e84b69u, 0xb6274ceau, 0x20ad9999u, 0xeb8ca67cu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 129 /* milli weight units */
}
,[ISSUANCE_ENTROPY] =
{ .tag = JET
, .jet = simplicity_issuance_entropy
, .cmr = {{0x6ddf99f2u, 0x8c70198du, 0x65417a83u, 0x6074f1b9u, 0xb610f21fu, 0x5119fd55u, 0x0f60b6f8u, 0xe995bbf3u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 153 /* milli weight units */
}
,[ISSUANCE_HASH] =
{ .tag = JET
, .jet = simplicity_issuance_hash
, .cmr = {{0x6e9f8ae8u, 0xc45bfb7eu, 0x4e0cf2d0u, 0x0f66832bu, 0x479de758u, 0x2cdbc851u, 0xd0b3230fu, 0xdfbbb78du}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 3738 /* milli weight units */
}
,[ISSUANCE_RANGE_PROOFS_HASH] =
{ .tag = JET
, .jet = simplicity_issuance_range_proofs_hash
, .cmr = {{0x7e97f519u, 0x534831c7u, 0x7acf73a2u, 0x0fd53cb3u, 0x1d1d156cu, 0x9acbbf87u, 0x3d1492d6u, 0x0d1367d8u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 129 /* milli weight units */
}
,[ISSUANCE_TOKEN] =
{ .tag = JET
, .jet = simplicity_issuance_token
, .cmr = {{0xbc2cb7e9u, 0x64b2a2aeu, 0xd79b75c5u, 0xf4408fe8u, 0x4a6dc9b9u, 0xe8cd2c69u, 0x4d57c5f9u, 0xb61205b9u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 149 /* milli weight units */
}
,[ISSUANCE_TOKEN_AMOUNT] =
{ .tag = JET
, .jet = simplicity_issuance_token_amount
, .cmr = {{0x7e106464u, 0xa13b6764u, 0xce74d3feu, 0x6ba1cbd5u, 0xf4aeb86bu, 0xbbdf5888u, 0xddf9901du, 0xd8b619aau}}
, .sourceIx = ty_w32
, .targetIx = ty_mmspbw256w64
, .cost = 196 /* milli weight units */
}
,[ISSUANCE_TOKEN_AMOUNTS_HASH] =
{ .tag = JET
, .jet = simplicity_issuance_token_amounts_hash
, .cmr = {{0x94b5a459u, 0x5b674dbfu, 0xc49c4e8eu, 0xa1a9abc9u, 0x3ed2f770u, 0x7c4212a3u, 0x2b5b86acu, 0x1318c9deu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 138 /* milli weight units */
}
,[ISSUANCE_TOKEN_PROOF] =
{ .tag = JET
, .jet = simplicity_issuance_token_proof
, .cmr = {{0x46ed5275u, 0xf55ab341u, 0xf57c3519u, 0xffedf4e3u, 0x7f3f59c9u, 0x7ce9b8e1u, 0x7fffae16u, 0xc6cc1c0eu}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 150 /* milli weight units */
}
,[ISSUANCES_HASH] =
{ .tag = JET
, .jet = simplicity_issuances_hash
, .cmr = {{0x58659f3fu, 0x7e0130c4u, 0xfc916f49u, 0xbc32eaa0u, 0x883acb90u, 0x22fcad5eu, 0x7805839au, 0x99e50cf9u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 141 /* milli weight units */
}
,[LBTC_ASSET] =
{ .tag = JET
, .jet = simplicity_lbtc_asset
, .cmr = {{0x2af7f73au, 0xe3fbbfa2u, 0xd23a8774u, 0xe6e3ca09u, 0x3254da2fu, 0xd2a9f397u, 0xa6ae0a30u, 0xbb48f0d7u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 145 /* milli weight units */
}
,[LE_16] =
{ .tag = JET
, .jet = simplicity_le_16
, .cmr = {{0x63da727cu, 0xcb4c6a9du, 0x4e000964u, 0xe763bff9u, 0x34eaafd0u, 0x44287e12u, 0x68d07ecdu, 0xfde207e1u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 112 /* milli weight units */
}
,[LE_32] =
{ .tag = JET
, .jet = simplicity_le_32
, .cmr = {{0xdee29a91u, 0x656d7ae7u, 0x3df4956fu, 0xd8a2c6b6u, 0x27aab51cu, 0x1129f9feu, 0x7f6ed3e3u, 0x4792c762u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 93 /* milli weight units */
}
,[LE_64] =
{ .tag = JET
, .jet = simplicity_le_64
, .cmr = {{0x01c55df7u, 0xd4465966u, 0x659ddfc9u, 0x4b36d033u, 0x242c2ec5u, 0x93cee121u, 0x22440775u, 0x66ed015fu}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 93 /* milli weight units */
}
,[LE_8] =
{ .tag = JET
, .jet = simplicity_le_8
, .cmr = {{0x0fb72d9fu, 0x8ee2370au, 0xba55663au, 0x4899162eu, 0x40ca5514u, 0x713efb25u, 0xe4a89e2au, 0x104b34dbu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 109 /* milli weight units */
}
,[LEFT_EXTEND_16_32] =
{ .tag = JET
, .jet = simplicity_left_extend_16_32
, .cmr = {{0xdcf42b65u, 0x42f6d41cu, 0xb7b50e7cu, 0x772f3c7fu, 0x6e432232u, 0xf2ba2079u, 0xb386a05du, 0x7b466addu}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 86 /* milli weight units */
}
,[LEFT_EXTEND_16_64] =
{ .tag = JET
, .jet = simplicity_left_extend_16_64
, .cmr = {{0x2eee48a9u, 0x2237947cu, 0x1a517df9u, 0x95f44f1du, 0xfef20ddbu, 0x4e9b530bu, 0x22d18a0au, 0x7fd628aau}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 89 /* milli weight units */
}
,[LEFT_EXTEND_1_16] =
{ .tag = JET
, .jet = simplicity_left_extend_1_16
, .cmr = {{0x9a48a477u, 0x8e7c3c28u, 0x5ab65329u, 0xd1ccc499u, 0x9d2d194eu, 0x005bd794u, 0x6949533du, 0x8cba806cu}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 67 /* milli weight units */
}
,[LEFT_EXTEND_1_32] =
{ .tag = JET
, .jet = simplicity_left_extend_1_32
, .cmr = {{0xdab6a533u, 0xcbcbe836u, 0x2cf1d5a1u, 0x6ea37cbcu, 0x7edc7fc8u, 0xa9428571u, 0xe171ec6eu, 0xe44d0800u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 60 /* milli weight units */
}
,[LEFT_EXTEND_1_64] =
{ .tag = JET
, .jet = simplicity_left_extend_1_64
, .cmr = {{0x110e5c1eu, 0xf0b469a7u, 0x638570dau, 0x944d232eu, 0x0f28c461u, 0x51a22535u, 0x7de3e904u, 0x57a88ea2u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 76 /* milli weight units */
}
,[LEFT_EXTEND_1_8] =
{ .tag = JET
, .jet = simplicity_left_extend_1_8
, .cmr = {{0x5a831ca9u, 0x9621517au, 0x2b354e5cu, 0xac38bc3au, 0x30c4001fu, 0x20d25d77u, 0x97addcacu, 0x5da86106u}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 65 /* milli weight units */
}
,[LEFT_EXTEND_32_64] =
{ .tag = JET
, .jet = simplicity_left_extend_32_64
, .cmr = {{0x84fcc69bu, 0xa1db50dbu, 0xd5363cf2u, 0x77795760u, 0x1de2568au, 0xdf07af41u, 0x61debb1eu, 0x5e37310au}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 63 /* milli weight units */
}
,[LEFT_EXTEND_8_16] =
{ .tag = JET
, .jet = simplicity_left_extend_8_16
, .cmr = {{0xfea1f25au, 0x82fdf6f8u, 0x669cc40fu, 0xbb8e54a9u, 0x2658bfabu, 0x94eb082fu, 0x717ba265u, 0xb5d844b4u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 88 /* milli weight units */
}
,[LEFT_EXTEND_8_32] =
{ .tag = JET
, .jet = simplicity_left_extend_8_32
, .cmr = {{0x09d703cau, 0x46f75d05u, 0x1a93d0e8u, 0xa2af0501u, 0xa38e8486u, 0x83ef109cu, 0x1fb4b5beu, 0x20e6315du}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 90 /* milli weight units */
}
,[LEFT_EXTEND_8_64] =
{ .tag = JET
, .jet = simplicity_left_extend_8_64
, .cmr = {{0xd3dafcbdu, 0xab69a2bbu, 0x320f8d23u, 0x0cefd09cu, 0x27a154c5u, 0x1e7e5cd5u, 0x334eafedu, 0x19e20df4u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 107 /* milli weight units */
}
,[LEFT_PAD_HIGH_16_32] =
{ .tag = JET
, .jet = simplicity_left_pad_high_16_32
, .cmr = {{0x888c7e0au, 0xb0031475u, 0xc514f9b3u, 0x7c81f45au, 0x47314984u, 0xe5027508u, 0xddc5eb8du, 0x8d10beb9u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 91 /* milli weight units */
}
,[LEFT_PAD_HIGH_16_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_16_64
, .cmr = {{0x526b3505u, 0x450136d6u, 0x81a50b4bu, 0xde4fa612u, 0xda9d69bdu, 0x08170ea3u, 0x2d0a2651u, 0x115072ebu}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 110 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_16] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_16
, .cmr = {{0x93aed6f6u, 0x8750774bu, 0x2dbf8314u, 0xcadebe5au, 0x415243fbu, 0xdf7c2eeau, 0x8b223df3u, 0x261e3bdbu}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 141 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_32] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_32
, .cmr = {{0x008298f8u, 0x2fb6cf37u, 0xe9dc703eu, 0xa4f94956u, 0x5c2965a7u, 0xc7f4fa22u, 0xf5545642u, 0x3408a3abu}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 263 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_64
, .cmr = {{0x2b454ebdu, 0x791ec7dau, 0xcedcb86cu, 0x69d02679u, 0x4a5dc372u, 0x5261e7dcu, 0x1650cc88u, 0x8117fc4fu}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 422 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_8] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_8
, .cmr = {{0x6c277c4cu, 0xd053dd35u, 0x02dbe0bbu, 0xc14eb0b3u, 0x6a201abeu, 0xf3b174b0u, 0xebfe0520u, 0x18b67e67u}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 99 /* milli weight units */
}
,[LEFT_PAD_HIGH_32_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_32_64
, .cmr = {{0x5d41221cu, 0xf6158297u, 0xb06c1957u, 0x112c0d12u, 0xf3eb917au, 0x2f509a53u, 0x9d5c9b79u, 0x10219b65u}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 93 /* milli weight units */
}
,[LEFT_PAD_HIGH_8_16] =
{ .tag = JET
, .jet = simplicity_left_pad_high_8_16
, .cmr = {{0x2178dc76u, 0xc04c79d9u, 0x1815d38cu, 0x967f3421u, 0x3ffcc6c5u, 0xf243c956u, 0x2973f090u, 0xca5caefau}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 88 /* milli weight units */
}
,[LEFT_PAD_HIGH_8_32] =
{ .tag = JET
, .jet = simplicity_left_pad_high_8_32
, .cmr = {{0xa4e86b53u, 0xe5d00fafu, 0x0b3e9d53u, 0x202af773u, 0x8dcb8887u, 0xa18dfee5u, 0xbe34c497u, 0x698ca6b7u}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 103 /* milli weight units */
}
,[LEFT_PAD_HIGH_8_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_8_64
, .cmr = {{0xc843a72cu, 0x41170f40u, 0x3433c436u, 0xa39b05cfu, 0x193c27d8u, 0xbe3530f9u, 0xb94e42d7u, 0x63003d54u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 136 /* milli weight units */
}
,[LEFT_PAD_LOW_16_32] =
{ .tag = JET
, .jet = simplicity_left_pad_low_16_32
, .cmr = {{0x21537f7du, 0x8f97f220u, 0x3cccb035u, 0xef1d4628u, 0x9ee8aa50u, 0xf0236077u, 0xd0d0b210u, 0x700440a1u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 69 /* milli weight units */
}
,[LEFT_PAD_LOW_16_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_16_64
, .cmr = {{0x6b2ea963u, 0x0c5dde03u, 0x7aab2bf7u, 0x33219b99u, 0xc7edc2ecu, 0xedb9a03au, 0xdfd16943u, 0x0b08bb9cu}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 106 /* milli weight units */
}
,[LEFT_PAD_LOW_1_16] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_16
, .cmr = {{0x4aa40520u, 0xfaed72e6u, 0xe9be3be6u, 0x930f1e32u, 0xb0b182c4u, 0x327ada94u, 0xa71f006du, 0x149015f9u}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 65 /* milli weight units */
}
,[LEFT_PAD_LOW_1_32] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_32
, .cmr = {{0xcfb4753bu, 0xb9ba3621u, 0xba093782u, 0x5fade643u, 0x098e385eu, 0xd68efb16u, 0xff58ecf3u, 0x65d7e5e2u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 63 /* milli weight units */
}
,[LEFT_PAD_LOW_1_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_64
, .cmr = {{0xe6f1c09bu, 0x5fe126d0u, 0xea86e7bfu, 0xc0b28e84u, 0x9f8f7efdu, 0x31064ea4u, 0xfd1cca07u, 0x1b45db93u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 61 /* milli weight units */
}
,[LEFT_PAD_LOW_1_8] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_8
, .cmr = {{0xdc5a47f8u, 0xd77765c9u, 0x94cbe86au, 0xae44a9c5u, 0xff2ebc38u, 0x10d79cd8u, 0x3bd2c409u, 0x8c762bf5u}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 56 /* milli weight units */
}
,[LEFT_PAD_LOW_32_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_32_64
, .cmr = {{0x2d88e4d0u, 0x1e0108c0u, 0xd6880f3cu, 0xe8482bb0u, 0x951f2b3fu, 0xc5df4b1au, 0xdb184a1bu, 0xfd1f6465u}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 91 /* milli weight units */
}
,[LEFT_PAD_LOW_8_16] =
{ .tag = JET
, .jet = simplicity_left_pad_low_8_16
, .cmr = {{0xac1a4c97u, 0x83e4dbedu, 0x2700eb29u, 0x52e3062au, 0x5a72712fu, 0x82159861u, 0xb08e67efu, 0x4a71f5f2u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 66 /* milli weight units */
}
,[LEFT_PAD_LOW_8_32] =
{ .tag = JET
, .jet = simplicity_left_pad_low_8_32
, .cmr = {{0x3da5f1a8u, 0xc97819aeu, 0x7e10b936u, 0x4ff84996u, 0xd0d73e69u, 0x8a49da69u, 0x1f69a273u, 0x254201cdu}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 61 /* milli weight units */
}
,[LEFT_PAD_LOW_8_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_8_64
, .cmr = {{0x25bc18d4u, 0x9f934072u, 0x277d3f61u, 0x3bf16c11u, 0x8df197bcu, 0x92872d2au, 0xffe417adu, 0xeaaf1a85u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 112 /* milli weight units */
}
,[LEFT_ROTATE_16] =
{ .tag = JET
, .jet = simplicity_left_rotate_16
, .cmr = {{0x88c12337u, 0xcd754f83u, 0x80986d86u, 0xfe3a89e2u, 0x62746653u, 0xe1badd9cu, 0xc9b47645u, 0xfe57195au}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 77 /* milli weight units */
}
,[LEFT_ROTATE_32] =
{ .tag = JET
, .jet = simplicity_left_rotate_32
, .cmr = {{0x39816ccdu, 0x9e9cf119u, 0x1f065d2eu, 0xb7a7fb83u, 0x828d91ecu, 0x7d9977a1u, 0xfc70be9bu, 0x31a468b9u}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 106 /* milli weight units */
}
,[LEFT_ROTATE_64] =
{ .tag = JET
, .jet = simplicity_left_rotate_64
, .cmr = {{0x8b2355c3u, 0x1e3b614bu, 0xd4b41c3eu, 0xcf277424u, 0xd026766bu, 0x37bc6c10u, 0x5621f4f6u, 0xa16f9bdfu}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 98 /* milli weight units */
}
,[LEFT_ROTATE_8] =
{ .tag = JET
, .jet = simplicity_left_rotate_8
, .cmr = {{0x9e966e88u, 0x0c6b0c48u, 0x3c90beeeu, 0xd7c5737cu, 0xa5f3facfu, 0x85aab3d5u, 0x31ad34bdu, 0x7b1a9b68u}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 88 /* milli weight units */
}
,[LEFT_SHIFT_16] =
{ .tag = JET
, .jet = simplicity_left_shift_16
, .cmr = {{0xb0536018u, 0x4d0602b5u, 0x81405e32u, 0x960b31c0u, 0x5219358du, 0xe89efdf4u, 0x9464723du, 0xd625617au}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 72 /* milli weight units */
}
,[LEFT_SHIFT_32] =
{ .tag = JET
, .jet = simplicity_left_shift_32
, .cmr = {{0x34bf54f5u, 0x94c26210u, 0x07f8c78bu, 0x30fad396u, 0x72009bb3u, 0x66aace1eu, 0x5e41ee4du, 0x9cc541a8u}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 78 /* milli weight units */
}
,[LEFT_SHIFT_64] =
{ .tag = JET
, .jet = simplicity_left_shift_64
, .cmr = {{0x5de953f0u, 0x4deaed90u, 0x47567647u, 0xa1eb7abeu, 0x665feccbu, 0xe7ed10cbu, 0x7dbe6912u, 0x73c094b0u}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 82 /* milli weight units */
}
,[LEFT_SHIFT_8] =
{ .tag = JET
, .jet = simplicity_left_shift_8
, .cmr = {{0xab9d3e9au, 0xc39038adu, 0x88b103f0u, 0x72254c0eu, 0xc6e27475u, 0xe275c245u, 0xe88cce0du, 0x072e6446u}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 91 /* milli weight units */
}
,[LEFT_SHIFT_WITH_16] =
{ .tag = JET
, .jet = simplicity_left_shift_with_16
, .cmr = {{0xe2910788u, 0x5550450eu, 0xb727d0cfu, 0x14e104aeu, 0x12f83a24u, 0xe2e2aca3u, 0xcce433deu, 0x2f35d7b3u}}
, .sourceIx = ty_pbpw4w16
, .targetIx = ty_w16
, .cost = 83 /* milli weight units */
}
,[LEFT_SHIFT_WITH_32] =
{ .tag = JET
, .jet = simplicity_left_shift_with_32
, .cmr = {{0xf39250c4u, 0x5a1310ccu, 0x638c788du, 0xeec5c365u, 0xb4d176d1u, 0x0efbf4c6u, 0x01cf5eebu, 0xe0a573e9u}}
, .sourceIx = ty_pbpw8w32
, .targetIx = ty_w32
, .cost = 95 /* milli weight units */
}
,[LEFT_SHIFT_WITH_64] =
{ .tag = JET
, .jet = simplicity_left_shift_with_64
, .cmr = {{0xad8794cfu, 0xaef2b7f7u, 0x74fa68d3u, 0x09bbc98du, 0xfee58c40u, 0x400b2eb5u, 0x78a212f4u, 0x38bd07abu}}
, .sourceIx = ty_pbpw8w64
, .targetIx = ty_w64
, .cost = 103 /* milli weight units */
}
,[LEFT_SHIFT_WITH_8] =
{ .tag = JET
, .jet = simplicity_left_shift_with_8
, .cmr = {{0xdd9cc1ceu, 0xa7490948u, 0x1ff58f87u, 0x6ff66e0fu, 0x5d52bf89u, 0xb0258fa9u, 0x5b320002u, 0xc32a7915u}}
, .sourceIx = ty_pbpw4w8
, .targetIx = ty_w8
, .cost = 107 /* milli weight units */
}
,[LEFTMOST_16_1] =
{ .tag = JET
, .jet = simplicity_leftmost_16_1
, .cmr = {{0xde6a4c98u, 0x337e680du, 0x6e6ee2bfu, 0x36d3a081u, 0x7d2a9a98u, 0x325f87e5u, 0xeceb8a6fu, 0x1168f5cau}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 93 /* milli weight units */
}
,[LEFTMOST_16_2] =
{ .tag = JET
, .jet = simplicity_leftmost_16_2
, .cmr = {{0x005809b8u, 0x051a2a50u, 0x2833b22cu, 0x2c17981eu, 0xaf9dd1d3u, 0xdbc8f8c8u, 0x94516c1du, 0x5f31146cu}}
, .sourceIx = ty_w16
, .targetIx = ty_w2
, .cost = 90 /* milli weight units */
}
,[LEFTMOST_16_4] =
{ .tag = JET
, .jet = simplicity_leftmost_16_4
, .cmr = {{0x9c50ee22u, 0x84d857c4u, 0x7c054447u, 0x1354105eu, 0x98dfe027u, 0x54d2e42du, 0xe11d3234u, 0xed10b642u}}
, .sourceIx = ty_w16
, .targetIx = ty_w4
, .cost = 75 /* milli weight units */
}
,[LEFTMOST_16_8] =
{ .tag = JET
, .jet = simplicity_leftmost_16_8
, .cmr = {{0x5a1a7291u, 0x4e149c22u, 0xb464c8f6u, 0xa3d9cf41u, 0xb07192beu, 0xf0d8a1ccu, 0x7cbe5704u, 0xa9e8ea70u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 71 /* milli weight units */
}
,[LEFTMOST_32_1] =
{ .tag = JET
, .jet = simplicity_leftmost_32_1
, .cmr = {{0x5fb8e634u, 0x2ab74ee2u, 0xc9225b87u, 0x2fa0c912u, 0x046a69dbu, 0xb719bcd6u, 0xc8d79b76u, 0x60c4ebcau}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 77 /* milli weight units */
}
,[LEFTMOST_32_16] =
{ .tag = JET
, .jet = simplicity_leftmost_32_16
, .cmr = {{0x54ae50b4u, 0x6b5b2e68u, 0xf536c01cu, 0x39617b0cu, 0xee42e1c4u, 0x9a2cd1d2u, 0x6af8ea87u, 0x15ac4d11u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 102 /* milli weight units */
}
,[LEFTMOST_32_2] =
{ .tag = JET
, .jet = simplicity_leftmost_32_2
, .cmr = {{0x12306855u, 0x4595427eu, 0x3c1de243u, 0xbab66f33u, 0x48368aaau, 0x44617d6au, 0x02479fb7u, 0x04bcfd1eu}}
, .sourceIx = ty_w32
, .targetIx = ty_w2
, .cost = 66 /* milli weight units */
}
,[LEFTMOST_32_4] =
{ .tag = JET
, .jet = simplicity_leftmost_32_4
, .cmr = {{0x55a87b66u, 0xc339e363u, 0xe03d4daau, 0xc62290ebu, 0xa93c1a3au, 0x7382cbf6u, 0x1f20b34au, 0x505124adu}}
, .sourceIx = ty_w32
, .targetIx = ty_w4
, .cost = 52 /* milli weight units */
}
,[LEFTMOST_32_8] =
{ .tag = JET
, .jet = simplicity_leftmost_32_8
, .cmr = {{0x9f345beeu, 0x0b162d42u, 0xa035718fu, 0x8ca1adc8u, 0xac2f710du, 0xc40052a8u, 0x2566e6d8u, 0x07bef8b8u}}
, .sourceIx = ty_w32
, .targetIx = ty_w8
, .cost = 103 /* milli weight units */
}
,[LEFTMOST_64_1] =
{ .tag = JET
, .jet = simplicity_leftmost_64_1
, .cmr = {{0xb924d33bu, 0x5efefc8eu, 0x20420819u, 0x25917cffu, 0x239b31c8u, 0xbdbdf4acu, 0xae6bb8d9u, 0xcd217b4fu}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 78 /* milli weight units */
}
,[LEFTMOST_64_16] =
{ .tag = JET
, .jet = simplicity_leftmost_64_16
, .cmr = {{0x12aa85e0u, 0x5c1e9622u, 0x279c4c2du, 0xdcf897c9u, 0x5ddcc011u, 0x3997283bu, 0x6b3e0949u, 0xbc8113cbu}}
, .sourceIx = ty_w64
, .targetIx = ty_w16
, .cost = 88 /* milli weight units */
}
,[LEFTMOST_64_2] =
{ .tag = JET
, .jet = simplicity_leftmost_64_2
, .cmr = {{0xbcec97f4u, 0x3ba55cd4u, 0x0d85a1e7u, 0x6cbade7bu, 0x0b1e9f13u, 0x9747793du, 0xcb3480beu, 0xe1f751cau}}
, .sourceIx = ty_w64
, .targetIx = ty_w2
, .cost = 71 /* milli weight units */
}
,[LEFTMOST_64_32] =
{ .tag = JET
, .jet = simplicity_leftmost_64_32
, .cmr = {{0x9c896939u, 0x86e55733u, 0xab962a30u, 0x0b057950u, 0x3d83de8au, 0xc19b179bu, 0x417e1ca2u, 0x5385b38fu}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 90 /* milli weight units */
}
,[LEFTMOST_64_4] =
{ .tag = JET
, .jet = simplicity_leftmost_64_4
, .cmr = {{0xd2d64520u, 0x92d6566fu, 0x89a1f64eu, 0x736596f9u, 0x000e5e6fu, 0x63e40017u, 0xd0cb80f3u, 0xf7adfd18u}}
, .sourceIx = ty_w64
, .targetIx = ty_w4
, .cost = 79 /* milli weight units */
}
,[LEFTMOST_64_8] =
{ .tag = JET
, .jet = simplicity_leftmost_64_8
, .cmr = {{0xf77b62bbu, 0x01b90511u, 0xb6d06ebfu, 0x2e36c065u, 0x65acb5aau, 0xd1efc77cu, 0x36a10a26u, 0x1de921dau}}
, .sourceIx = ty_w64
, .targetIx = ty_w8
, .cost = 86 /* milli weight units */
}
,[LEFTMOST_8_1] =
{ .tag = JET
, .jet = simplicity_leftmost_8_1
, .cmr = {{0x5a730b58u, 0xe3abcb2fu, 0x4de22159u, 0x80302310u, 0x2cd66421u, 0x911920cau, 0x21a2a05cu, 0x9b211ce8u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 90 /* milli weight units */
}
,[LEFTMOST_8_2] =
{ .tag = JET
, .jet = simplicity_leftmost_8_2
, .cmr = {{0x25790856u, 0x103dce6cu, 0x7bbb3dd7u, 0x18b16910u, 0x9cae8537u, 0x99d12456u, 0xc85d8349u, 0xec18dc53u}}
, .sourceIx = ty_w8
, .targetIx = ty_w2
, .cost = 90 /* milli weight units */
}
,[LEFTMOST_8_4] =
{ .tag = JET
, .jet = simplicity_leftmost_8_4
, .cmr = {{0x73d9f018u, 0x157a1478u, 0x4ee70b21u, 0x9ceb4042u, 0xfa621d0eu, 0xe6d545a0u, 0xfdbab944u, 0x4346e331u}}
, .sourceIx = ty_w8
, .targetIx = ty_w4
, .cost = 87 /* milli weight units */
}
,[LINEAR_COMBINATION_1] =
{ .tag = JET
, .jet = simplicity_linear_combination_1
, .cmr = {{0x6d9f4a87u, 0x0fbf740cu, 0x220efff3u, 0x07b5ed91u, 0xa58c5e51u, 0xa8adfc3bu, 0x159030f5u, 0x12d39941u}}
, .sourceIx = ty_ppw256pw512w256w256
, .targetIx = ty_pw512w256
, .cost = 84674 /* milli weight units */
}
,[LINEAR_VERIFY_1] =
{ .tag = JET
, .jet = simplicity_linear_verify_1
, .cmr = {{0x278313d7u, 0xce4ad589u, 0x11de24eeu, 0x540d19ecu, 0xebb62f4au, 0xb4a71e2au, 0xadd4512bu, 0x2e4bc2e2u}}
, .sourceIx = ty_pppw256w512w256w512
, .targetIx = ty_u
, .cost = 43364 /* milli weight units */
}
,[LOCK_TIME] =
{ .tag = JET
, .jet = simplicity_lock_time
, .cmr = {{0xa16897c5u, 0xdb4927c6u, 0xcb07b165u, 0xbd2cc38bu, 0x36a60268u, 0x6c74a190u, 0x34aa3787u, 0xac582a0bu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 85 /* milli weight units */
}
,[LOW_1] =
{ .tag = JET
, .jet = simplicity_low_1
, .cmr = {{0xf27b69bbu, 0x091609f5u, 0x9e003305u, 0x0d01a5bcu, 0x77ff07d9u, 0x42707a79u, 0xcf5ee410u, 0xa998a043u}}
, .sourceIx = ty_u
, .targetIx = ty_b
, .cost = 38 /* milli weight units */
}
,[LOW_16] =
{ .tag = JET
, .jet = simplicity_low_16
, .cmr = {{0x977cbd1eu, 0x7ffc05e7u, 0x16d9c1b4u, 0x9f7d517fu, 0x853dbf3eu, 0x98a4c748u, 0x046eacf4u, 0x17f89c2fu}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 69 /* milli weight units */
}
,[LOW_32] =
{ .tag = JET
, .jet = simplicity_low_32
, .cmr = {{0xbe416982u, 0x8f076778u, 0xb60d5456u, 0xf7886ed7u, 0xf30b101du, 0x6ccbd9eau, 0x0c4db142u, 0xeac66b12u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 62 /* milli weight units */
}
,[LOW_64] =
{ .tag = JET
, .jet = simplicity_low_64
, .cmr = {{0x019a66bfu, 0xba175168u, 0x8be71389u, 0xed7bf371u, 0xb3014dfbu, 0x329562acu, 0x3b3e9dfeu, 0x9206a5bcu}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 47 /* milli weight units */
}
,[LOW_8] =
{ .tag = JET
, .jet = simplicity_low_8
, .cmr = {{0x217b5643u, 0x956b4833u, 0xaa5622f0u, 0x0f0688bau, 0x860d4adbu, 0xf940cbdcu, 0xd2b59f26u, 0xd61593b1u}}
, .sourceIx = ty_u
, .targetIx = ty_w8
, .cost = 47 /* milli weight units */
}
,[LT_16] =
{ .tag = JET
, .jet = simplicity_lt_16
, .cmr = {{0x56a20d55u, 0xedb44388u, 0x180544c3u, 0xed404145u, 0xa3b66fd2u, 0xc4113842u, 0xf64eaeafu, 0xbad4bb06u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 123 /* milli weight units */
}
,[LT_32] =
{ .tag = JET
, .jet = simplicity_lt_32
, .cmr = {{0xcab0dc5bu, 0x0ecbf6d2u, 0x4816fc20u, 0x10fc3119u, 0x3663c306u, 0x968d9ceeu, 0x3b004c0bu, 0xc184b478u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 107 /* milli weight units */
}
,[LT_64] =
{ .tag = JET
, .jet = simplicity_lt_64
, .cmr = {{0x47d67e52u, 0xb27ba78eu, 0xdd075aa2u, 0x70ded007u, 0xa7a9a684u, 0x99344f28u, 0x62f50690u, 0x49a0cefeu}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 76 /* milli weight units */
}
,[LT_8] =
{ .tag = JET
, .jet = simplicity_lt_8
, .cmr = {{0x73d00446u, 0x55c0df45u, 0xc271a171u, 0x3ff9b9a4u, 0x3dde56e6u, 0x74d1754eu, 0x76edb16fu, 0x949c4fabu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 107 /* milli weight units */
}
,[MAJ_1] =
{ .tag = JET
, .jet = simplicity_maj_1
, .cmr = {{0xcb2d986du, 0x7f00107au, 0x3c25f6b2u, 0xf14891d0u, 0x2e20ae16u, 0xf0a1252cu, 0x92d9b58au, 0xe73388aau}}
, .sourceIx = ty_pbw2
, .targetIx = ty_b
, .cost = 62 /* milli weight units */
}
,[MAJ_16] =
{ .tag = JET
, .jet = simplicity_maj_16
, .cmr = {{0x0af6d0c1u, 0x71fe33a2u, 0x159bf988u, 0x00f0412cu, 0x2597e997u, 0x84d074fdu, 0xfa33d7fdu, 0xe597ddfdu}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 80 /* milli weight units */
}
,[MAJ_32] =
{ .tag = JET
, .jet = simplicity_maj_32
, .cmr = {{0x3060838du, 0x48456f33u, 0x92d5d69bu, 0x5eec0892u, 0x76cd58bbu, 0x67a12c64u, 0x2ec73aebu, 0x9adacbddu}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 96 /* milli weight units */
}
,[MAJ_64] =
{ .tag = JET
, .jet = simplicity_maj_64
, .cmr = {{0x8ebcc174u, 0x57ea2b14u, 0x231b0e90u, 0x1ea7b1d4u, 0x7b9b7898u, 0x6372a441u, 0x6fe73f67u, 0x63feb24bu}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 93 /* milli weight units */
}
,[MAJ_8] =
{ .tag = JET
, .jet = simplicity_maj_8
, .cmr = {{0x8930d1d0u, 0x991b0a58u, 0x1d0b1d85u, 0xad72147du, 0x6649a359u, 0x93283fc9u, 0x7214431fu, 0x0b6a7aa8u}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 94 /* milli weight units */
}
,[MAX_16] =
{ .tag = JET
, .jet = simplicity_max_16
, .cmr = {{0xe0114717u, 0x691ac1a7u, 0x39288fc6u, 0xffa1c650u, 0x7c43e6f1u, 0xd4c18770u, 0xffa166aeu, 0x839dd533u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 114 /* milli weight units */
}
,[MAX_32] =
{ .tag = JET
, .jet = simplicity_max_32
, .cmr = {{0x1d723cb3u, 0x89942219u, 0xec103485u, 0x317fa5d8u, 0x7ee15c24u, 0xb2080f50u, 0x46650d80u, 0x308b189du}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 92 /* milli weight units */
}
,[MAX_64] =
{ .tag = JET
, .jet = simplicity_max_64
, .cmr = {{0x0073ac3cu, 0x6ea939dcu, 0xc7eee4eau, 0x63dcfd75u, 0x2037355bu, 0x484f6e70u, 0x16b300e2u, 0xd28c07c3u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 104 /* milli weight units */
}
,[MAX_8] =
{ .tag = JET
, .jet = simplicity_max_8
, .cmr = {{0x6bc10370u, 0xf3e7a7b9u, 0x2acb1423u, 0xbbdf0b3du, 0x7e3cd0d2u, 0xdbc705a3u, 0x4d8dc99cu, 0x910422fbu}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 96 /* milli weight units */
}
,[MEDIAN_16] =
{ .tag = JET
, .jet = simplicity_median_16
, .cmr = {{0x2414e3c4u, 0x39659d8au, 0xa9d087e1u, 0xade77266u, 0x673d1c8bu, 0xd4e7501bu, 0x22ac46a3u, 0xff39975du}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 123 /* milli weight units */
}
,[MEDIAN_32] =
{ .tag = JET
, .jet = simplicity_median_32
, .cmr = {{0x0792356bu, 0x610b57d0u, 0xec199e98u, 0x535ea9bcu, 0xcce843a5u, 0xdf5dd240u, 0x8c414886u, 0xdfd6bd1eu}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 101 /* milli weight units */
}
,[MEDIAN_64] =
{ .tag = JET
, .jet = simplicity_median_64
, .cmr = {{0x0766d89bu, 0x430ffdf0u, 0x38691b18u, 0x439cd6fcu, 0x4929172eu, 0xa884fdafu, 0x166936b3u, 0x8b15fd0cu}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 109 /* milli weight units */
}
,[MEDIAN_8] =
{ .tag = JET
, .jet = simplicity_median_8
, .cmr = {{0xa4a0b631u, 0x0ff0ed4au, 0x4c3e03ebu, 0xc7a91306u, 0xef660424u, 0xbc95a0d3u, 0xf2fdb71fu, 0xb6afd8b7u}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 122 /* milli weight units */
}
,[MIN_16] =
{ .tag = JET
, .jet = simplicity_min_16
, .cmr = {{0xf158f40au, 0x860993b4u, 0x107fb271u, 0xfb4c8f95u, 0x5ba4542au, 0xd1821cd2u, 0xf13c880cu, 0xa4bee2e2u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 97 /* milli weight units */
}
,[MIN_32] =
{ .tag = JET
, .jet = simplicity_min_32
, .cmr = {{0xe5e413dcu, 0x5de5e22du, 0x66f32d8du, 0xbf50053eu, 0xd278e175u, 0xc0d4b344u, 0xebd461beu, 0xb108e55eu}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 113 /* milli weight units */
}
,[MIN_64] =
{ .tag = JET
, .jet = simplicity_min_64
, .cmr = {{0x43d82f6cu, 0x6128aa01u, 0xa997bb17u, 0xe5e7f501u, 0xe7be7db9u, 0x589e566du, 0xe97a32eau, 0xe7e7b339u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 102 /* milli weight units */
}
,[MIN_8] =
{ .tag = JET
, .jet = simplicity_min_8
, .cmr = {{0x6b012ca3u, 0x185dc005u, 0xe8942cfbu, 0xc9f238dcu, 0xedaf0c00u, 0x43526447u, 0xe3ec31ceu, 0xfa6e4064u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 99 /* milli weight units */
}
,[MODULO_16] =
{ .tag = JET
, .jet = simplicity_modulo_16
, .cmr = {{0x62c179acu, 0x84c5750bu, 0x425f9a1bu, 0x8f81edaau, 0x7f5cf22cu, 0x19d86b0du, 0xcf96dea6u, 0xbad99b3bu}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 103 /* milli weight units */
}
,[MODULO_32] =
{ .tag = JET
, .jet = simplicity_modulo_32
, .cmr = {{0xa1f01c10u, 0x6fc36a76u, 0x4e99b233u, 0x98e21e7cu, 0x267f889fu, 0xccebd148u, 0x7d3de1ccu, 0x67c32bd9u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 102 /* milli weight units */
}
,[MODULO_64] =
{ .tag = JET
, .jet = simplicity_modulo_64
, .cmr = {{0x50c82fd0u, 0x3109c98bu, 0x7237e916u, 0x74041964u, 0x381e6c2eu, 0xbbe25bf3u, 0xe0d37a9fu, 0x060f1502u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 85 /* milli weight units */
}
,[MODULO_8] =
{ .tag = JET
, .jet = simplicity_modulo_8
, .cmr = {{0x5c63c77au, 0x1608e2f6u, 0xa3748c11u, 0x0fbb9a1cu, 0x569fb4d5u, 0x40f3dd2eu, 0x4f80e90du, 0xd5ea9982u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 102 /* milli weight units */
}
,[MULTIPLY_16] =
{ .tag = JET
, .jet = simplicity_multiply_16
, .cmr = {{0x46e62abfu, 0x8e30a774u, 0x6de0e929u, 0xf7beeddbu, 0xde8b269bu, 0xab08f76eu, 0x9547108bu, 0x1c360174u}}
, .sourceIx = ty_w32
, .targetIx = ty_w32
, .cost = 90 /* milli weight units */
}
,[MULTIPLY_32] =
{ .tag = JET
, .jet = simplicity_multiply_32
, .cmr = {{0x2decdc5bu, 0x0c6ff63du, 0x11f53852u, 0xe0deed11u, 0x4481355bu, 0xb6c6ce15u, 0x46ae9f81u, 0x5bee7750u}}
, .sourceIx = ty_w64
, .targetIx = ty_w64
, .cost = 90 /* milli weight units */
}
,[MULTIPLY_64] =
{ .tag = JET
, .jet = simplicity_multiply_64
, .cmr = {{0xbfa8626du, 0xbf10001du, 0xe390d997u, 0xf2ee7b19u, 0x0c24a78cu, 0xfecb91f5u, 0xd7c10c3fu, 0x9ddbb1e6u}}
, .sourceIx = ty_w128
, .targetIx = ty_w128
, .cost = 85 /* milli weight units */
}
,[MULTIPLY_8] =
{ .tag = JET
, .jet = simplicity_multiply_8
, .cmr = {{0x29da1337u, 0x4f7cb308u, 0x405fe230u, 0xf899485cu, 0x500e6e95u, 0x20c15e8au, 0x76e53a92u, 0xe7ac64d6u}}
, .sourceIx = ty_w16
, .targetIx = ty_w16
, .cost = 93 /* milli weight units */
}
,[NEGATE_16] =
{ .tag = JET
, .jet = simplicity_negate_16
, .cmr = {{0xf642173bu, 0x85ef2196u, 0x9d8d9048u, 0x807e3d4fu, 0xacf3f5f9u, 0xe59aa5cfu, 0x0c60f874u, 0x22ed7c8fu}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw16
, .cost = 70 /* milli weight units */
}
,[NEGATE_32] =
{ .tag = JET
, .jet = simplicity_negate_32
, .cmr = {{0x549b65ceu, 0x97c6b334u, 0xb8ae9456u, 0x960e365bu, 0xb284d76du, 0x4005e921u, 0xf489bc36u, 0x26171b06u}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw32
, .cost = 85 /* milli weight units */
}
,[NEGATE_64] =
{ .tag = JET
, .jet = simplicity_negate_64
, .cmr = {{0x35acca27u, 0xce658579u, 0xef1c55adu, 0x1abea005u, 0x0d9366d1u, 0x2209ad13u, 0x052549c3u, 0x436491d0u}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw64
, .cost = 94 /* milli weight units */
}
,[NEGATE_8] =
{ .tag = JET
, .jet = simplicity_negate_8
, .cmr = {{0xd871c542u, 0x473f4dd9u, 0x02d31fe3u, 0xfc9ac0f3u, 0x319e42e8u, 0x0cae2181u, 0xffc85e6cu, 0x60fb0988u}}
, .sourceIx = ty_w8
, .targetIx = ty_pbw8
, .cost = 91 /* milli weight units */
}
,[NEW_ISSUANCE_CONTRACT] =
{ .tag = JET
, .jet = simplicity_new_issuance_contract
, .cmr = {{0xdb64808cu, 0x3ff44880u, 0xd72bc295u, 0xd9ac0643u, 0xe51404deu, 0x5398ad9eu, 0x931bd3d2u, 0xadbca6c4u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 157 /* milli weight units */
}
,[NONCE_HASH] =
{ .tag = JET
, .jet = simplicity_nonce_hash
, .cmr = {{0xfb1e0344u, 0xdcdaf9dbu, 0x91b987e8u, 0xf9661067u, 0x9f05deb2u, 0x06b40105u, 0x7502a484u, 0xaa87f8bfu}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256mspbw256w256
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 317 /* milli weight units */
}
,[NUM_INPUTS] =
{ .tag = JET
, .jet = simplicity_num_inputs
, .cmr = {{0xb2288ebau, 0xadcbcfceu, 0x1c631964u, 0xc86b127du, 0x6f91dc65u, 0x7c59a7fbu, 0x453e9111u, 0xd87481f5u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 86 /* milli weight units */
}
,[NUM_OUTPUTS] =
{ .tag = JET
, .jet = simplicity_num_outputs
, .cmr = {{0x267d4165u, 0x4493903eu, 0x0257751au, 0xe6d42d9cu, 0xe1bb9224u, 0x6d0ef767u, 0xc337b0c3u, 0x2b1972fdu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 79 /* milli weight units */
}
,[ONE_16] =
{ .tag = JET
, .jet = simplicity_one_16
, .cmr = {{0x3f9f8dd1u, 0x4c46ee02u, 0x47155792u, 0x9ac2bb6cu, 0x1aca0052u, 0x1d8afaf0u, 0xdcd9f2cau, 0x7f31e604u}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 60 /* milli weight units */
}
,[ONE_32] =
{ .tag = JET
, .jet = simplicity_one_32
, .cmr = {{0x478dc39du, 0xc3995e2eu, 0xdb7ec674u, 0x656cae79u, 0x8f52e572u, 0x926174a6u, 0x68cc97bcu, 0xa448d1ccu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 59 /* milli weight units */
}
,[ONE_64] =
{ .tag = JET
, .jet = simplicity_one_64
, .cmr = {{0xa392cefcu, 0x0da53c65u, 0xaee612f5u, 0xc6816ca8u, 0x92fc156du, 0x43714876u, 0xb3a00568u, 0xe1ba3ebau}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 59 /* milli weight units */
}
,[ONE_8] =
{ .tag = JET
, .jet = simplicity_one_8
, .cmr = {{0xff594e22u, 0xbfd75813u, 0xc056e0a2u, 0x34ed12fau, 0x8287d1d5u, 0x316f2390u, 0x2bf079dbu, 0xcc4f4ea8u}}
, .sourceIx = ty_u
, .targetIx = ty_w8
, .cost = 62 /* milli weight units */
}
,[OR_1] =
{ .tag = JET
, .jet = simplicity_or_1
, .cmr = {{0x9bf59174u, 0x410a809du, 0x3da2b58cu, 0x7e0d05c5u, 0x5cec38bdu, 0xaa5fcac3u, 0x82a31177u, 0x0ee0eb38u}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 77 /* milli weight units */
}
,[OR_16] =
{ .tag = JET
, .jet = simplicity_or_16
, .cmr = {{0xdd9a3193u, 0xd619d959u, 0xfa0b6d8bu, 0x47af7854u, 0xf7e0467bu, 0xa35901ceu, 0x43d800fcu, 0xaf730ff9u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 94 /* milli weight units */
}
,[OR_32] =
{ .tag = JET
, .jet = simplicity_or_32
, .cmr = {{0x9a019f07u, 0xdf4996b3u, 0x3e647f4du, 0xe7e56c1du, 0x8f03269cu, 0xbfa3c758u, 0x2cfe808eu, 0x909870b7u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 105 /* milli weight units */
}
,[OR_64] =
{ .tag = JET
, .jet = simplicity_or_64
, .cmr = {{0xc24f3580u, 0x05f80377u, 0x2b1c3e43u, 0x9cf1b709u, 0xbd9f4d42u, 0x52759130u, 0x3a36f6b1u, 0xc3cf29ccu}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 99 /* milli weight units */
}
,[OR_8] =
{ .tag = JET
, .jet = simplicity_or_8
, .cmr = {{0x84b53689u, 0xf21d4e69u, 0x7d0fe898u, 0x8ce736abu, 0x72c9c86fu, 0x847589dau, 0xa9ae6a78u, 0x4630e620u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 93 /* milli weight units */
}
,[OUTPOINT_HASH] =
{ .tag = JET
, .jet = simplicity_outpoint_hash
, .cmr = {{0x8d2a5aa0u, 0x6e98c41bu, 0x30fd0703u, 0xe5e54c86u, 0xb42bee33u, 0xbc89cb60u, 0x93cdb094u, 0x05a72d9du}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256pmw256pw256w32
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 319 /* milli weight units */
}
,[OUTPUT_AMOUNT] =
{ .tag = JET
, .jet = simplicity_output_amount
, .cmr = {{0x55e41399u, 0x2169408eu, 0x338e11a8u, 0xd7be602bu, 0x3ec9511eu, 0x7eb691dfu, 0x32d9797au, 0x6395a1a1u}}
, .sourceIx = ty_w32
, .targetIx = ty_mpspbw256w256spbw256w64
, .cost = 298 /* milli weight units */
}
,[OUTPUT_AMOUNTS_HASH] =
{ .tag = JET
, .jet = simplicity_output_amounts_hash
, .cmr = {{0x40236963u, 0xe10ef8adu, 0x9cb6e0a7u, 0xc79ac866u, 0x0d2fc1a5u, 0x7feff885u, 0x15a78527u, 0x91b186cbu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 140 /* milli weight units */
}
,[OUTPUT_ASSET] =
{ .tag = JET
, .jet = simplicity_output_asset
, .cmr = {{0x575a49bau, 0xb21ffee8u, 0xe5bd00edu, 0xce324061u, 0x04a05ddeu, 0xf5fc31a1u, 0xa645b129u, 0x548c8f3eu}}
, .sourceIx = ty_w32
, .targetIx = ty_mspbw256w256
, .cost = 170 /* milli weight units */
}
,[OUTPUT_HASH] =
{ .tag = JET
, .jet = simplicity_output_hash
, .cmr = {{0x7cb17f8fu, 0xc7a1ae4eu, 0xfce30a14u, 0x54e52f85u, 0x85213cd0u, 0xf367a127u, 0xac27bb97u, 0x66ea9eeeu}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 2849 /* milli weight units */
}
,[OUTPUT_IS_FEE] =
{ .tag = JET
, .jet = simplicity_output_is_fee
, .cmr = {{0x9fbbe8aeu, 0xbdedd577u, 0xae466280u, 0xa99b67b7u, 0x343287efu, 0x34069253u, 0x4a009803u, 0x15e51772u}}
, .sourceIx = ty_w32
, .targetIx = ty_mb
, .cost = 92 /* milli weight units */
}
,[OUTPUT_NONCE] =
{ .tag = JET
, .jet = simplicity_output_nonce
, .cmr = {{0x7175fd99u, 0xa2baa1c3u, 0xafaaf7fau, 0xbdaa67c2u, 0xc827483bu, 0xc8a0b976u, 0x50cb7942u, 0xea295a18u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmspbw256w256
, .cost = 196 /* milli weight units */
}
,[OUTPUT_NONCES_HASH] =
{ .tag = JET
, .jet = simplicity_output_nonces_hash
, .cmr = {{0x50f18766u, 0xde9828dbu, 0x0a8c41d7u, 0xbeb840f7u, 0x7bc6e121u, 0xff123d54u, 0x96f1ea0du, 0x6a78b83au}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 151 /* milli weight units */
}
,[OUTPUT_NULL_DATUM] =
{ .tag = JET
, .jet = simplicity_output_null_datum
, .cmr = {{0x38a5a7e8u, 0x61b3b36cu, 0xce683a77u, 0x2db0c862u, 0x8b7fd3b4u, 0xcf8e7bceu, 0x70758fa6u, 0xf6f147bfu}}
, .sourceIx = ty_w64
, .targetIx = ty_mmspw2w256sbw4
, .cost = 87 /* milli weight units */
}
,[OUTPUT_RANGE_PROOF] =
{ .tag = JET
, .jet = simplicity_output_range_proof
, .cmr = {{0x6ff9ef3du, 0xac504310u, 0xdfbc6056u, 0x1d2addebu, 0x44932f77u, 0x7e7f13adu, 0xf0774eb6u, 0xb14df966u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 154 /* milli weight units */
}
,[OUTPUT_RANGE_PROOFS_HASH] =
{ .tag = JET
, .jet = simplicity_output_range_proofs_hash
, .cmr = {{0x0d7d8ac9u, 0x7af9539au, 0xbba64d81u, 0x04d17e97u, 0x0854677cu, 0x27461d25u, 0xdf21a7f6u, 0xb662081eu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 136 /* milli weight units */
}
,[OUTPUT_SCRIPT_HASH] =
{ .tag = JET
, .jet = simplicity_output_script_hash
, .cmr = {{0x2f513926u, 0x6f143a33u, 0xe1668e59u, 0x4f43e8d1u, 0x394407e3u, 0xa9b8b802u, 0xb39a5e1eu, 0xdc54e851u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 151 /* milli weight units */
}
,[OUTPUT_SCRIPTS_HASH] =
{ .tag = JET
, .jet = simplicity_output_scripts_hash
, .cmr = {{0x21333ac9u, 0x58a4ebfdu, 0x5eab7956u, 0xdb73ed03u, 0x269426afu, 0x8d7d2498u, 0xde57eef6u, 0x8704fb0au}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 142 /* milli weight units */
}
,[OUTPUT_SURJECTION_PROOF] =
{ .tag = JET
, .jet = simplicity_output_surjection_proof
, .cmr = {{0x09166008u, 0x1d52c33fu, 0x17a956bdu, 0xdac62aa2u, 0xd2ed928fu, 0x8cffef4cu, 0xf8dbb697u, 0xb153aa65u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 151 /* milli weight units */
}
,[OUTPUT_SURJECTION_PROOFS_HASH] =
{ .tag = JET
, .jet = simplicity_output_surjection_proofs_hash
, .cmr = {{0x3630b520u, 0x8302641bu, 0xf19c4bf5u, 0xca047ac3u, 0xeb7af54du, 0x5d3f5bf8u, 0xf1d41312u, 0x499d6384u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 138 /* milli weight units */
}
,[OUTPUTS_HASH] =
{ .tag = JET
, .jet = simplicity_outputs_hash
, .cmr = {{0x95b3511au, 0xea00d043u, 0xa4e028b2u, 0x5a171c74u, 0x2ec2ff7au, 0xd3a7df2fu, 0x1f6edad0u, 0x8bad2b69u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 135 /* milli weight units */
}
,[PARSE_LOCK] =
{ .tag = JET
, .jet = simplicity_parse_lock
, .cmr = {{0x3d3836fdu, 0x3085c1fbu, 0xac6cd5fau, 0x0dbf4a3fu, 0xb2554593u, 0x17a266d6u, 0xd6f7382bu, 0xb05f07adu}}
, .sourceIx = ty_w32
, .targetIx = ty_sw32w32
, .cost = 97 /* milli weight units */
}
,[PARSE_SEQUENCE] =
{ .tag = JET
, .jet = simplicity_parse_sequence
, .cmr = {{0x74f35c01u, 0x9ef514b7u, 0x0ab008bfu, 0x2a126de7u, 0xe00f6e3cu, 0xcd285d51u, 0xdbd3ac71u, 0xbea9c88du}}
, .sourceIx = ty_w32
, .targetIx = ty_msw16w16
, .cost = 116 /* milli weight units */
}
,[POINT_VERIFY_1] =
{ .tag = JET
, .jet = simplicity_point_verify_1
, .cmr = {{0x90a3d669u, 0xb00da795u, 0xefb2bed8u, 0xc370c9e3u, 0xea0f19c4u, 0x1c7cf23eu, 0x492e3317u, 0x1a47f5ffu}}
, .sourceIx = ty_pppw256pbw256w256pbw256
, .targetIx = ty_u
, .cost = 41494 /* milli weight units */
}
,[REISSUANCE_BLINDING] =
{ .tag = JET
, .jet = simplicity_reissuance_blinding
, .cmr = {{0xeb6e581eu, 0x20ba0babu, 0xe904c8dau, 0x71a81f7cu, 0x88b9cf4fu, 0x42d78b54u, 0x0c1b398cu, 0x3f105cccu}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 91 /* milli weight units */
}
,[REISSUANCE_ENTROPY] =
{ .tag = JET
, .jet = simplicity_reissuance_entropy
, .cmr = {{0xd1d22c49u, 0x7f129c6du, 0x6fdbe491u, 0x364a3513u, 0xbcd66e15u, 0x618cbbe2u, 0xe50739bdu, 0x834db7f8u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 93 /* milli weight units */
}
,[RIGHT_EXTEND_16_32] =
{ .tag = JET
, .jet = simplicity_right_extend_16_32
, .cmr = {{0x780716d3u, 0xe8291a51u, 0xe45ada50u, 0x558efe41u, 0x1c475c08u, 0x5eec5a28u, 0xad9791c3u, 0x12fee2bcu}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 74 /* milli weight units */
}
,[RIGHT_EXTEND_16_64] =
{ .tag = JET
, .jet = simplicity_right_extend_16_64
, .cmr = {{0xc770497eu, 0x452308ebu, 0xf52e51b0u, 0x585e9151u, 0xe0ffc350u, 0x86ab772du, 0x7241532au, 0x1be15e07u}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 82 /* milli weight units */
}
,[RIGHT_EXTEND_32_64] =
{ .tag = JET
, .jet = simplicity_right_extend_32_64
, .cmr = {{0x42b43adcu, 0x74b5266cu, 0x91d73df4u, 0x91dcae59u, 0x738804ebu, 0x440b23dau, 0x32753048u, 0x7486b7e8u}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 94 /* milli weight units */
}
,[RIGHT_EXTEND_8_16] =
{ .tag = JET
, .jet = simplicity_right_extend_8_16
, .cmr = {{0xbcb2683au, 0x8cb8b8c2u, 0x35faa896u, 0xa9c069e1u, 0xb55bb055u, 0x8e739e70u, 0xe2891421u, 0x1e3275c8u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 76 /* milli weight units */
}
,[RIGHT_EXTEND_8_32] =
{ .tag = JET
, .jet = simplicity_right_extend_8_32
, .cmr = {{0x6ddb5548u, 0xfd583cd2u, 0xd3586e6bu, 0x8bf99524u, 0x6b61934fu, 0x49764467u, 0x77dd5740u, 0xb319e462u}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 106 /* milli weight units */
}
,[RIGHT_EXTEND_8_64] =
{ .tag = JET
, .jet = simplicity_right_extend_8_64
, .cmr = {{0xda4f9c21u, 0x45512682u, 0x0758a2e4u, 0xb53fceb4u, 0x523e6e7au, 0x2923a1a1u, 0x61fc3789u, 0x2ac8da2au}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 124 /* milli weight units */
}
,[RIGHT_PAD_HIGH_16_32] =
{ .tag = JET
, .jet = simplicity_right_pad_high_16_32
, .cmr = {{0x3e4e5e9eu, 0x71e137a2u, 0x686343e0u, 0x5ac56316u, 0xacfc5899u, 0x1cb38db1u, 0xb3234413u, 0xf730a142u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 70 /* milli weight units */
}
,[RIGHT_PAD_HIGH_16_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_16_64
, .cmr = {{0xde09df9du, 0x43ddad2du, 0x69120498u, 0x6cf0819du, 0x6b8045bcu, 0xa414d80au, 0xf2162892u, 0xa9257eadu}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 88 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_16] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_16
, .cmr = {{0xff1297d8u, 0x78e26e19u, 0x59bcc7e8u, 0xaef97ac0u, 0xb65adc39u, 0x923ec650u, 0x5e50f983u, 0x05733b6cu}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 143 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_32] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_32
, .cmr = {{0x283f8afbu, 0x41382d2bu, 0xe18f8a77u, 0xc314ba17u, 0x76cb80c8u, 0xec36ca12u, 0xaa67b32bu, 0xb64ed843u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 223 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_64
, .cmr = {{0xa3423528u, 0x60a3350du, 0x79c3e9fcu, 0x7a4ab378u, 0x9b8b0297u, 0x856fd169u, 0xca4d7de2u, 0x5f7d7cc4u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 476 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_8] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_8
, .cmr = {{0x7103c0feu, 0x00f522a2u, 0x216c4a6bu, 0xe5f7e0ebu, 0x4d703ca7u, 0x8f9c598fu, 0x6b3dfde4u, 0x37d80c84u}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 107 /* milli weight units */
}
,[RIGHT_PAD_HIGH_32_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_32_64
, .cmr = {{0x5dc9107du, 0x4534958cu, 0xe4422767u, 0x563a031au, 0x380f60d3u, 0x837148abu, 0x3c8cc9c4u, 0xc7d996a2u}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 94 /* milli weight units */
}
,[RIGHT_PAD_HIGH_8_16] =
{ .tag = JET
, .jet = simplicity_right_pad_high_8_16
, .cmr = {{0xc0e2fd46u, 0xf7883b12u, 0x85a6f1a1u, 0xdb96d93cu, 0x2548040fu, 0xcd3f5c23u, 0xfbb20b5eu, 0x83037c96u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 89 /* milli weight units */
}
,[RIGHT_PAD_HIGH_8_32] =
{ .tag = JET
, .jet = simplicity_right_pad_high_8_32
, .cmr = {{0x291e6277u, 0x08520c2cu, 0xa6aece32u, 0xa877b778u, 0x49c4a7a2u, 0x13cb89e1u, 0xbda7c5c5u, 0xfe755f73u}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 110 /* milli weight units */
}
,[RIGHT_PAD_HIGH_8_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_8_64
, .cmr = {{0x6b6fa237u, 0x2ed25e4au, 0x34d4ae17u, 0x2342adbbu, 0x259be898u, 0x7600db19u, 0x2ecb8da4u, 0x34b9d88fu}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 107 /* milli weight units */
}
,[RIGHT_PAD_LOW_16_32] =
{ .tag = JET
, .jet = simplicity_right_pad_low_16_32
, .cmr = {{0x7731d560u, 0xd37592d1u, 0xa31f7362u, 0x967ab2e4u, 0x7592aca6u, 0xe92ab858u, 0x823792dau, 0xe5d2db52u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 71 /* milli weight units */
}
,[RIGHT_PAD_LOW_16_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_16_64
, .cmr = {{0x0fe1c0dbu, 0x9d4a2d63u, 0xe2ba4a33u, 0x117aadbau, 0x64514a2bu, 0x87a7a4e7u, 0x93faacfeu, 0x6b363447u}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 96 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_16] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_16
, .cmr = {{0x7914c8f2u, 0x2247c2c3u, 0x4b9c84e9u, 0x2d1444aeu, 0xc2e17a0eu, 0xf586bab2u, 0x788ee6efu, 0x68840d98u}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 81 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_32] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_32
, .cmr = {{0x31b6ce26u, 0xe559f76cu, 0xf366f480u, 0x6985ecc2u, 0x99550f15u, 0xd4c3a672u, 0x9e29d70eu, 0x39895652u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 75 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_64
, .cmr = {{0xc5524ae6u, 0x548acd63u, 0x082d9489u, 0x3e18f9edu, 0xbb9231e7u, 0x6bb4e11bu, 0xbff6a7bdu, 0x16f4b029u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 73 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_8] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_8
, .cmr = {{0x59d72270u, 0xef0e8f77u, 0x0c8d11f3u, 0x1773f9b6u, 0xe90a4aecu, 0xeb5bfb3du, 0xfe968c4eu, 0x9dac5fe8u}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 68 /* milli weight units */
}
,[RIGHT_PAD_LOW_32_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_32_64
, .cmr = {{0xd4227d06u, 0x6f18b911u, 0xd6f5d9bfu, 0xb9d9f46eu, 0x9aeadbbeu, 0xfa34d474u, 0x432a1e78u, 0x9e4886ffu}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 80 /* milli weight units */
}
,[RIGHT_PAD_LOW_8_16] =
{ .tag = JET
, .jet = simplicity_right_pad_low_8_16
, .cmr = {{0xaba47a53u, 0x6e1227e1u, 0x22baacf1u, 0x9cfd2823u, 0xb9b78d79u, 0xcc06d34cu, 0x348b14a1u, 0xa15abd64u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 75 /* milli weight units */
}
,[RIGHT_PAD_LOW_8_32] =
{ .tag = JET
, .jet = simplicity_right_pad_low_8_32
, .cmr = {{0x8f80a6c2u, 0x74716b67u, 0x22041134u, 0xea1c68aau, 0xbf021329u, 0x8f4e18f8u, 0xf492dc53u, 0x808a3174u}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 77 /* milli weight units */
}
,[RIGHT_PAD_LOW_8_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_8_64
, .cmr = {{0xd69c85e7u, 0xb2d7e949u, 0x436cb129u, 0x5e4aa705u, 0x57d75e7cu, 0xbdec02ccu, 0xa85fbfb1u, 0x3308b210u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 82 /* milli weight units */
}
,[RIGHT_ROTATE_16] =
{ .tag = JET
, .jet = simplicity_right_rotate_16
, .cmr = {{0xe5107082u, 0x47f91b4fu, 0x0a8a22a4u, 0x46b8137du, 0x0d42bee7u, 0x4c8c1eddu, 0x6d446edbu, 0x2013b598u}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 99 /* milli weight units */
}
,[RIGHT_ROTATE_32] =
{ .tag = JET
, .jet = simplicity_right_rotate_32
, .cmr = {{0x98915731u, 0x412922dbu, 0xc516a737u, 0x3afc4de6u, 0x4809f83bu, 0x264bcfcau, 0x6ae74883u, 0xdbe104d6u}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 92 /* milli weight units */
}
,[RIGHT_ROTATE_64] =
{ .tag = JET
, .jet = simplicity_right_rotate_64
, .cmr = {{0x9e2fb98au, 0xdf102933u, 0x9dbe45a2u, 0x2a54a390u, 0xca0986edu, 0xcea32eacu, 0xb82ebcc8u, 0x94a2711au}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 93 /* milli weight units */
}
,[RIGHT_ROTATE_8] =
{ .tag = JET
, .jet = simplicity_right_rotate_8
, .cmr = {{0x00c7c26du, 0x95a50b5au, 0xf9349ffeu, 0x47e1d43fu, 0x3d761f17u, 0xa7453c98u, 0x4791e87du, 0xc6a311c8u}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 75 /* milli weight units */
}
,[RIGHT_SHIFT_16] =
{ .tag = JET
, .jet = simplicity_right_shift_16
, .cmr = {{0x8b5e0febu, 0x958130f0u, 0x50833215u, 0x9e54c2dfu, 0x98af8352u, 0x1acab308u, 0x4fd4f7c3u, 0xa2ccea77u}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 84 /* milli weight units */
}
,[RIGHT_SHIFT_32] =
{ .tag = JET
, .jet = simplicity_right_shift_32
, .cmr = {{0x4b1f2580u, 0xe0850d38u, 0xe2a11573u, 0x38052f1cu, 0x379f9d81u, 0x57f62d33u, 0x890af24fu, 0xd9a7f73eu}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 88 /* milli weight units */
}
,[RIGHT_SHIFT_64] =
{ .tag = JET
, .jet = simplicity_right_shift_64
, .cmr = {{0x91a297d7u, 0xb58a393bu, 0xf5902594u, 0x7747c86du, 0xd487659cu, 0xc56fb5a6u, 0xf6439955u, 0x129a9563u}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 91 /* milli weight units */
}
,[RIGHT_SHIFT_8] =
{ .tag = JET
, .jet = simplicity_right_shift_8
, .cmr = {{0xa4c3546fu, 0xf27e56d6u, 0x4e918ab2u, 0xfa6d00fcu, 0x2704585bu, 0x25bde004u, 0x9d6d8f48u, 0xd8cf1cd0u}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 88 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_16] =
{ .tag = JET
, .jet = simplicity_right_shift_with_16
, .cmr = {{0xfd977030u, 0xe3a25a32u, 0xe775b8d5u, 0xe87174a7u, 0xa9e8731eu, 0xc36cf132u, 0x6420ad91u, 0x502e6e98u}}
, .sourceIx = ty_pbpw4w16
, .targetIx = ty_w16
, .cost = 105 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_32] =
{ .tag = JET
, .jet = simplicity_right_shift_with_32
, .cmr = {{0x2829ba02u, 0x1f54077au, 0xffb66ac6u, 0xb6dfd3feu, 0xf38bc414u, 0x91845a41u, 0xce9dd370u, 0x586c2d04u}}
, .sourceIx = ty_pbpw8w32
, .targetIx = ty_w32
, .cost = 92 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_64] =
{ .tag = JET
, .jet = simplicity_right_shift_with_64
, .cmr = {{0x006fa3c5u, 0x45797547u, 0x86fc64dcu, 0x32e19a22u, 0x5cc152c9u, 0x4deeb3c6u, 0xab2967ddu, 0xbfc64653u}}
, .sourceIx = ty_pbpw8w64
, .targetIx = ty_w64
, .cost = 97 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_8] =
{ .tag = JET
, .jet = simplicity_right_shift_with_8
, .cmr = {{0xfcb5be65u, 0x07f0ca44u, 0xbe2be1ccu, 0x3c3cfe39u, 0x94404b80u, 0x83bd7602u, 0xb2102cb1u, 0xfcfa2c61u}}
, .sourceIx = ty_pbpw4w8
, .targetIx = ty_w8
, .cost = 103 /* milli weight units */
}
,[RIGHTMOST_16_1] =
{ .tag = JET
, .jet = simplicity_rightmost_16_1
, .cmr = {{0x3f3c4346u, 0x87174226u, 0x5e87f001u, 0xb46de7d1u, 0x98751b34u, 0xfaa18018u, 0xde60c846u, 0x8d9b98a4u}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 70 /* milli weight units */
}
,[RIGHTMOST_16_2] =
{ .tag = JET
, .jet = simplicity_rightmost_16_2
, .cmr = {{0x78f17147u, 0x6a3b0ed1u, 0xe3a5455au, 0x5fbbcc90u, 0x1981b323u, 0x0fea1264u, 0x204dacd0u, 0x81f94080u}}
, .sourceIx = ty_w16
, .targetIx = ty_w2
, .cost = 82 /* milli weight units */
}
,[RIGHTMOST_16_4] =
{ .tag = JET
, .jet = simplicity_rightmost_16_4
, .cmr = {{0x75a1dfb6u, 0xae2c066bu, 0x2d0e2093u, 0x048adbc5u, 0x0d465065u, 0x6fb2d357u, 0x8b57d9deu, 0x4c61c8b5u}}
, .sourceIx = ty_w16
, .targetIx = ty_w4
, .cost = 76 /* milli weight units */
}
,[RIGHTMOST_16_8] =
{ .tag = JET
, .jet = simplicity_rightmost_16_8
, .cmr = {{0xee769c1cu, 0xc8a3fdd1u, 0x838fc9f0u, 0x490ce703u, 0x93fd91bau, 0x3cbd4abdu, 0x08649fb9u, 0xc44311bdu}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 69 /* milli weight units */
}
,[RIGHTMOST_32_1] =
{ .tag = JET
, .jet = simplicity_rightmost_32_1
, .cmr = {{0xcb0db569u, 0xa36186a2u, 0x5605a9d2u, 0xe4e10a20u, 0xc111d50cu, 0x34f17246u, 0x520bc454u, 0xd8682836u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 90 /* milli weight units */
}
,[RIGHTMOST_32_16] =
{ .tag = JET
, .jet = simplicity_rightmost_32_16
, .cmr = {{0x06faa3beu, 0x678cd6fdu, 0xd7f3112eu, 0xbf2c4862u, 0x7afa7875u, 0xf7068d26u, 0xa9cc045bu, 0x2c8f11bcu}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 64 /* milli weight units */
}
,[RIGHTMOST_32_2] =
{ .tag = JET
, .jet = simplicity_rightmost_32_2
, .cmr = {{0x00b8815au, 0xd7423dd5u, 0x8cb98be8u, 0x2cad2667u, 0x5c3bf54au, 0x0bedbadeu, 0x3464b4feu, 0x5a4e8ce6u}}
, .sourceIx = ty_w32
, .targetIx = ty_w2
, .cost = 74 /* milli weight units */
}
,[RIGHTMOST_32_4] =
{ .tag = JET
, .jet = simplicity_rightmost_32_4
, .cmr = {{0x3dfa7a20u, 0x198e42d6u, 0xa7948c8eu, 0xd8e0d47eu, 0xc7c0007bu, 0x3d6866cau, 0x15e3da04u, 0x5b8563c7u}}
, .sourceIx = ty_w32
, .targetIx = ty_w4
, .cost = 92 /* milli weight units */
}
,[RIGHTMOST_32_8] =
{ .tag = JET
, .jet = simplicity_rightmost_32_8
, .cmr = {{0x17b58d6eu, 0x304b1c7eu, 0x5dbf0c4du, 0xf6fcc803u, 0xc008944cu, 0x7995555bu, 0x94e1289bu, 0x2549be99u}}
, .sourceIx = ty_w32
, .targetIx = ty_w8
, .cost = 78 /* milli weight units */
}
,[RIGHTMOST_64_1] =
{ .tag = JET
, .jet = simplicity_rightmost_64_1
, .cmr = {{0x5e8fb49fu, 0xace03448u, 0x1dc65361u, 0x8e2a8b65u, 0xeaf0993fu, 0x28844cc9u, 0xb130caccu, 0xe45e82deu}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 77 /* milli weight units */
}
,[RIGHTMOST_64_16] =
{ .tag = JET
, .jet = simplicity_rightmost_64_16
, .cmr = {{0xc64ca996u, 0x5536f237u, 0xbc4d166eu, 0x4aeca56eu, 0xac2662e6u, 0x3accb98bu, 0x6e542560u, 0xf9e538dau}}
, .sourceIx = ty_w64
, .targetIx = ty_w16
, .cost = 86 /* milli weight units */
}
,[RIGHTMOST_64_2] =
{ .tag = JET
, .jet = simplicity_rightmost_64_2
, .cmr = {{0x83d2da6fu, 0x3420d779u, 0xbcb8f60du, 0x0b696eedu, 0x74c31db0u, 0x8addbebdu, 0x1235a5dfu, 0x8f59c42fu}}
, .sourceIx = ty_w64
, .targetIx = ty_w2
, .cost = 74 /* milli weight units */
}
,[RIGHTMOST_64_32] =
{ .tag = JET
, .jet = simplicity_rightmost_64_32
, .cmr = {{0x7d2dff6eu, 0x3dd504bbu, 0x0e5703a0u, 0x33586d27u, 0xd96644c0u, 0x48ab34a4u, 0x5bf53512u, 0x9d501167u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 76 /* milli weight units */
}
,[RIGHTMOST_64_4] =
{ .tag = JET
, .jet = simplicity_rightmost_64_4
, .cmr = {{0x841bbd65u, 0x2742ddd3u, 0xadeae43cu, 0xfed6329fu, 0x2fd62e6fu, 0xecd0fd58u, 0xe3c3fb8bu, 0x5a0e4dd5u}}
, .sourceIx = ty_w64
, .targetIx = ty_w4
, .cost = 70 /* milli weight units */
}
,[RIGHTMOST_64_8] =
{ .tag = JET
, .jet = simplicity_rightmost_64_8
, .cmr = {{0xa0a61c76u, 0x58a18623u, 0xbf1d011au, 0x9792d518u, 0xfbd02414u, 0x2a904400u, 0xecdeea92u, 0x457a0a81u}}
, .sourceIx = ty_w64
, .targetIx = ty_w8
, .cost = 69 /* milli weight units */
}
,[RIGHTMOST_8_1] =
{ .tag = JET
, .jet = simplicity_rightmost_8_1
, .cmr = {{0x999b686eu, 0x60b3d1ecu, 0xd6c6d77fu, 0xbca82cb2u, 0xabbd4182u, 0xc8211267u, 0x475fa0c1u, 0x901d89f9u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 79 /* milli weight units */
}
,[RIGHTMOST_8_2] =
{ .tag = JET
, .jet = simplicity_rightmost_8_2
, .cmr = {{0x5307ffbfu, 0x516cd0eeu, 0xf3ff4387u, 0xb9052c14u, 0x4a4dfa23u, 0x29237c6bu, 0x274992b2u, 0xc8047b60u}}
, .sourceIx = ty_w8
, .targetIx = ty_w2
, .cost = 98 /* milli weight units */
}
,[RIGHTMOST_8_4] =
{ .tag = JET
, .jet = simplicity_rightmost_8_4
, .cmr = {{0x7f52e645u, 0xbbbbd792u, 0x69c43ef0u, 0x2db982f8u, 0xc63633c1u, 0x79e40691u, 0x73933604u, 0xcc635bcau}}
, .sourceIx = ty_w8
, .targetIx = ty_w4
, .cost = 98 /* milli weight units */
}
,[SCALAR_ADD] =
{ .tag = JET
, .jet = simplicity_scalar_add
, .cmr = {{0x34baa40bu, 0x2e0aa8cbu, 0x7e97c73eu, 0x3ed3b365u, 0xa15b7c3fu, 0x7661fb19u, 0x715ec605u, 0xc1149d11u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 739 /* milli weight units */
}
,[SCALAR_INVERT] =
{ .tag = JET
, .jet = simplicity_scalar_invert
, .cmr = {{0x6231bdabu, 0x73ca34eau, 0x7e837daau, 0xd692ede5u, 0xbabfae09u, 0xb5756d2au, 0xb36c5a36u, 0x475a6589u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 3193 /* milli weight units */
}
,[SCALAR_IS_ZERO] =
{ .tag = JET
, .jet = simplicity_scalar_is_zero
, .cmr = {{0xf75eda06u, 0xce6af09fu, 0xae37db4eu, 0x6225e6a8u, 0xac86a236u, 0x37627d62u, 0x6409190fu, 0xf3b39d90u}}
, .sourceIx = ty_w256
, .targetIx = ty_b
, .cost = 271 /* milli weight units */
}
,[SCALAR_MULTIPLY] =
{ .tag = JET
, .jet = simplicity_scalar_multiply
, .cmr = {{0xb2bcc390u, 0xd637b9e0u, 0x3fbfc42fu, 0xff71d22eu, 0x7200f693u, 0x29cef716u, 0x9e68a8c7u, 0x1a7f0a4bu}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 774 /* milli weight units */
}
,[SCALAR_MULTIPLY_LAMBDA] =
{ .tag = JET
, .jet = simplicity_scalar_multiply_lambda
, .cmr = {{0x89d5855cu, 0x5f85c003u, 0x5d27b0c0u, 0x9e20330bu, 0x001c684bu, 0x5986abceu, 0xd8360cd3u, 0x9b08c4e1u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 557 /* milli weight units */
}
,[SCALAR_NEGATE] =
{ .tag = JET
, .jet = simplicity_scalar_negate
, .cmr = {{0x0705acdfu, 0xb8664000u, 0x0e3d3badu, 0x509a14a7u, 0x8c171f61u, 0xedc08423u, 0xb042b947u, 0x48439cf8u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 490 /* milli weight units */
}
,[SCALAR_NORMALIZE] =
{ .tag = JET
, .jet = simplicity_scalar_normalize
, .cmr = {{0xa061e19du, 0x75c325a2u, 0x6d565aadu, 0x7e3f9ae2u, 0x6b222f25u, 0xe802174fu, 0x6bacd511u, 0x277aeaa5u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 472 /* milli weight units */
}
,[SCALAR_SQUARE] =
{ .tag = JET
, .jet = simplicity_scalar_square
, .cmr = {{0x49f734a2u, 0x659ca0abu, 0x7c9e67fcu, 0xfc3c0d72u, 0xaf0f917cu, 0x9edcb992u, 0x9d177a0fu, 0x0de89d59u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 575 /* milli weight units */
}
,[SCALE] =
{ .tag = JET
, .jet = simplicity_scale
, .cmr = {{0xc04543dcu, 0x85ef1137u, 0x4a930f4au, 0x948eb735u, 0xa6500a1au, 0x7158d573u, 0x123f0721u, 0x7175f318u}}
, .sourceIx = ty_pw256pw512w256
, .targetIx = ty_pw512w256
, .cost = 72675 /* milli weight units */
}
,[SCRIPT_CMR] =
{ .tag = JET
, .jet = simplicity_script_cmr
, .cmr = {{0xec15f59cu, 0x524d9414u, 0x69811954u, 0x654f6358u, 0x30ecd7deu, 0x517662b2u, 0x5ca4e8d0u, 0x5c2cce89u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 136 /* milli weight units */
}
,[SHA_256_BLOCK] =
{ .tag = JET
, .jet = simplicity_sha_256_block
, .cmr = {{0x0c97a008u, 0xade87bb1u, 0xe0ac06b7u, 0xd0313023u, 0x362858efu, 0x90ec14ecu, 0x9cb95f0du, 0xa964e008u}}
, .sourceIx = ty_pw256w512
, .targetIx = ty_w256
, .cost = 771 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_1] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_1
, .cmr = {{0x37066c67u, 0xad95249du, 0x4ba6e181u, 0x44ca0a41u, 0x5d9c832au, 0xa6b60628u, 0xe97c967eu, 0xb1793383u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w8
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 642 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_128] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_128
, .cmr = {{0x2dcf484cu, 0x257f6794u, 0x0ca375bau, 0x98e83ce0u, 0xe2a71e16u, 0xda5051d1u, 0xbb19fb5fu, 0x346f154fu}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w1Ki
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1779 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_16] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_16
, .cmr = {{0x82992520u, 0x40cb39e3u, 0x26a248d5u, 0xc788f951u, 0x6d15a2ffu, 0x4145bb64u, 0xad6577aeu, 0x1a3ef727u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w128
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 747 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_2] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_2
, .cmr = {{0x8bae3e7eu, 0x1ed4dcbau, 0x6e645aa1u, 0x4341bbaeu, 0x0dbb3ae2u, 0x1bb63dc0u, 0x30ca0e44u, 0x7a857ec2u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w16
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 661 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_256] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_256
, .cmr = {{0x44b717e1u, 0x970999b6u, 0x6b693d8cu, 0x9d1d3b06u, 0x05c2b7a6u, 0x213e6ba5u, 0x6c69af8du, 0x7fae1686u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w2Ki
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 2912 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_32] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_32
, .cmr = {{0x39239a43u, 0xa84bac6fu, 0x2969bfa9u, 0x5bfe6a04u, 0xfcba8092u, 0x895939f1u, 0x2a1ce0e2u, 0x6321ec10u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w256
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 896 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_4] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_4
, .cmr = {{0xd7d74561u, 0x4b37a7e0u, 0x7dce22f6u, 0x4e7b1edfu, 0xe23beda8u, 0x51f1e76fu, 0x1a6b028fu, 0xcc5e9fc0u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w32
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 645 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_512] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_512
, .cmr = {{0xbe368032u, 0xd86ebcf2u, 0x13ca45bau, 0x6ecab54cu, 0xb1f2661du, 0x403da059u, 0x06300bc5u, 0x1137aab5u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w4Ki
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 5299 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_64] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_64
, .cmr = {{0xfdc434ceu, 0x83dbdce0u, 0x782aa36du, 0x418def7fu, 0x99af8293u, 0xafb29e83u, 0x9fe4948fu, 0x6234f77fu}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w512
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1187 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_8] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_8
, .cmr = {{0x9c988330u, 0x799a680bu, 0xfe73d7cau, 0xa3689fe4u, 0xe483da4eu, 0xe6d81858u, 0x7927c7f4u, 0x3392def7u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w64
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 674 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_BUFFER_511] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_buffer_511
, .cmr = {{0xc027e106u, 0x2996ae94u, 0xac3971a2u, 0xc4fae549u, 0x97ebf09bu, 0x9f7da575u, 0x639be617u, 0x167f02e3u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256pmw2Kipmw1Kipmw512pmw256pmw128pmw64pmw32pmw16mw8
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 5060 /* milli weight units */
}
,[SHA_256_CTX_8_FINALIZE] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_finalize
, .cmr = {{0xcbba1f1du, 0x8a97ab4du, 0x1fa9686eu, 0x7aeef066u, 0xfb5bf290u, 0x716eae10u, 0xe70b6199u, 0x96c59594u}}
, .sourceIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .targetIx = ty_w256
, .cost = 835 /* milli weight units */
}
,[SHA_256_CTX_8_INIT] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_init
, .cmr = {{0xa53c7679u, 0xe3ae0347u, 0xd4d79126u, 0xa7c7e49au, 0xc0dec90cu, 0xdf935799u, 0xcddb58dau, 0x8f4496e4u}}
, .sourceIx = ty_u
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 118 /* milli weight units */
}
,[SHA_256_IV] =
{ .tag = JET
, .jet = simplicity_sha_256_iv
, .cmr = {{0x7389f002u, 0x5305dce8u, 0x28d4a1feu, 0x83743046u, 0xa367c923u, 0xf18abf36u, 0x5e391e5bu, 0x04af1a47u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 93 /* milli weight units */
}
,[SIG_ALL_HASH] =
{ .tag = JET
, .jet = simplicity_sig_all_hash
, .cmr = {{0x6ac53d3fu, 0x93b8caf3u, 0xea8534aeu, 0x612abd32u, 0x325d2b0fu, 0xec17de36u, 0xaf0f71aau, 0x978f7cdeu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 133 /* milli weight units */
}
,[SOME_1] =
{ .tag = JET
, .jet = simplicity_some_1
, .cmr = {{0xfbdad6b0u, 0x22a0c78fu, 0xf35604aau, 0xfacd27ccu, 0x10f51ee0u, 0x698c41f1u, 0xada90397u, 0x618d526fu}}
, .sourceIx = ty_b
, .targetIx = ty_b
, .cost = 70 /* milli weight units */
}
,[SOME_16] =
{ .tag = JET
, .jet = simplicity_some_16
, .cmr = {{0x7e2ccdbfu, 0xc24dd8d8u, 0xa904b017u, 0xdd4f57e7u, 0xc8749634u, 0x8aca7d04u, 0x58c9d16bu, 0x68bcda1cu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 63 /* milli weight units */
}
,[SOME_32] =
{ .tag = JET
, .jet = simplicity_some_32
, .cmr = {{0x4536aeb1u, 0x21c4273fu, 0xfc2a48feu, 0xd9eed031u, 0x2ebd972du, 0xec5681f4u, 0x7ead0f62u, 0xd954452au}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 64 /* milli weight units */
}
,[SOME_64] =
{ .tag = JET
, .jet = simplicity_some_64
, .cmr = {{0x7f0bbd9du, 0x6631c130u, 0x9f901c2fu, 0x0d7a0d28u, 0x4a34416cu, 0xf750db1fu, 0xe2b9f3d6u, 0xed709409u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 93 /* milli weight units */
}
,[SOME_8] =
{ .tag = JET
, .jet = simplicity_some_8
, .cmr = {{0x2d8c8f71u, 0xee5e7582u, 0xf0ed65f5u, 0x26c02605u, 0xdcb93c0bu, 0xddb9433au, 0xff3f25c2u, 0x28acda8au}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 75 /* milli weight units */
}
,[SUBTRACT_16] =
{ .tag = JET
, .jet = simplicity_subtract_16
, .cmr = {{0x569e6c6bu, 0x39e7d812u, 0x659b67aau, 0xc08ad150u, 0x99eead79u, 0x8fd1d42du, 0xa17ee3f0u, 0xd4d4492au}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw16
, .cost = 113 /* milli weight units */
}
,[SUBTRACT_32] =
{ .tag = JET
, .jet = simplicity_subtract_32
, .cmr = {{0x19d35e0au, 0xf1e16514u, 0xa6dfc29au, 0x91418713u, 0x3964c480u, 0xf660e7ebu, 0x924ee16du, 0xbaa249cbu}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw32
, .cost = 118 /* milli weight units */
}
,[SUBTRACT_64] =
{ .tag = JET
, .jet = simplicity_subtract_64
, .cmr = {{0x523e1186u, 0x28bf3ac1u, 0xa6be5a72u, 0xbdb1141bu, 0x89e0e001u, 0xe402addau, 0x82587900u, 0x03f88ad8u}}
, .sourceIx = ty_w128
, .targetIx = ty_pbw64
, .cost = 115 /* milli weight units */
}
,[SUBTRACT_8] =
{ .tag = JET
, .jet = simplicity_subtract_8
, .cmr = {{0x40950b86u, 0xf6f1f993u, 0x55dee11fu, 0x77daf279u, 0xa0cb6c6du, 0x156ae44bu, 0x7d5d2571u, 0x64b267c5u}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw8
, .cost = 109 /* milli weight units */
}
,[SWU] =
{ .tag = JET
, .jet = simplicity_swu
, .cmr = {{0xabf70be0u, 0x0b30f577u, 0xf987cb50u, 0x488996bau, 0x3596dbf9u, 0xc1e844a8u, 0xb1b8b710u, 0x853b65ebu}}
, .sourceIx = ty_w256
, .targetIx = ty_w512
, .cost = 32120 /* milli weight units */
}
,[TAP_ENV_HASH] =
{ .tag = JET
, .jet = simplicity_tap_env_hash
, .cmr = {{0x72270771u, 0x93ec1741u, 0x8ebf7877u, 0xafdc7097u, 0xbd5eed57u, 0xe826df66u, 0x05c64869u, 0x1d003403u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 162 /* milli weight units */
}
,[TAPDATA_INIT] =
{ .tag = JET
, .jet = simplicity_tapdata_init
, .cmr = {{0x6c67e5c1u, 0x0735305eu, 0xe7deb59au, 0x6c6ac2efu, 0xfcab4ff7u, 0xbb479ea7u, 0x0081606eu, 0x60484ca7u}}
, .sourceIx = ty_u
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1178 /* milli weight units */
}
,[TAPLEAF_HASH] =
{ .tag = JET
, .jet = simplicity_tapleaf_hash
, .cmr = {{0xd45a661fu, 0xc9100e2eu, 0xa9df789cu, 0xa2346da8u, 0x42717ef6u, 0xa8c82418u, 0x4f2da8c7u, 0xfb016203u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 136 /* milli weight units */
}
,[TAPLEAF_VERSION] =
{ .tag = JET
, .jet = simplicity_tapleaf_version
, .cmr = {{0xddb0edd6u, 0xce1e0798u, 0x3d1a6226u, 0xf00b1251u, 0x89af13bdu, 0x5cd06812u, 0x3ad43f48u, 0x201b006fu}}
, .sourceIx = ty_u
, .targetIx = ty_w8
, .cost = 105 /* milli weight units */
}
,[TAPPATH] =
{ .tag = JET
, .jet = simplicity_tappath
, .cmr = {{0x42c0c1f2u, 0xef0e28aeu, 0x7defa15eu, 0x84a06f35u, 0xfbf6fc89u, 0x7b29f683u, 0xfe1a24e3u, 0x0dc58bafu}}
, .sourceIx = ty_w8
, .targetIx = ty_mw256
, .cost = 83 /* milli weight units */
}
,[TAPPATH_HASH] =
{ .tag = JET
, .jet = simplicity_tappath_hash
, .cmr = {{0x9da906e3u, 0xec086065u, 0xbe144136u, 0xb51620b6u, 0x6ddf4d71u, 0x3857354eu, 0xba3b97c7u, 0x7585f443u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 143 /* milli weight units */
}
,[TOTAL_FEE] =
{ .tag = JET
, .jet = simplicity_total_fee
, .cmr = {{0x4eba3f22u, 0xbac0a465u, 0xe4d08a9au, 0xc40518efu, 0x972c2e86u, 0x12090a51u, 0xe3203fcbu, 0xd2f4edbeu}}
, .sourceIx = ty_w256
, .targetIx = ty_w64
, .cost = 230 /* milli weight units */
}
,[TRANSACTION_ID] =
{ .tag = JET
, .jet = simplicity_transaction_id
, .cmr = {{0x08b8eb3cu, 0x2d2d9584u, 0x58e163e9u, 0x4334950cu, 0x0bb914a0u, 0xb9d214fcu, 0x5e9de8f7u, 0x0a17a0d4u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 139 /* milli weight units */
}
,[TX_HASH] =
{ .tag = JET
, .jet = simplicity_tx_hash
, .cmr = {{0xc36102aeu, 0xda8bcf62u, 0xa85e714bu, 0x18e4c50cu, 0xd417ffbfu, 0x86d73734u, 0xe02cbfcbu, 0x5313fccfu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 143 /* milli weight units */
}
,[TX_IS_FINAL] =
{ .tag = JET
, .jet = simplicity_tx_is_final
, .cmr = {{0x8b314572u, 0x2470a07du, 0xe90a28bau, 0x89f3f886u, 0x42610096u, 0x54ce866cu, 0xd8eaf76cu, 0x5d8626ebu}}
, .sourceIx = ty_u
, .targetIx = ty_b
, .cost = 71 /* milli weight units */
}
,[TX_LOCK_DISTANCE] =
{ .tag = JET
, .jet = simplicity_tx_lock_distance
, .cmr = {{0x4c7773b8u, 0x18cb7ee5u, 0xf54f925au, 0xad015677u, 0xa043a72fu, 0x316a187cu, 0xc28c696cu, 0xfcb90807u}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 91 /* milli weight units */
}
,[TX_LOCK_DURATION] =
{ .tag = JET
, .jet = simplicity_tx_lock_duration
, .cmr = {{0xcc9c64c8u, 0xb6eb4bf0u, 0x9694af5au, 0x35d957a4u, 0x05e66c1bu, 0x35224ed6u, 0x75878918u, 0x452440b2u}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 84 /* milli weight units */
}
,[TX_LOCK_HEIGHT] =
{ .tag = JET
, .jet = simplicity_tx_lock_height
, .cmr = {{0xc20257f8u, 0xe76ecd0au, 0xe7ad634fu, 0x5dfa68aeu, 0x9a5eded0u, 0xe2eebe4eu, 0xe52cb47au, 0xcfb0264cu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 80 /* milli weight units */
}
,[TX_LOCK_TIME] =
{ .tag = JET
, .jet = simplicity_tx_lock_time
, .cmr = {{0x3ee19005u, 0x42d01efdu, 0x4e9a01d4u, 0xefb1f9ddu, 0x992ced35u, 0xb7a752f8u, 0x3da59338u, 0x1538dea4u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 80 /* milli weight units */
}
,[VERIFY] =
{ .tag = JET
, .jet = simplicity_verify
, .cmr = {{0x343e6dc1u, 0x6b3f52e8u, 0x3e3b4cccu, 0x99b8c6f9u, 0x6a074fe3u, 0x99327af3u, 0x64bc285eu, 0x299745a2u}}
, .sourceIx = ty_b
, .targetIx = ty_u
, .cost = 57 /* milli weight units */
}
,[VERSION] =
{ .tag = JET
, .jet = simplicity_version
, .cmr = {{0x087fc95cu, 0x41003348u, 0x759fd840u, 0x372c6f91u, 0x2d9be4e6u, 0x1eee3c6au, 0x7a40dc13u, 0xc9c1bc70u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 93 /* milli weight units */
}
,[XOR_1] =
{ .tag = JET
, .jet = simplicity_xor_1
, .cmr = {{0x9dc9fe42u, 0xf7eb3464u, 0x9f1c72d2u, 0xe5dd167du, 0xb21be532u, 0x1372d5cau, 0x7f6a184fu, 0x93e05ee3u}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 67 /* milli weight units */
}
,[XOR_16] =
{ .tag = JET
, .jet = simplicity_xor_16
, .cmr = {{0x1fcaf40bu, 0xdedd72e7u, 0x97b09fe7u, 0x8753b0abu, 0x27872c0bu, 0xd12b0349u, 0x55fbfac2u, 0x3812ef26u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 83 /* milli weight units */
}
,[XOR_32] =
{ .tag = JET
, .jet = simplicity_xor_32
, .cmr = {{0x1d49fc94u, 0xf22b5d31u, 0xb7f9efb5u, 0x378e5f8au, 0x42626aedu, 0x4e927993u, 0x48d6b788u, 0xdfe86b1cu}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 92 /* milli weight units */
}
,[XOR_64] =
{ .tag = JET
, .jet = simplicity_xor_64
, .cmr = {{0x7a3f3f55u, 0x20478365u, 0x3344311du, 0x1dc509d3u, 0x5b6639c0u, 0xd8b967a2u, 0x07806cd8u, 0x7d31d6e6u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 95 /* milli weight units */
}
,[XOR_8] =
{ .tag = JET
, .jet = simplicity_xor_8
, .cmr = {{0xd8335f48u, 0x90c1d8edu, 0x766c7135u, 0x902e01a0u, 0x094e3a98u, 0x16f70c84u, 0x7cc3d7c0u, 0x00406efeu}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 85 /* milli weight units */
}
,[XOR_XOR_1] =
{ .tag = JET
, .jet = simplicity_xor_xor_1
, .cmr = {{0x1e107b05u, 0xff941d31u, 0xd7578b43u, 0x7328ba52u, 0xf3ff20a0u, 0x68c0d2bdu, 0xef087680u, 0x93cc7c63u}}
, .sourceIx = ty_pbw2
, .targetIx = ty_b
, .cost = 72 /* milli weight units */
}
,[XOR_XOR_16] =
{ .tag = JET
, .jet = simplicity_xor_xor_16
, .cmr = {{0xb776989du, 0xa5095c4bu, 0xe94b1aefu, 0x759466e1u, 0x1f639c19u, 0x39471fa1u, 0x8e36e7e4u, 0x90c38961u}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 79 /* milli weight units */
}
,[XOR_XOR_32] =
{ .tag = JET
, .jet = simplicity_xor_xor_32
, .cmr = {{0xd168fac1u, 0xac7fc483u, 0x57be1b65u, 0x3375ec5eu, 0x3f05823au, 0xae6ac985u, 0xe9403eeau, 0xb12bb9f8u}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 96 /* milli weight units */
}
,[XOR_XOR_64] =
{ .tag = JET
, .jet = simplicity_xor_xor_64
, .cmr = {{0x361c5793u, 0x0ef97d49u, 0xcbc679fau, 0xef1e3bcfu, 0xfb787995u, 0xb961e553u, 0x7d2b1eebu, 0xc9c9a6e8u}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 93 /* milli weight units */
}
,[XOR_XOR_8] =
{ .tag = JET
, .jet = simplicity_xor_xor_8
, .cmr = {{0xc2da6e9cu, 0xa64d8a73u, 0xc1772667u, 0xb3d7a093u, 0x8bcb8a6cu, 0x43fd0473u, 0xeec71b77u, 0x494aad94u}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 98 /* milli weight units */
}
