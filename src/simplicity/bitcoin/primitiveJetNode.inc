/* This file has been automatically generated. */
[ADD_16] =
{ .tag = JET
, .jet = simplicity_add_16
, .cmr = {{0x49425a86u, 0xe20a676du, 0x8b87e3c1u, 0xa9b8ea6eu, 0xc75d859cu, 0x12c51bcbu, 0x7fa9f969u, 0x12c349cfu}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw16
, .cost = 80 /* milli weight units */
}
,[ADD_32] =
{ .tag = JET
, .jet = simplicity_add_32
, .cmr = {{0x4668cd55u, 0xe8d15919u, 0x53327014u, 0xec64c8e7u, 0xd52b86b5u, 0x3e11c014u, 0x57eaf2c3u, 0xd3cebf9fu}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw32
, .cost = 92 /* milli weight units */
}
,[ADD_64] =
{ .tag = JET
, .jet = simplicity_add_64
, .cmr = {{0xbe2b7519u, 0x303a67eeu, 0xa6b48295u, 0x0eda8343u, 0x5e1de855u, 0x9c394a23u, 0x6222ff5bu, 0xf089d346u}}
, .sourceIx = ty_w128
, .targetIx = ty_pbw64
, .cost = 105 /* milli weight units */
}
,[ADD_8] =
{ .tag = JET
, .jet = simplicity_add_8
, .cmr = {{0xdfa179adu, 0xf4550b28u, 0x4873bf30u, 0x123e0d4eu, 0x54069b08u, 0x5834ce56u, 0x5815ef7eu, 0x45784acbu}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw8
, .cost = 97 /* milli weight units */
}
,[ALL_16] =
{ .tag = JET
, .jet = simplicity_all_16
, .cmr = {{0x24f482a5u, 0x13d33362u, 0x015d28dfu, 0x4bb6c3eeu, 0x08ab8afbu, 0xbd25571fu, 0x0ea89d8cu, 0xaba31404u}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 60 /* milli weight units */
}
,[ALL_32] =
{ .tag = JET
, .jet = simplicity_all_32
, .cmr = {{0xa716522du, 0x0f3787c8u, 0xb4d50764u, 0x7f1f807bu, 0x67f320d6u, 0xeb67b84bu, 0x609cec1du, 0x2f12218au}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 62 /* milli weight units */
}
,[ALL_64] =
{ .tag = JET
, .jet = simplicity_all_64
, .cmr = {{0x7aeefe2eu, 0xce24bab3u, 0x7c6e5430u, 0xeed419fcu, 0xd5f03791u, 0x2d1770cbu, 0x7d6520dcu, 0xe525291au}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 63 /* milli weight units */
}
,[ALL_8] =
{ .tag = JET
, .jet = simplicity_all_8
, .cmr = {{0x4637f40eu, 0x5f4726b0u, 0x0570765au, 0xc794e29eu, 0xd1bb2655u, 0xffc412b2u, 0xdc41258eu, 0x41aac624u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 50 /* milli weight units */
}
,[AND_1] =
{ .tag = JET
, .jet = simplicity_and_1
, .cmr = {{0x10684d0du, 0xd72cb0a8u, 0x26a86383u, 0x4e011f50u, 0xfa0d558bu, 0xa77d6b9fu, 0x49a1ac22u, 0x902a6ad0u}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 77 /* milli weight units */
}
,[AND_16] =
{ .tag = JET
, .jet = simplicity_and_16
, .cmr = {{0x373c730fu, 0xad3e8847u, 0x991aa417u, 0xd9f080eeu, 0x1cb88a7fu, 0x7206f3fau, 0x840b1950u, 0x7761fb23u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 83 /* milli weight units */
}
,[AND_32] =
{ .tag = JET
, .jet = simplicity_and_32
, .cmr = {{0x13b02c4cu, 0x60ae6ea4u, 0x91161649u, 0xacf9a47au, 0x7025af84u, 0x7d5f581eu, 0x6f1cccfbu, 0x21d3001du}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 77 /* milli weight units */
}
,[AND_64] =
{ .tag = JET
, .jet = simplicity_and_64
, .cmr = {{0x92185535u, 0xd4505407u, 0xdea3c8a6u, 0x0826ede6u, 0x4a8fbb3du, 0xb486d56fu, 0x642d217cu, 0x29cbd795u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 78 /* milli weight units */
}
,[AND_8] =
{ .tag = JET
, .jet = simplicity_and_8
, .cmr = {{0x269a1b44u, 0x6266f8f4u, 0xa4a38fa7u, 0xe7e39182u, 0xf1521436u, 0x142badedu, 0xf3aa63fbu, 0x2f172d2fu}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 98 /* milli weight units */
}
,[ANNEX_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_annex_hash
, .cmr = {{0x51fa1913u, 0xa6297348u, 0x4a700af3u, 0xff932694u, 0xd3890ae0u, 0xad87a455u, 0xdaf4906fu, 0x224a48d5u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256mw256
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1491 /* milli weight units */
}
,[BIP_0340_VERIFY] =
{ .tag = JET
, .jet = simplicity_bip_0340_verify
, .cmr = {{0x491565feu, 0x23a7bdc1u, 0x842be749u, 0x509337f9u, 0x6890d5b3u, 0x58b36520u, 0x90da5566u, 0x54e29549u}}
, .sourceIx = ty_w1Ki
, .targetIx = ty_u
, .cost = 49421 /* milli weight units */
}
,[BUILD_TAPBRANCH] =
{ .tag = JET
, .jet = simplicity_bitcoin_build_tapbranch
, .cmr = {{0xef8e9291u, 0x9b2608eau, 0x6b5bcdd3u, 0x9c5178e5u, 0x46249553u, 0x914b0f8du, 0x33735d28u, 0x86e1a5e8u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 2554 /* milli weight units */
}
,[BUILD_TAPLEAF_SIMPLICITY] =
{ .tag = JET
, .jet = simplicity_bitcoin_build_tapleaf_simplicity
, .cmr = {{0x22411909u, 0x984147deu, 0x8f5a0428u, 0x358b4716u, 0xdb087664u, 0xa7285608u, 0x52b0e616u, 0xebc62d30u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 1927 /* milli weight units */
}
,[BUILD_TAPTWEAK] =
{ .tag = JET
, .jet = simplicity_bitcoin_build_taptweak
, .cmr = {{0xf192db17u, 0x06608defu, 0x165dbddau, 0x72a38c88u, 0x82cb36c6u, 0xda47070du, 0x8a5f5896u, 0xfbda8434u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 77780 /* milli weight units */
}
,[CH_1] =
{ .tag = JET
, .jet = simplicity_ch_1
, .cmr = {{0x73b2a981u, 0xd721986fu, 0x8cded697u, 0xe06305d4u, 0x5854102du, 0xff20c0e5u, 0xb98ae176u, 0x232ff25bu}}
, .sourceIx = ty_pbw2
, .targetIx = ty_b
, .cost = 50 /* milli weight units */
}
,[CH_16] =
{ .tag = JET
, .jet = simplicity_ch_16
, .cmr = {{0x78de465eu, 0x61d9a50fu, 0x78252ff4u, 0xab23c9e6u, 0x3ae68c76u, 0x9d366612u, 0x71207dc6u, 0x93f469b4u}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 83 /* milli weight units */
}
,[CH_32] =
{ .tag = JET
, .jet = simplicity_ch_32
, .cmr = {{0xed93bef1u, 0xf66e3a75u, 0xe6120602u, 0xecee6740u, 0x653e7bd4u, 0x6e07eb77u, 0x144ef1bbu, 0x2c9de53du}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 69 /* milli weight units */
}
,[CH_64] =
{ .tag = JET
, .jet = simplicity_ch_64
, .cmr = {{0xced0079bu, 0x0bd1cc00u, 0x209a7cbcu, 0x23f13dfdu, 0x202808f0u, 0xf5257d8au, 0x50ac543eu, 0x64ee3a05u}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 78 /* milli weight units */
}
,[CH_8] =
{ .tag = JET
, .jet = simplicity_ch_8
, .cmr = {{0xc707ca72u, 0x3e24f6b2u, 0x5bf394a9u, 0x9a4d75e8u, 0x1379b467u, 0x8438ac78u, 0x9dee188eu, 0xdcce75fau}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 86 /* milli weight units */
}
,[CHECK_LOCK_DISTANCE] =
{ .tag = JET
, .jet = simplicity_bitcoin_check_lock_distance
, .cmr = {{0xdbaa6fedu, 0xb73b2e31u, 0xa80ccbd1u, 0x4daaa5b8u, 0xce212cdeu, 0x0cbfd96du, 0x5db9715fu, 0xd19f8899u}}
, .sourceIx = ty_w16
, .targetIx = ty_u
, .cost = 84 /* milli weight units */
}
,[CHECK_LOCK_DURATION] =
{ .tag = JET
, .jet = simplicity_bitcoin_check_lock_duration
, .cmr = {{0xdde93f33u, 0xf9b9d1d3u, 0xa3f2b3e8u, 0xb90f6d8bu, 0xeffccb45u, 0x81210eeau, 0xf06bd32fu, 0x6319df6eu}}
, .sourceIx = ty_w16
, .targetIx = ty_u
, .cost = 78 /* milli weight units */
}
,[CHECK_LOCK_HEIGHT] =
{ .tag = JET
, .jet = simplicity_bitcoin_check_lock_height
, .cmr = {{0xb90f151fu, 0x45b4eb37u, 0x210621f9u, 0x700a36c8u, 0xc504bee0u, 0x67771163u, 0xa8b83a77u, 0x18e6686au}}
, .sourceIx = ty_w32
, .targetIx = ty_u
, .cost = 108 /* milli weight units */
}
,[CHECK_LOCK_TIME] =
{ .tag = JET
, .jet = simplicity_bitcoin_check_lock_time
, .cmr = {{0xa451ebb2u, 0x25d6b133u, 0xa5e63539u, 0x7800d487u, 0xb3968b0du, 0xe55c96ebu, 0x82f290ecu, 0xff9c2590u}}
, .sourceIx = ty_w32
, .targetIx = ty_u
, .cost = 123 /* milli weight units */
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
, .cmr = {{0x1bcfae13u, 0xd5d237a0u, 0xbb9b1d75u, 0x32047462u, 0xb27690deu, 0x5cac0e20u, 0x19298964u, 0x57934060u}}
, .sourceIx = ty_b
, .targetIx = ty_b
, .cost = 51 /* milli weight units */
}
,[COMPLEMENT_16] =
{ .tag = JET
, .jet = simplicity_complement_16
, .cmr = {{0x81ad4d2cu, 0x3d16bf34u, 0x0af3886du, 0x355cc5bdu, 0x1d5967e1u, 0x6ace924fu, 0x19ecf7d4u, 0x86d6c7e9u}}
, .sourceIx = ty_w16
, .targetIx = ty_w16
, .cost = 86 /* milli weight units */
}
,[COMPLEMENT_32] =
{ .tag = JET
, .jet = simplicity_complement_32
, .cmr = {{0x13742c18u, 0x04a96e6cu, 0x039528bfu, 0xd03b8cf2u, 0xb462526bu, 0xb181a3d8u, 0xb432f99au, 0xc4f5a7efu}}
, .sourceIx = ty_w32
, .targetIx = ty_w32
, .cost = 58 /* milli weight units */
}
,[COMPLEMENT_64] =
{ .tag = JET
, .jet = simplicity_complement_64
, .cmr = {{0x65b7bd09u, 0x3639c56du, 0xa285cefau, 0x2d046464u, 0x5e14dd13u, 0x642f3495u, 0x7d4737bdu, 0x52fac588u}}
, .sourceIx = ty_w64
, .targetIx = ty_w64
, .cost = 64 /* milli weight units */
}
,[COMPLEMENT_8] =
{ .tag = JET
, .jet = simplicity_complement_8
, .cmr = {{0x954b70dcu, 0xec539e6bu, 0x67dffec5u, 0x3cf24a66u, 0x9939608bu, 0x223f5f8bu, 0x6d129daau, 0x48ca1cf0u}}
, .sourceIx = ty_w8
, .targetIx = ty_w8
, .cost = 62 /* milli weight units */
}
,[CURRENT_ANNEX_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_current_annex_hash
, .cmr = {{0xced9022eu, 0xdc69241fu, 0xe70749a7u, 0xf5d489c3u, 0x135ee8c9u, 0x6fe64c44u, 0x64401f98u, 0x51d7a17du}}
, .sourceIx = ty_u
, .targetIx = ty_mw256
, .cost = 74 /* milli weight units */
}
,[CURRENT_INDEX] =
{ .tag = JET
, .jet = simplicity_bitcoin_current_index
, .cmr = {{0x0e8c964cu, 0x2f2b3490u, 0x362f3bbcu, 0x7483dea3u, 0x7fda810bu, 0x69314ff6u, 0x64fea0e3u, 0x2708ec8fu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 66 /* milli weight units */
}
,[CURRENT_PREV_OUTPOINT] =
{ .tag = JET
, .jet = simplicity_bitcoin_current_prev_outpoint
, .cmr = {{0x6443391bu, 0x34408684u, 0x6d5a17a6u, 0x2e3e0628u, 0x2ad6962cu, 0x4dfced2fu, 0x6a83d0dfu, 0xbf6a5c54u}}
, .sourceIx = ty_u
, .targetIx = ty_pw256w32
, .cost = 130 /* milli weight units */
}
,[CURRENT_SCRIPT_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_current_script_hash
, .cmr = {{0x23498dd6u, 0x645ed138u, 0xb344937cu, 0xf654aaffu, 0xa627f85au, 0x47caa689u, 0x54f13f4cu, 0x6a4dc772u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 127 /* milli weight units */
}
,[CURRENT_SCRIPT_SIG_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_current_script_sig_hash
, .cmr = {{0x34a3c55du, 0x147374d8u, 0xf3a1761bu, 0xbab28696u, 0x8455404cu, 0x2ca03f69u, 0x393f6339u, 0xd0f58b59u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 126 /* milli weight units */
}
,[CURRENT_SEQUENCE] =
{ .tag = JET
, .jet = simplicity_bitcoin_current_sequence
, .cmr = {{0xc49f76acu, 0x79f8f15cu, 0x409ba815u, 0xc16edcb8u, 0xd11e9a07u, 0x565c8e09u, 0xb63e7fdfu, 0x3103009fu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 66 /* milli weight units */
}
,[CURRENT_VALUE] =
{ .tag = JET
, .jet = simplicity_bitcoin_current_value
, .cmr = {{0x91b96e82u, 0x9e3b4972u, 0xb0cb091au, 0x0a904ba4u, 0x11338abfu, 0xc08da786u, 0xd5a84f04u, 0x9b5ba3b8u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 85 /* milli weight units */
}
,[DECOMPRESS] =
{ .tag = JET
, .jet = simplicity_decompress
, .cmr = {{0x890056dfu, 0x828a766eu, 0xe9f65607u, 0x221e8946u, 0xfa77c256u, 0xbb96e231u, 0xe194d300u, 0x8cf356f6u}}
, .sourceIx = ty_pbw256
, .targetIx = ty_mw512
, .cost = 10495 /* milli weight units */
}
,[DECREMENT_16] =
{ .tag = JET
, .jet = simplicity_decrement_16
, .cmr = {{0x35fda38bu, 0x672c3831u, 0xd8ca11a4u, 0xf3a95962u, 0x22529eb1u, 0xc15f8c70u, 0x5013977du, 0x7dfb5d8bu}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw16
, .cost = 58 /* milli weight units */
}
,[DECREMENT_32] =
{ .tag = JET
, .jet = simplicity_decrement_32
, .cmr = {{0x3b2b1939u, 0x552284f6u, 0x14694ba1u, 0x8dce70ceu, 0xe476ff42u, 0xdcd089e1u, 0xa3c0a42bu, 0xebd108f6u}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw32
, .cost = 57 /* milli weight units */
}
,[DECREMENT_64] =
{ .tag = JET
, .jet = simplicity_decrement_64
, .cmr = {{0x7ef7bdd3u, 0x5db685aeu, 0x99055337u, 0x35a2c7a7u, 0xccbc1708u, 0xae636f93u, 0x1b5ce026u, 0xe5a17fedu}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw64
, .cost = 79 /* milli weight units */
}
,[DECREMENT_8] =
{ .tag = JET
, .jet = simplicity_decrement_8
, .cmr = {{0xe364f2e5u, 0xc08ae011u, 0x8ebe993eu, 0x8b3c958cu, 0x2bcc6062u, 0xa33baab9u, 0x28c04b3eu, 0xc932f51bu}}
, .sourceIx = ty_w8
, .targetIx = ty_pbw8
, .cost = 77 /* milli weight units */
}
,[DIV_MOD_128_64] =
{ .tag = JET
, .jet = simplicity_div_mod_128_64
, .cmr = {{0x9a9443a2u, 0xb541e29fu, 0x272ffd56u, 0x7d1bf742u, 0xd68ccbe9u, 0x538a8729u, 0x1b0ca638u, 0x1563ac2cu}}
, .sourceIx = ty_pw128w64
, .targetIx = ty_w128
, .cost = 169 /* milli weight units */
}
,[DIV_MOD_16] =
{ .tag = JET
, .jet = simplicity_div_mod_16
, .cmr = {{0x39bcb5c0u, 0x1dc1805cu, 0x4919895cu, 0xb59e8f3bu, 0x41446717u, 0xf7ff48fdu, 0xc937dd03u, 0x8024a08au}}
, .sourceIx = ty_w32
, .targetIx = ty_w32
, .cost = 92 /* milli weight units */
}
,[DIV_MOD_32] =
{ .tag = JET
, .jet = simplicity_div_mod_32
, .cmr = {{0xfb1202f4u, 0xe8663a87u, 0xf568992au, 0x185024c7u, 0x0b4f079fu, 0xbe953001u, 0x0f6db284u, 0x218af6cdu}}
, .sourceIx = ty_w64
, .targetIx = ty_w64
, .cost = 90 /* milli weight units */
}
,[DIV_MOD_64] =
{ .tag = JET
, .jet = simplicity_div_mod_64
, .cmr = {{0x6764df5eu, 0x2aa03032u, 0x6ee544c6u, 0xe53ff38eu, 0xf0b28517u, 0x915eec65u, 0xc72ea57au, 0x129828ebu}}
, .sourceIx = ty_w128
, .targetIx = ty_w128
, .cost = 82 /* milli weight units */
}
,[DIV_MOD_8] =
{ .tag = JET
, .jet = simplicity_div_mod_8
, .cmr = {{0xd300244eu, 0x480dd974u, 0x1213e4cbu, 0x0eba836du, 0x3059e778u, 0xb8122f78u, 0x90032673u, 0x739c6a2cu}}
, .sourceIx = ty_w16
, .targetIx = ty_w16
, .cost = 91 /* milli weight units */
}
,[DIVIDE_16] =
{ .tag = JET
, .jet = simplicity_divide_16
, .cmr = {{0x52abfef1u, 0x79754c90u, 0xf9a4260fu, 0x323a8ca4u, 0x95159290u, 0x2b8ecbd6u, 0x4ba42656u, 0xfac05968u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 85 /* milli weight units */
}
,[DIVIDE_32] =
{ .tag = JET
, .jet = simplicity_divide_32
, .cmr = {{0x4a8ae535u, 0x44e147edu, 0x02250423u, 0x7934cc25u, 0x4479bcf9u, 0x3de1e197u, 0x4ddab3bbu, 0x516e606cu}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 82 /* milli weight units */
}
,[DIVIDE_64] =
{ .tag = JET
, .jet = simplicity_divide_64
, .cmr = {{0xd7025d05u, 0xadfae66bu, 0x4710d0ffu, 0x1e87e828u, 0x15573e9cu, 0xb631b4c7u, 0xd13d2f1bu, 0xe4dd26d2u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 81 /* milli weight units */
}
,[DIVIDE_8] =
{ .tag = JET
, .jet = simplicity_divide_8
, .cmr = {{0x40cd1dacu, 0xea24669bu, 0x6a589b61u, 0x475474afu, 0x31d14f8du, 0x46877084u, 0x52d3df37u, 0x30253126u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 85 /* milli weight units */
}
,[DIVIDES_16] =
{ .tag = JET
, .jet = simplicity_divides_16
, .cmr = {{0x10bb1818u, 0x0eab5badu, 0xdc165d03u, 0x37c4ada0u, 0x88e157b1u, 0xaa678334u, 0x2a4520a3u, 0x24dd9d2bu}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 84 /* milli weight units */
}
,[DIVIDES_32] =
{ .tag = JET
, .jet = simplicity_divides_32
, .cmr = {{0xf5e8e78cu, 0x82769a48u, 0xc9103e44u, 0xddb47f84u, 0x1d7693b0u, 0x419e5e7du, 0xa4e68b78u, 0xb237a572u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 80 /* milli weight units */
}
,[DIVIDES_64] =
{ .tag = JET
, .jet = simplicity_divides_64
, .cmr = {{0x9ebd55fau, 0xe418885eu, 0xea04c3cdu, 0xfff531b7u, 0xd714d059u, 0x4fa7da87u, 0xeb6555d3u, 0x6b953db2u}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 67 /* milli weight units */
}
,[DIVIDES_8] =
{ .tag = JET
, .jet = simplicity_divides_8
, .cmr = {{0xa236bc3eu, 0x5cf4d256u, 0x408ba38cu, 0x1eaee736u, 0x9a9c402fu, 0x74bcd1c8u, 0x02f9094fu, 0xbf36803du}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 73 /* milli weight units */
}
,[EQ_1] =
{ .tag = JET
, .jet = simplicity_eq_1
, .cmr = {{0x6549f986u, 0x203a6497u, 0x356e432bu, 0x2aa160d6u, 0xee870b11u, 0x190865bdu, 0x36a47cb0u, 0x470433a5u}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 63 /* milli weight units */
}
,[EQ_16] =
{ .tag = JET
, .jet = simplicity_eq_16
, .cmr = {{0x0c5402b0u, 0xadc8fc65u, 0x701bb75bu, 0x3254c835u, 0xf8fec130u, 0x81cd35e1u, 0x328f2bd7u, 0xdbd23fa6u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 68 /* milli weight units */
}
,[EQ_256] =
{ .tag = JET
, .jet = simplicity_eq_256
, .cmr = {{0x260e1d13u, 0x6dd744fcu, 0xb0507a2du, 0x277027a7u, 0x724354ebu, 0x176b2fbfu, 0x31c6c7d7u, 0xfb3ecd6fu}}
, .sourceIx = ty_w512
, .targetIx = ty_b
, .cost = 188 /* milli weight units */
}
,[EQ_32] =
{ .tag = JET
, .jet = simplicity_eq_32
, .cmr = {{0xf5d6edc8u, 0xb6164e12u, 0x5bbbef08u, 0xc9e08a1eu, 0x6fd492f5u, 0xbdca6fdcu, 0x8b5f5a6fu, 0x05c5ab96u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 74 /* milli weight units */
}
,[EQ_64] =
{ .tag = JET
, .jet = simplicity_eq_64
, .cmr = {{0x1f93acb8u, 0x092fa06du, 0xeaf3c387u, 0xf54a18ffu, 0xeaa69a47u, 0xa6f5caf4u, 0xae497e5cu, 0xc2b36c43u}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 82 /* milli weight units */
}
,[EQ_8] =
{ .tag = JET
, .jet = simplicity_eq_8
, .cmr = {{0xd752fa7fu, 0x51473014u, 0xebb69e1eu, 0x1d2c86d5u, 0x1148b6bau, 0xa02137a4u, 0x8f62d57eu, 0xaf8df1cdu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 76 /* milli weight units */
}
,[FE_ADD] =
{ .tag = JET
, .jet = simplicity_fe_add
, .cmr = {{0xa6c90e02u, 0xfde4ee6eu, 0xef666737u, 0x492e14afu, 0xc8762504u, 0x974af5d5u, 0x472bb943u, 0x3ad2d294u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 777 /* milli weight units */
}
,[FE_INVERT] =
{ .tag = JET
, .jet = simplicity_fe_invert
, .cmr = {{0x7c4abaceu, 0x33c72b3bu, 0xe1fd0ee3u, 0x9fc6cb3eu, 0xe5c8f11eu, 0xf21998c0u, 0x602b5215u, 0xaa2a75c2u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 3237 /* milli weight units */
}
,[FE_IS_ODD] =
{ .tag = JET
, .jet = simplicity_fe_is_odd
, .cmr = {{0x30f5171fu, 0x58f1089du, 0x5dcfb6e6u, 0x683f5adeu, 0x984c0799u, 0x763ca738u, 0x3f75df1cu, 0xa0813efeu}}
, .sourceIx = ty_w256
, .targetIx = ty_b
, .cost = 313 /* milli weight units */
}
,[FE_IS_ZERO] =
{ .tag = JET
, .jet = simplicity_fe_is_zero
, .cmr = {{0xb0b74d86u, 0x51ff557cu, 0xa96044ddu, 0x97281338u, 0xa8f7d3acu, 0xb3847d03u, 0xacbf3d32u, 0xd96fae55u}}
, .sourceIx = ty_w256
, .targetIx = ty_b
, .cost = 277 /* milli weight units */
}
,[FE_MULTIPLY] =
{ .tag = JET
, .jet = simplicity_fe_multiply
, .cmr = {{0x506b9319u, 0xc17a14a9u, 0x469d4627u, 0x61a3303au, 0xb47ddb3au, 0x3079fba3u, 0x4073aa55u, 0x4216a388u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 813 /* milli weight units */
}
,[FE_MULTIPLY_BETA] =
{ .tag = JET
, .jet = simplicity_fe_multiply_beta
, .cmr = {{0x6e180eeau, 0xbe8422b7u, 0x9968e711u, 0xdd00a4b6u, 0x578bb275u, 0xbef47fe5u, 0xff968f14u, 0x72d76f2au}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 607 /* milli weight units */
}
,[FE_NEGATE] =
{ .tag = JET
, .jet = simplicity_fe_negate
, .cmr = {{0xd437ea00u, 0x339880b3u, 0x83d85fb2u, 0xaeaf201bu, 0xbe8ffc83u, 0x705062f9u, 0xc968590du, 0x5db337f6u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 541 /* milli weight units */
}
,[FE_NORMALIZE] =
{ .tag = JET
, .jet = simplicity_fe_normalize
, .cmr = {{0xec0c3dd9u, 0xc5286364u, 0x78bec0e1u, 0x60e50ad9u, 0xbf452c5bu, 0x6f84e940u, 0xe16584ebu, 0x085ace38u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 656 /* milli weight units */
}
,[FE_SQUARE] =
{ .tag = JET
, .jet = simplicity_fe_square
, .cmr = {{0xb904772du, 0x74a185b8u, 0x28eb1547u, 0x28d249c5u, 0x084711e9u, 0xa1832b89u, 0xcaf2af59u, 0xf960e118u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 570 /* milli weight units */
}
,[FE_SQUARE_ROOT] =
{ .tag = JET
, .jet = simplicity_fe_square_root
, .cmr = {{0x16fb9aceu, 0xbe8b5b87u, 0xf2ea7db6u, 0xaa3a2af8u, 0x8ca2b58fu, 0x02cdc87eu, 0x7ce6be0cu, 0x1ffce014u}}
, .sourceIx = ty_w256
, .targetIx = ty_mw256
, .cost = 10162 /* milli weight units */
}
,[FEE] =
{ .tag = JET
, .jet = simplicity_bitcoin_fee
, .cmr = {{0xfb9ca939u, 0x81673d1du, 0x23aed24du, 0x612c1f5du, 0xc7cd49f8u, 0x6d348e67u, 0xa15bc4a7u, 0x130ae185u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 65 /* milli weight units */
}
,[FULL_ADD_16] =
{ .tag = JET
, .jet = simplicity_full_add_16
, .cmr = {{0xc503b078u, 0xdde399c6u, 0x3ac4a232u, 0xbd2a329bu, 0x04308c75u, 0xeaec53a2u, 0xf889b8dfu, 0x0d033472u}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw16
, .cost = 106 /* milli weight units */
}
,[FULL_ADD_32] =
{ .tag = JET
, .jet = simplicity_full_add_32
, .cmr = {{0xa7afd040u, 0xfcb0b2f2u, 0x7190781au, 0xe53a6ccau, 0x00e9fe59u, 0x531115c2u, 0x58ccb69du, 0x3be5a213u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw32
, .cost = 96 /* milli weight units */
}
,[FULL_ADD_64] =
{ .tag = JET
, .jet = simplicity_full_add_64
, .cmr = {{0x80a3ef6cu, 0xdb84ae7cu, 0x8dbcf3a1u, 0x842484c0u, 0x98df6f19u, 0x427a5a4au, 0xdfe76cd5u, 0xff2836cau}}
, .sourceIx = ty_pbw128
, .targetIx = ty_pbw64
, .cost = 93 /* milli weight units */
}
,[FULL_ADD_8] =
{ .tag = JET
, .jet = simplicity_full_add_8
, .cmr = {{0x4b9076b8u, 0xc1ad56c9u, 0xdb6bb3bau, 0xf5938954u, 0x46ce61c7u, 0x4f797eb8u, 0xb230d205u, 0x421c9617u}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw8
, .cost = 131 /* milli weight units */
}
,[FULL_DECREMENT_16] =
{ .tag = JET
, .jet = simplicity_full_decrement_16
, .cmr = {{0xfba3c978u, 0x6ea307f6u, 0xd85434fdu, 0xa2562482u, 0x43a00bacu, 0x9a5353b6u, 0x1ed39c60u, 0x55b693b0u}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw16
, .cost = 60 /* milli weight units */
}
,[FULL_DECREMENT_32] =
{ .tag = JET
, .jet = simplicity_full_decrement_32
, .cmr = {{0x623d21d0u, 0x467922c0u, 0x01c56568u, 0x61d0ddb8u, 0x60c0c9a8u, 0x6bd4cfdcu, 0x37a14c14u, 0x06e3446eu}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw32
, .cost = 71 /* milli weight units */
}
,[FULL_DECREMENT_64] =
{ .tag = JET
, .jet = simplicity_full_decrement_64
, .cmr = {{0x148b3ee1u, 0xf749ea0bu, 0xfba763beu, 0xe999a296u, 0x77456eaeu, 0x9ef53ad8u, 0x78f8b614u, 0x94f08f00u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw64
, .cost = 71 /* milli weight units */
}
,[FULL_DECREMENT_8] =
{ .tag = JET
, .jet = simplicity_full_decrement_8
, .cmr = {{0xb41afe97u, 0x4eaa1182u, 0xac461052u, 0x1e282781u, 0x318ce295u, 0xa3f23f0bu, 0x876ae269u, 0x673fb1dfu}}
, .sourceIx = ty_pbw8
, .targetIx = ty_pbw8
, .cost = 68 /* milli weight units */
}
,[FULL_INCREMENT_16] =
{ .tag = JET
, .jet = simplicity_full_increment_16
, .cmr = {{0xa68eccdbu, 0x9ead2926u, 0xc3e45b4bu, 0xae431cc4u, 0x66d58b8fu, 0xacc95a1bu, 0x4844b912u, 0xdf5676dfu}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw16
, .cost = 70 /* milli weight units */
}
,[FULL_INCREMENT_32] =
{ .tag = JET
, .jet = simplicity_full_increment_32
, .cmr = {{0xd0eb0e94u, 0xa5c25713u, 0xeb944cadu, 0x4d701c6au, 0x968809bcu, 0x1af903fdu, 0xb11e6fadu, 0x0bb31b10u}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw32
, .cost = 57 /* milli weight units */
}
,[FULL_INCREMENT_64] =
{ .tag = JET
, .jet = simplicity_full_increment_64
, .cmr = {{0xc003d2e9u, 0xb0a510c2u, 0xdd783e7du, 0x64eb87b3u, 0x3855d329u, 0x90dfc286u, 0x266e478du, 0xa4e74791u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw64
, .cost = 68 /* milli weight units */
}
,[FULL_INCREMENT_8] =
{ .tag = JET
, .jet = simplicity_full_increment_8
, .cmr = {{0x0bea2429u, 0x18f2dd17u, 0x64777811u, 0xe4442863u, 0x935225b0u, 0xf8b239c2u, 0x3752f9d8u, 0x5392a139u}}
, .sourceIx = ty_pbw8
, .targetIx = ty_pbw8
, .cost = 73 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_1
, .cmr = {{0xb366a816u, 0x922fc455u, 0x010fe88au, 0x5f6a5cf2u, 0xcea917e1u, 0x2bd140aeu, 0x6d43b641u, 0xe57f42b3u}}
, .sourceIx = ty_pw16b
, .targetIx = ty_pbw16
, .cost = 76 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_2
, .cmr = {{0x27960d0du, 0xf2fbbc38u, 0x993d86fbu, 0x8f0cd2c3u, 0x434edb11u, 0x048213c1u, 0x411893cau, 0x9933b2eeu}}
, .sourceIx = ty_pw16w2
, .targetIx = ty_pw2w16
, .cost = 59 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_4
, .cmr = {{0x655137beu, 0xc5c0368fu, 0x29bc992cu, 0x88421a15u, 0x98564039u, 0x7b617fc4u, 0x8d33210fu, 0xc0053ad1u}}
, .sourceIx = ty_pw16w4
, .targetIx = ty_pw4w16
, .cost = 68 /* milli weight units */
}
,[FULL_LEFT_SHIFT_16_8] =
{ .tag = JET
, .jet = simplicity_full_left_shift_16_8
, .cmr = {{0x168f576au, 0xa56ea47eu, 0x070646e7u, 0x8896beb2u, 0x498b1ae6u, 0xb1ff9c78u, 0x6270e955u, 0x65841929u}}
, .sourceIx = ty_pw16w8
, .targetIx = ty_pw8w16
, .cost = 68 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_1
, .cmr = {{0xd7cd5224u, 0x49118e81u, 0x00a7662fu, 0x4df039f8u, 0xcaebf433u, 0xeb039edcu, 0x42e88237u, 0x92ccea8au}}
, .sourceIx = ty_pw32b
, .targetIx = ty_pbw32
, .cost = 58 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_16] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_16
, .cmr = {{0x8bd80d4du, 0x2f8b2246u, 0xc12315c4u, 0x2841b4e4u, 0x0a71ae76u, 0x966a0895u, 0x4d666b86u, 0x32867437u}}
, .sourceIx = ty_pw32w16
, .targetIx = ty_pw16w32
, .cost = 52 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_2
, .cmr = {{0x13063d62u, 0x93832931u, 0x1fb7dabbu, 0x15c3fe58u, 0xc2887683u, 0x0097ecc6u, 0xbfdd480bu, 0xe1988146u}}
, .sourceIx = ty_pw32w2
, .targetIx = ty_pw2w32
, .cost = 73 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_4
, .cmr = {{0x25a1b5ddu, 0xe5db284eu, 0x8a882126u, 0x7c265301u, 0x14bbe671u, 0xcfafb44au, 0x60d75027u, 0x67db782eu}}
, .sourceIx = ty_pw32w4
, .targetIx = ty_pw4w32
, .cost = 59 /* milli weight units */
}
,[FULL_LEFT_SHIFT_32_8] =
{ .tag = JET
, .jet = simplicity_full_left_shift_32_8
, .cmr = {{0xce5470afu, 0xbfadbfbau, 0x68f9b2d5u, 0xb0645a44u, 0x08be6f85u, 0xa69c7f09u, 0xd0964553u, 0x68048764u}}
, .sourceIx = ty_pw32w8
, .targetIx = ty_pw8w32
, .cost = 60 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_1
, .cmr = {{0x051f3605u, 0x86c379acu, 0x2ce399cbu, 0xeb687e77u, 0x53b15d73u, 0x03dd316cu, 0xbd123012u, 0x087cc66fu}}
, .sourceIx = ty_pw64b
, .targetIx = ty_pbw64
, .cost = 74 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_16] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_16
, .cmr = {{0xb248be4du, 0xfcb88c5du, 0x89b1ca61u, 0x86a041e9u, 0x02b4c8a6u, 0x2bb16e09u, 0xfe15616eu, 0x0e3abd6du}}
, .sourceIx = ty_pw64w16
, .targetIx = ty_pw16w64
, .cost = 69 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_2
, .cmr = {{0x34bb5162u, 0x6b1d6b89u, 0x7abc155du, 0x034fe066u, 0x3a0ec0fdu, 0x8f640e5fu, 0xe1bf3cb7u, 0x670a2925u}}
, .sourceIx = ty_pw64w2
, .targetIx = ty_pw2w64
, .cost = 70 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_32] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_32
, .cmr = {{0x9dac8cd7u, 0xfd8b4888u, 0x9e55c5aau, 0x12fe97b7u, 0x29febc04u, 0x1a9fff44u, 0xc4d9b6f1u, 0xe07eb442u}}
, .sourceIx = ty_pw64w32
, .targetIx = ty_pw32w64
, .cost = 73 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_4
, .cmr = {{0x94b73dadu, 0xc3eeeb2eu, 0xe4a4d444u, 0xdd0f72acu, 0x306201f2u, 0xffcf714bu, 0x8ebe7982u, 0x744c0c7eu}}
, .sourceIx = ty_pw64w4
, .targetIx = ty_pw4w64
, .cost = 66 /* milli weight units */
}
,[FULL_LEFT_SHIFT_64_8] =
{ .tag = JET
, .jet = simplicity_full_left_shift_64_8
, .cmr = {{0x0ef71475u, 0x7ecf11cau, 0x3c73ce25u, 0xef24ee72u, 0x95dd4171u, 0xcc16287fu, 0xe6971bd6u, 0x7b478b46u}}
, .sourceIx = ty_pw64w8
, .targetIx = ty_pw8w64
, .cost = 68 /* milli weight units */
}
,[FULL_LEFT_SHIFT_8_1] =
{ .tag = JET
, .jet = simplicity_full_left_shift_8_1
, .cmr = {{0x9bfa48b7u, 0xad510091u, 0xba608544u, 0x62d859efu, 0xd1a2aa18u, 0x731b5f0cu, 0x9e2ba4d8u, 0x9d3aa843u}}
, .sourceIx = ty_pw8b
, .targetIx = ty_pbw8
, .cost = 60 /* milli weight units */
}
,[FULL_LEFT_SHIFT_8_2] =
{ .tag = JET
, .jet = simplicity_full_left_shift_8_2
, .cmr = {{0x797c2087u, 0x01b2a4e1u, 0x049e83d6u, 0x95f554b9u, 0x84f5db28u, 0x21525558u, 0x7c373421u, 0x51b7241du}}
, .sourceIx = ty_pw8w2
, .targetIx = ty_pw2w8
, .cost = 64 /* milli weight units */
}
,[FULL_LEFT_SHIFT_8_4] =
{ .tag = JET
, .jet = simplicity_full_left_shift_8_4
, .cmr = {{0x37bdac91u, 0x538f2219u, 0xcb89df0eu, 0xf9f197cdu, 0x68031f27u, 0x67e894f0u, 0x01c26fffu, 0x5eeb58cdu}}
, .sourceIx = ty_pw8w4
, .targetIx = ty_pw4w8
, .cost = 72 /* milli weight units */
}
,[FULL_MULTIPLY_16] =
{ .tag = JET
, .jet = simplicity_full_multiply_16
, .cmr = {{0x09baff92u, 0x1e9f14d1u, 0x208d1dd8u, 0x264cf1f3u, 0xb854c9afu, 0x21f778b2u, 0xb55a8a42u, 0x6dfe8928u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 99 /* milli weight units */
}
,[FULL_MULTIPLY_32] =
{ .tag = JET
, .jet = simplicity_full_multiply_32
, .cmr = {{0x10c4b8c4u, 0xc0acd973u, 0x90f85cb3u, 0xf5ffe36au, 0x292037c1u, 0x90eebab3u, 0xe98934feu, 0x93b2ed90u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 87 /* milli weight units */
}
,[FULL_MULTIPLY_64] =
{ .tag = JET
, .jet = simplicity_full_multiply_64
, .cmr = {{0x2db19dbau, 0x90ef867bu, 0x5a3e914bu, 0x89fda2dau, 0x637ca80cu, 0x4267e198u, 0x1837ee3cu, 0x6fe3daf5u}}
, .sourceIx = ty_w256
, .targetIx = ty_w128
, .cost = 103 /* milli weight units */
}
,[FULL_MULTIPLY_8] =
{ .tag = JET
, .jet = simplicity_full_multiply_8
, .cmr = {{0x7f46ee72u, 0x84f39e73u, 0x4275a250u, 0x9a0b737eu, 0xd939115fu, 0x0219a574u, 0xd469cd30u, 0xb819efe3u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 95 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_1
, .cmr = {{0x7ebe0c66u, 0xc3c7dc16u, 0xa5469e91u, 0x79098417u, 0xac5f20a3u, 0x9cc41ac3u, 0x82fb1dbdu, 0x98e8e30fu}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pw16b
, .cost = 55 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_2
, .cmr = {{0x8db0c216u, 0x19c62d63u, 0xd4c27bfcu, 0xf647d709u, 0xce37bed0u, 0x5718e93eu, 0x4515e29eu, 0xf3730cf4u}}
, .sourceIx = ty_pw2w16
, .targetIx = ty_pw16w2
, .cost = 60 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_4] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_4
, .cmr = {{0x5c74b132u, 0x06317917u, 0xe070e5fcu, 0x1c82f4c5u, 0xc2fbe9f3u, 0x1b812946u, 0xba230d8cu, 0x94d40616u}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_pw16w4
, .cost = 64 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_16_8] =
{ .tag = JET
, .jet = simplicity_full_right_shift_16_8
, .cmr = {{0x1105818au, 0xc948d7bbu, 0x634707e6u, 0x9dbf1f67u, 0x9058a13du, 0x35fac2a6u, 0x4df97262u, 0xf242b63bu}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_pw16w8
, .cost = 55 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_1
, .cmr = {{0x9b42c8f3u, 0x3bc5750eu, 0x2a83aadbu, 0xf29cc7fcu, 0xb950fe5au, 0x40aa0ec5u, 0x2452e533u, 0xf825a115u}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pw32b
, .cost = 49 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_16] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_16
, .cmr = {{0x0ae5659cu, 0x2fa75794u, 0x78ebd57cu, 0x4c98aee7u, 0x77015645u, 0xb2843181u, 0x64fcbd30u, 0x65fc873cu}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_pw32w16
, .cost = 48 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_2
, .cmr = {{0x57fb1c03u, 0xc2eb17f6u, 0x23478734u, 0xfd6937f9u, 0xe3ef027cu, 0x1560038fu, 0xa6066905u, 0x1789e368u}}
, .sourceIx = ty_pw2w32
, .targetIx = ty_pw32w2
, .cost = 66 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_4] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_4
, .cmr = {{0x8582cdfau, 0x74ef466bu, 0x8127b197u, 0x88134593u, 0x998e4969u, 0x00b38f0fu, 0x3d375818u, 0xd673451eu}}
, .sourceIx = ty_pw4w32
, .targetIx = ty_pw32w4
, .cost = 49 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_32_8] =
{ .tag = JET
, .jet = simplicity_full_right_shift_32_8
, .cmr = {{0xd905932eu, 0xbfca2a38u, 0x619d807eu, 0x28ff2e0du, 0x3be08a26u, 0x0676d257u, 0xefa040c3u, 0x05aadc33u}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_pw32w8
, .cost = 66 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_1
, .cmr = {{0x3c15209bu, 0x99d2845eu, 0x225e14e1u, 0xe9e5e6a4u, 0x878bc8ceu, 0xa3f9f36bu, 0x8b535ac6u, 0x83e29d00u}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pw64b
, .cost = 60 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_16] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_16
, .cmr = {{0x0285257bu, 0x090d8da1u, 0x28ef64a8u, 0x0c8d16fdu, 0xc3bf5ce5u, 0x0fcd56feu, 0xc5f90255u, 0xd9c8df47u}}
, .sourceIx = ty_pw16w64
, .targetIx = ty_pw64w16
, .cost = 73 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_2
, .cmr = {{0x7ec2dd65u, 0xc9e013e3u, 0xe4ce90fbu, 0xeb3fb1c7u, 0x8ccc5d2au, 0x7d26d8afu, 0x77f99de8u, 0x4cf72973u}}
, .sourceIx = ty_pw2w64
, .targetIx = ty_pw64w2
, .cost = 76 /* milli weight units */
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
, .cmr = {{0x05464a33u, 0x35afbb09u, 0xd046828au, 0x922c4da0u, 0xeceeb109u, 0x77e46801u, 0xc93cdd66u, 0x8f22ee63u}}
, .sourceIx = ty_pw4w64
, .targetIx = ty_pw64w4
, .cost = 56 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_64_8] =
{ .tag = JET
, .jet = simplicity_full_right_shift_64_8
, .cmr = {{0x70172e1au, 0x6948bf40u, 0x120e68fbu, 0x8b4b23bcu, 0x355a1200u, 0x2ccc1db6u, 0x47c89b12u, 0xd10ec506u}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_pw64w8
, .cost = 68 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_8_1] =
{ .tag = JET
, .jet = simplicity_full_right_shift_8_1
, .cmr = {{0x5669dbfcu, 0xc633ec0bu, 0xdf59e22fu, 0x03ed4b64u, 0x192095f5u, 0xdf20ffc1u, 0x2dd90d7cu, 0xda11374fu}}
, .sourceIx = ty_pbw8
, .targetIx = ty_pw8b
, .cost = 59 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_8_2] =
{ .tag = JET
, .jet = simplicity_full_right_shift_8_2
, .cmr = {{0x1f944361u, 0x09df52b3u, 0x45fa3a89u, 0xac2a49edu, 0xc9d285f2u, 0x1f45ed11u, 0xd775f7f7u, 0xf39d3e8fu}}
, .sourceIx = ty_pw2w8
, .targetIx = ty_pw8w2
, .cost = 49 /* milli weight units */
}
,[FULL_RIGHT_SHIFT_8_4] =
{ .tag = JET
, .jet = simplicity_full_right_shift_8_4
, .cmr = {{0x714698a2u, 0x7684b5bau, 0xa6b6480eu, 0xe3b257cbu, 0xb7cdab74u, 0x72f371a6u, 0x270618c0u, 0xab12908bu}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_pw8w4
, .cost = 51 /* milli weight units */
}
,[FULL_SUBTRACT_16] =
{ .tag = JET
, .jet = simplicity_full_subtract_16
, .cmr = {{0x40096152u, 0xb54e7425u, 0x4555a65du, 0xccc629dfu, 0x57b979c8u, 0x47005450u, 0x36fe190au, 0x6af3d38au}}
, .sourceIx = ty_pbw32
, .targetIx = ty_pbw16
, .cost = 99 /* milli weight units */
}
,[FULL_SUBTRACT_32] =
{ .tag = JET
, .jet = simplicity_full_subtract_32
, .cmr = {{0xe7930d64u, 0x35a9680bu, 0xefb49db7u, 0xd87c2f50u, 0xafd46d98u, 0x880ded50u, 0xe5055fa3u, 0x09e1afcau}}
, .sourceIx = ty_pbw64
, .targetIx = ty_pbw32
, .cost = 92 /* milli weight units */
}
,[FULL_SUBTRACT_64] =
{ .tag = JET
, .jet = simplicity_full_subtract_64
, .cmr = {{0xff281df8u, 0xc42a3159u, 0xd9ffa925u, 0x16ca893eu, 0x23b0eb93u, 0x8b4cb0b3u, 0xf134468eu, 0x9f4ebc46u}}
, .sourceIx = ty_pbw128
, .targetIx = ty_pbw64
, .cost = 109 /* milli weight units */
}
,[FULL_SUBTRACT_8] =
{ .tag = JET
, .jet = simplicity_full_subtract_8
, .cmr = {{0x7e3dcfe4u, 0x56ae3c5cu, 0x87debf04u, 0x7189c274u, 0x82a4ff4eu, 0x8cfd1f17u, 0x30c87d2bu, 0x7bff73bau}}
, .sourceIx = ty_pbw16
, .targetIx = ty_pbw8
, .cost = 106 /* milli weight units */
}
,[GE_IS_ON_CURVE] =
{ .tag = JET
, .jet = simplicity_ge_is_on_curve
, .cmr = {{0x7d448719u, 0xf5f9572bu, 0xf5402e12u, 0xd193aff6u, 0x77482d66u, 0xff3dcf27u, 0x48f25c6bu, 0x7377028cu}}
, .sourceIx = ty_w512
, .targetIx = ty_b
, .cost = 688 /* milli weight units */
}
,[GE_NEGATE] =
{ .tag = JET
, .jet = simplicity_ge_negate
, .cmr = {{0x3d2c8de4u, 0xc7015fd3u, 0x132695fdu, 0x66dfcf0fu, 0x1778c791u, 0x85268e9fu, 0xae7789dau, 0x538eca59u}}
, .sourceIx = ty_w512
, .targetIx = ty_w512
, .cost = 1071 /* milli weight units */
}
,[GEJ_ADD] =
{ .tag = JET
, .jet = simplicity_gej_add
, .cmr = {{0x45ba7f3du, 0x1e1e6d34u, 0x9fcf8698u, 0x7b0e7f7au, 0xce662e82u, 0x201d3502u, 0x60454e2fu, 0xfeecb54du}}
, .sourceIx = ty_ppw512w256pw512w256
, .targetIx = ty_pw512w256
, .cost = 3000 /* milli weight units */
}
,[GEJ_DOUBLE] =
{ .tag = JET
, .jet = simplicity_gej_double
, .cmr = {{0x23e978f3u, 0x4154119bu, 0xdefc5d13u, 0xfcfd0a34u, 0xa75e3726u, 0xd6cb2581u, 0x3370ad7du, 0x9de5e133u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_pw512w256
, .cost = 1862 /* milli weight units */
}
,[GEJ_EQUIV] =
{ .tag = JET
, .jet = simplicity_gej_equiv
, .cmr = {{0xb94b2aacu, 0x73a67f44u, 0x95859913u, 0x4de23017u, 0x9e9d6bb6u, 0x47fd0611u, 0x158aaba7u, 0x0b73e400u}}
, .sourceIx = ty_ppw512w256pw512w256
, .targetIx = ty_b
, .cost = 2376 /* milli weight units */
}
,[GEJ_GE_ADD] =
{ .tag = JET
, .jet = simplicity_gej_ge_add
, .cmr = {{0xf1160b6fu, 0x5ee2c582u, 0xe49566e6u, 0xc386b380u, 0x94abc1a7u, 0x182d33a1u, 0x501fa2aau, 0xf00af3eau}}
, .sourceIx = ty_ppw512w256w512
, .targetIx = ty_pw512w256
, .cost = 2609 /* milli weight units */
}
,[GEJ_GE_ADD_EX] =
{ .tag = JET
, .jet = simplicity_gej_ge_add_ex
, .cmr = {{0xc3d7347fu, 0xfe2d9c83u, 0x9aac567eu, 0x2998e016u, 0xaf394e2au, 0x1929314bu, 0x52e31eedu, 0x678e30bfu}}
, .sourceIx = ty_ppw512w256w512
, .targetIx = ty_pw256pw512w256
, .cost = 2860 /* milli weight units */
}
,[GEJ_GE_EQUIV] =
{ .tag = JET
, .jet = simplicity_gej_ge_equiv
, .cmr = {{0x27c29969u, 0x139f8d57u, 0xedc9895cu, 0x30403df0u, 0x15c50ce7u, 0x21c381fbu, 0x197c0c04u, 0x03f1db0cu}}
, .sourceIx = ty_ppw512w256w512
, .targetIx = ty_b
, .cost = 1823 /* milli weight units */
}
,[GEJ_INFINITY] =
{ .tag = JET
, .jet = simplicity_gej_infinity
, .cmr = {{0xaafb9380u, 0xd61a7f14u, 0x7846806bu, 0x2cc374fbu, 0xe82dd1aeu, 0xd485b98au, 0x0f164b3au, 0x54c2c0b0u}}
, .sourceIx = ty_u
, .targetIx = ty_pw512w256
, .cost = 765 /* milli weight units */
}
,[GEJ_IS_INFINITY] =
{ .tag = JET
, .jet = simplicity_gej_is_infinity
, .cmr = {{0xdb495fd1u, 0x3142e9b3u, 0x3763fc6du, 0x48d2fb0eu, 0x71b0d9d9u, 0x9bd726f4u, 0x7ad13fc5u, 0x560670a2u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_b
, .cost = 701 /* milli weight units */
}
,[GEJ_IS_ON_CURVE] =
{ .tag = JET
, .jet = simplicity_gej_is_on_curve
, .cmr = {{0xbf4ca13fu, 0xf212e34bu, 0xf17d90c1u, 0x2e453d08u, 0xac7daa4au, 0x47d57e85u, 0xb43f2d43u, 0x66d43ddau}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_b
, .cost = 1039 /* milli weight units */
}
,[GEJ_NEGATE] =
{ .tag = JET
, .jet = simplicity_gej_negate
, .cmr = {{0x01bd1a35u, 0x1fb8164cu, 0x813d916du, 0x07774999u, 0x6b7db118u, 0xd31586cau, 0x9d75e756u, 0x3518f454u}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_pw512w256
, .cost = 1549 /* milli weight units */
}
,[GEJ_NORMALIZE] =
{ .tag = JET
, .jet = simplicity_gej_normalize
, .cmr = {{0xec597d17u, 0xe2efb6d2u, 0xa002d50eu, 0x677527d3u, 0xd4a2907au, 0x119d68f1u, 0x2284b9a1u, 0xb0d2303au}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_mw512
, .cost = 4184 /* milli weight units */
}
,[GEJ_RESCALE] =
{ .tag = JET
, .jet = simplicity_gej_rescale
, .cmr = {{0x2977d953u, 0xef7a1156u, 0xcec6db2du, 0xc2925412u, 0x75cbc82fu, 0xb829fd67u, 0x1b972e89u, 0xebed0c24u}}
, .sourceIx = ty_ppw512w256w256
, .targetIx = ty_pw512w256
, .cost = 2011 /* milli weight units */
}
,[GEJ_X_EQUIV] =
{ .tag = JET
, .jet = simplicity_gej_x_equiv
, .cmr = {{0xf9f189fcu, 0x00b61f72u, 0xf10baaa2u, 0x1bcd88e5u, 0xd22e0aa9u, 0xb7509ae1u, 0x62a183a4u, 0xb664a4afu}}
, .sourceIx = ty_pw256pw512w256
, .targetIx = ty_b
, .cost = 1103 /* milli weight units */
}
,[GEJ_Y_IS_ODD] =
{ .tag = JET
, .jet = simplicity_gej_y_is_odd
, .cmr = {{0x9eb6e453u, 0x5fb69bf6u, 0x09916599u, 0xf1345ad7u, 0x735da3f3u, 0x948d0686u, 0x908e44f4u, 0x5b2ff60cu}}
, .sourceIx = ty_pw512w256
, .targetIx = ty_b
, .cost = 3702 /* milli weight units */
}
,[GENERATE] =
{ .tag = JET
, .jet = simplicity_generate
, .cmr = {{0x148885acu, 0x73813113u, 0xc523e809u, 0xbea47ffdu, 0x8b1daf37u, 0x8d9dd54bu, 0xf966ccb8u, 0x83b1a984u}}
, .sourceIx = ty_w256
, .targetIx = ty_pw512w256
, .cost = 49851 /* milli weight units */
}
,[HASH_TO_CURVE] =
{ .tag = JET
, .jet = simplicity_hash_to_curve
, .cmr = {{0xef4f548bu, 0x3c6c7517u, 0x5f2ce2d1u, 0x993b2d19u, 0x9beb16c0u, 0xa140175cu, 0x48a1277eu, 0xfc43a99bu}}
, .sourceIx = ty_w256
, .targetIx = ty_w512
, .cost = 69844 /* milli weight units */
}
,[HIGH_1] =
{ .tag = JET
, .jet = simplicity_high_1
, .cmr = {{0xb109cf1cu, 0xce35f7e9u, 0xb649671au, 0x9b45dbc2u, 0x4099a713u, 0xaeb9a89cu, 0xc4cf6ef6u, 0xed8b308bu}}
, .sourceIx = ty_u
, .targetIx = ty_b
, .cost = 42 /* milli weight units */
}
,[HIGH_16] =
{ .tag = JET
, .jet = simplicity_high_16
, .cmr = {{0x035dadd9u, 0xd7bf7433u, 0x6445e71du, 0xdc4d8202u, 0x24ff7e38u, 0xe0b8d52bu, 0xec9729b5u, 0x72b531f9u}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 50 /* milli weight units */
}
,[HIGH_32] =
{ .tag = JET
, .jet = simplicity_high_32
, .cmr = {{0xc5f1df0du, 0x64a2737au, 0x631b3aaeu, 0x8f260e8bu, 0x8dc1957bu, 0xd092911bu, 0x91d2078au, 0xd21e418au}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 64 /* milli weight units */
}
,[HIGH_64] =
{ .tag = JET
, .jet = simplicity_high_64
, .cmr = {{0xa312633eu, 0x0a2305e6u, 0x9b3f341du, 0x91d683ddu, 0x94196a2fu, 0x9005c9b1u, 0x872a2c15u, 0xad46cf17u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 52 /* milli weight units */
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
, .cmr = {{0x86774949u, 0x39b27b86u, 0xcb5a8c7fu, 0x8172ad55u, 0x509531c9u, 0xb0e11e99u, 0x757e296cu, 0xc3c7c192u}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw16
, .cost = 56 /* milli weight units */
}
,[INCREMENT_32] =
{ .tag = JET
, .jet = simplicity_increment_32
, .cmr = {{0x6bdbab7cu, 0xfc16c503u, 0x363c2f07u, 0x7e02c335u, 0xda406175u, 0xd192fbefu, 0x50c07fc2u, 0x79b3f40cu}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw32
, .cost = 73 /* milli weight units */
}
,[INCREMENT_64] =
{ .tag = JET
, .jet = simplicity_increment_64
, .cmr = {{0x20e75e71u, 0x7cb76d46u, 0x95564f7cu, 0x20221b7au, 0x01431387u, 0x38f151aau, 0x195eb170u, 0xec13c049u}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw64
, .cost = 64 /* milli weight units */
}
,[INCREMENT_8] =
{ .tag = JET
, .jet = simplicity_increment_8
, .cmr = {{0x5f4e056eu, 0xf4ed8d68u, 0xbf911fc5u, 0xcb69037eu, 0xbf6c9221u, 0x7343a890u, 0x5d38c432u, 0xc183233cu}}
, .sourceIx = ty_w8
, .targetIx = ty_pbw8
, .cost = 69 /* milli weight units */
}
,[INPUT_ANNEX_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_annex_hash
, .cmr = {{0x945b147eu, 0x5f0edb83u, 0x2d62348cu, 0xaeeac224u, 0x56eee944u, 0x65376dbfu, 0x596b9d62u, 0x985b01b6u}}
, .sourceIx = ty_w32
, .targetIx = ty_mmw256
, .cost = 77 /* milli weight units */
}
,[INPUT_ANNEXES_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_annexes_hash
, .cmr = {{0x89b60270u, 0x44141f20u, 0x65b6f236u, 0xcfcc13b9u, 0x68485e00u, 0x746b7859u, 0x286903c6u, 0x8c7f880du}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 124 /* milli weight units */
}
,[INPUT_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_hash
, .cmr = {{0x3ad22231u, 0xd6b9dff6u, 0xa0b4dbcfu, 0xf044e11cu, 0x082e0468u, 0x4e73ce95u, 0xc205e0c4u, 0x496e5ecau}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 832 /* milli weight units */
}
,[INPUT_OUTPOINTS_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_outpoints_hash
, .cmr = {{0x1759c6f4u, 0x70b9aba6u, 0x2a31791eu, 0xa01056e3u, 0x608bdf22u, 0xf5dd43bfu, 0x7db00ea5u, 0x823f7cceu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 126 /* milli weight units */
}
,[INPUT_PREV_OUTPOINT] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_prev_outpoint
, .cmr = {{0x5d171242u, 0x09ee0521u, 0x24e55238u, 0xe0b6a6feu, 0x85a48688u, 0xc1e15e61u, 0x81de94f7u, 0x8db04018u}}
, .sourceIx = ty_w32
, .targetIx = ty_mpw256w32
, .cost = 140 /* milli weight units */
}
,[INPUT_SCRIPT_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_script_hash
, .cmr = {{0x0562412fu, 0x021da481u, 0x9b019c8cu, 0xd887d530u, 0x492b9f94u, 0x2c4ebf21u, 0xae39ed32u, 0x330d7cfeu}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 143 /* milli weight units */
}
,[INPUT_SCRIPT_SIG_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_script_sig_hash
, .cmr = {{0x1c7efb37u, 0xcbc4b996u, 0xd9b8c617u, 0x08e66573u, 0xce87c1f0u, 0xaa0522d0u, 0x65ba9025u, 0x056e3f80u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 137 /* milli weight units */
}
,[INPUT_SCRIPT_SIGS_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_script_sigs_hash
, .cmr = {{0xe729c5f5u, 0x8a938908u, 0x1a5ca845u, 0xf76cb980u, 0xf08599ceu, 0xb3c7d7eau, 0xe1145b53u, 0xa60ae3d5u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 127 /* milli weight units */
}
,[INPUT_SCRIPTS_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_scripts_hash
, .cmr = {{0xf177671du, 0x60960e46u, 0x3a9a8f7eu, 0x1f5213aau, 0xba9122d2u, 0xdb75fe91u, 0xf8f1cf91u, 0xbe001907u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 129 /* milli weight units */
}
,[INPUT_SEQUENCE] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_sequence
, .cmr = {{0x5dd17fe1u, 0x550f48f7u, 0xbaed4d06u, 0x8008d4a1u, 0xff98cbebu, 0xe2541c5du, 0xc77ac53au, 0xd83fa779u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw32
, .cost = 78 /* milli weight units */
}
,[INPUT_SEQUENCES_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_sequences_hash
, .cmr = {{0x55973d64u, 0x43457472u, 0x81377364u, 0xbad2a80bu, 0x758f4566u, 0x60c18de5u, 0xc01a3882u, 0x071d50e8u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 125 /* milli weight units */
}
,[INPUT_UTXO_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_utxo_hash
, .cmr = {{0x931e4e95u, 0xe0a64760u, 0x276e91b5u, 0xdc746780u, 0xd0697d0au, 0xf5aaf5bbu, 0xc81dbeb3u, 0x98596abbu}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 824 /* milli weight units */
}
,[INPUT_UTXOS_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_utxos_hash
, .cmr = {{0xd6f90cd1u, 0x04e1a5c6u, 0x1a4b5000u, 0xad9aba8du, 0x43004bf9u, 0x43df325fu, 0xa636d1a2u, 0x2beca0cbu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 122 /* milli weight units */
}
,[INPUT_VALUE] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_value
, .cmr = {{0x7d3c3f95u, 0x5b2cf0d0u, 0xd1280a1bu, 0xb1204692u, 0x92d1329cu, 0x83a9c2ffu, 0x7e7e1eb3u, 0xf69783a3u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw64
, .cost = 81 /* milli weight units */
}
,[INPUT_VALUES_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_input_values_hash
, .cmr = {{0x29839eadu, 0x0eb03fe4u, 0x6542e36du, 0x71e9e6afu, 0xdf969301u, 0x533d74eeu, 0x099b1266u, 0xa250552cu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 126 /* milli weight units */
}
,[INPUTS_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_inputs_hash
, .cmr = {{0xabbfe1c7u, 0xd115c419u, 0x1f504839u, 0xf98c3f20u, 0x422b84e7u, 0xfa14da14u, 0x02896c4du, 0x98bfa8d8u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 122 /* milli weight units */
}
,[INTERNAL_KEY] =
{ .tag = JET
, .jet = simplicity_bitcoin_internal_key
, .cmr = {{0x37483699u, 0x2810022fu, 0x88e0145bu, 0xcad77f4au, 0x8491fa80u, 0x83cb51c3u, 0x01fcf7a1u, 0x3478c2ccu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 124 /* milli weight units */
}
,[IS_ONE_16] =
{ .tag = JET
, .jet = simplicity_is_one_16
, .cmr = {{0x1bd3a253u, 0xdb243fcau, 0x45533799u, 0xfe914838u, 0xc38e3806u, 0xb12bd7e8u, 0x5ca71207u, 0xa88462b0u}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 64 /* milli weight units */
}
,[IS_ONE_32] =
{ .tag = JET
, .jet = simplicity_is_one_32
, .cmr = {{0x78b1bae0u, 0x99ec9c59u, 0xcbf41262u, 0x51c1e967u, 0x41b350d5u, 0x63bd74d5u, 0x4418ba78u, 0xebea25bfu}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 64 /* milli weight units */
}
,[IS_ONE_64] =
{ .tag = JET
, .jet = simplicity_is_one_64
, .cmr = {{0x817b95a5u, 0x395efbecu, 0xbb8515a5u, 0x5b3ffe1au, 0x4d7bac6eu, 0x23dbca54u, 0xad606666u, 0x2f202b93u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 66 /* milli weight units */
}
,[IS_ONE_8] =
{ .tag = JET
, .jet = simplicity_is_one_8
, .cmr = {{0xf6925491u, 0xd34b3774u, 0x2cb08decu, 0x193ee512u, 0x5f933cadu, 0xcc232aedu, 0xeedb572du, 0x1260ffd5u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 47 /* milli weight units */
}
,[IS_ZERO_16] =
{ .tag = JET
, .jet = simplicity_is_zero_16
, .cmr = {{0x1ba7213bu, 0x588be092u, 0xb446599cu, 0x2a60ff54u, 0x67136a79u, 0x7599610bu, 0xd7a5f178u, 0x04e32a2cu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 52 /* milli weight units */
}
,[IS_ZERO_32] =
{ .tag = JET
, .jet = simplicity_is_zero_32
, .cmr = {{0x5ebf1466u, 0x93f0e2d2u, 0xf9361b47u, 0x6dba3485u, 0x8b832d66u, 0xfacf713bu, 0xfb32c3bbu, 0x8db9eebfu}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 58 /* milli weight units */
}
,[IS_ZERO_64] =
{ .tag = JET
, .jet = simplicity_is_zero_64
, .cmr = {{0x19ab9ac0u, 0xcf426682u, 0x19ba6cb8u, 0x97e487feu, 0x3680937fu, 0xffa8d203u, 0x511db75du, 0xbb10c7e5u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 68 /* milli weight units */
}
,[IS_ZERO_8] =
{ .tag = JET
, .jet = simplicity_is_zero_8
, .cmr = {{0x8eff6208u, 0x4407e9afu, 0xd540f318u, 0xf66bcf31u, 0xdf1d42a5u, 0xc161cae3u, 0x5a294818u, 0x0ca2aa2eu}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 59 /* milli weight units */
}
,[LE_16] =
{ .tag = JET
, .jet = simplicity_le_16
, .cmr = {{0x016705a7u, 0xd7dce1afu, 0xc63eab84u, 0x203f5f42u, 0xd6b6bbadu, 0x75cee38cu, 0xec5a515bu, 0x5997489fu}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 83 /* milli weight units */
}
,[LE_32] =
{ .tag = JET
, .jet = simplicity_le_32
, .cmr = {{0x5351fc5du, 0xebe5b298u, 0xad7057e4u, 0xa5a76a3bu, 0x9c658acdu, 0xe7d1bb52u, 0xe5889ca1u, 0xe38f5efbu}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 99 /* milli weight units */
}
,[LE_64] =
{ .tag = JET
, .jet = simplicity_le_64
, .cmr = {{0xae2de1e0u, 0xcf730d1du, 0xcc96d7ccu, 0xfe71168au, 0x240deaf8u, 0x04615a7bu, 0xa920dc16u, 0xfd6ea45fu}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 79 /* milli weight units */
}
,[LE_8] =
{ .tag = JET
, .jet = simplicity_le_8
, .cmr = {{0xaf29f616u, 0x8ebdc09eu, 0xfbe0e639u, 0xcb750b12u, 0x05788f90u, 0x21d666efu, 0xcefe13f1u, 0x2f9671f0u}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 93 /* milli weight units */
}
,[LEFT_EXTEND_16_32] =
{ .tag = JET
, .jet = simplicity_left_extend_16_32
, .cmr = {{0x289997fbu, 0xa1fae7ecu, 0x1c4531c5u, 0x0bbf8671u, 0xb897139bu, 0xdd3aad97u, 0xa3763957u, 0x4a047c80u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 72 /* milli weight units */
}
,[LEFT_EXTEND_16_64] =
{ .tag = JET
, .jet = simplicity_left_extend_16_64
, .cmr = {{0x5dff21f6u, 0xe6124775u, 0xc578eaf4u, 0x855c0b01u, 0x64f7879bu, 0x1760f902u, 0x7cb50f7bu, 0x5acb4918u}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 69 /* milli weight units */
}
,[LEFT_EXTEND_1_16] =
{ .tag = JET
, .jet = simplicity_left_extend_1_16
, .cmr = {{0x8c87d756u, 0xd14bd3d9u, 0xa7869081u, 0x2912b894u, 0x29c0171au, 0x41103a58u, 0xc6e9f225u, 0x141a0222u}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 50 /* milli weight units */
}
,[LEFT_EXTEND_1_32] =
{ .tag = JET
, .jet = simplicity_left_extend_1_32
, .cmr = {{0xc8f154d4u, 0x6d2e7895u, 0xda1b33c2u, 0xb315e6d4u, 0xd4851ddeu, 0xe28aef8bu, 0x70709061u, 0x6bc7eea0u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 48 /* milli weight units */
}
,[LEFT_EXTEND_1_64] =
{ .tag = JET
, .jet = simplicity_left_extend_1_64
, .cmr = {{0xa3404df6u, 0x8cc92075u, 0x4c6e1847u, 0x207db384u, 0x5d11c749u, 0x09d07ca8u, 0x2ad1f1ccu, 0x67bf3a9bu}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 49 /* milli weight units */
}
,[LEFT_EXTEND_1_8] =
{ .tag = JET
, .jet = simplicity_left_extend_1_8
, .cmr = {{0x3bca3397u, 0xb83c27f3u, 0x6316f8b8u, 0xb303350au, 0xfe8ba007u, 0x8f77f1d4u, 0x2a9b7892u, 0xb2a4dbeeu}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 46 /* milli weight units */
}
,[LEFT_EXTEND_32_64] =
{ .tag = JET
, .jet = simplicity_left_extend_32_64
, .cmr = {{0x42cbeb01u, 0xfe7a3a6du, 0xd3311db3u, 0x365f91e5u, 0xc118c7e4u, 0x1f03aae7u, 0xb283de6bu, 0xb9053e6bu}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 69 /* milli weight units */
}
,[LEFT_EXTEND_8_16] =
{ .tag = JET
, .jet = simplicity_left_extend_8_16
, .cmr = {{0x9a57c96au, 0xf5714896u, 0xb724de45u, 0xeb9fe97du, 0x73697de6u, 0x2e8dad78u, 0x71eb58f5u, 0x81a011bbu}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 58 /* milli weight units */
}
,[LEFT_EXTEND_8_32] =
{ .tag = JET
, .jet = simplicity_left_extend_8_32
, .cmr = {{0xd624bd40u, 0x40763cb1u, 0x3ccad498u, 0xf53d38c1u, 0x12f19295u, 0x6826dafeu, 0xc9ac9165u, 0x792b347au}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 86 /* milli weight units */
}
,[LEFT_EXTEND_8_64] =
{ .tag = JET
, .jet = simplicity_left_extend_8_64
, .cmr = {{0x9dc4a205u, 0x4d5d2634u, 0x2ac590b6u, 0x67f1b01du, 0xf54fd0cdu, 0xaa405ef8u, 0xcbb76fd8u, 0xf9b00ee5u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 98 /* milli weight units */
}
,[LEFT_PAD_HIGH_16_32] =
{ .tag = JET
, .jet = simplicity_left_pad_high_16_32
, .cmr = {{0x0545c4b5u, 0x8f004a21u, 0xe7f129a4u, 0xc0518997u, 0x1714caa2u, 0xd91d1dfdu, 0x5fad3e63u, 0x24499428u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 71 /* milli weight units */
}
,[LEFT_PAD_HIGH_16_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_16_64
, .cmr = {{0x1c61d03du, 0x493bbd05u, 0x822259d1u, 0x730a8d7au, 0x5f55b0bau, 0x2a9391a6u, 0xc8881eb4u, 0x7504affdu}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 82 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_16] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_16
, .cmr = {{0x56fdf54fu, 0x1fcd1982u, 0x5e7c3b79u, 0x0615c1d3u, 0xfe82886cu, 0x747bc487u, 0x5987f505u, 0x16945fb3u}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 106 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_32] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_32
, .cmr = {{0xdb33059au, 0xbe2d432du, 0x67f42b1eu, 0x942756dcu, 0xa6cde637u, 0x85e5bd43u, 0x0dc8f4aeu, 0xfc31b8dfu}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 220 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_64
, .cmr = {{0x1d669c1fu, 0xa5fd3ef6u, 0x6eb4aef6u, 0x186e3ec1u, 0x36ee7584u, 0x10df3edeu, 0xbb31bf26u, 0xd4562051u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 302 /* milli weight units */
}
,[LEFT_PAD_HIGH_1_8] =
{ .tag = JET
, .jet = simplicity_left_pad_high_1_8
, .cmr = {{0x9a1bad3du, 0x8ab90030u, 0x3da202f0u, 0xf449f0b7u, 0xe6795c2au, 0x7c121718u, 0x800ac40cu, 0x87d82729u}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 73 /* milli weight units */
}
,[LEFT_PAD_HIGH_32_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_32_64
, .cmr = {{0x3920cc4bu, 0x33baf7efu, 0xa5caf9e7u, 0x80014467u, 0x06f6e4e8u, 0x26567405u, 0x7eed8717u, 0x78089e94u}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 69 /* milli weight units */
}
,[LEFT_PAD_HIGH_8_16] =
{ .tag = JET
, .jet = simplicity_left_pad_high_8_16
, .cmr = {{0x752e29f2u, 0xfe2becc3u, 0xf66290feu, 0x44e1aeb3u, 0x784180ddu, 0x905e1962u, 0x4e195f21u, 0x6c07c57cu}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 65 /* milli weight units */
}
,[LEFT_PAD_HIGH_8_32] =
{ .tag = JET
, .jet = simplicity_left_pad_high_8_32
, .cmr = {{0xbee88f1cu, 0x8c30634cu, 0x6e95caccu, 0x0e9add49u, 0x413221fdu, 0xabbd8d4cu, 0x0accf1cau, 0xe2d2a778u}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 105 /* milli weight units */
}
,[LEFT_PAD_HIGH_8_64] =
{ .tag = JET
, .jet = simplicity_left_pad_high_8_64
, .cmr = {{0x392387f6u, 0xdc04bfc5u, 0x4dd4a281u, 0x19c81d15u, 0xd7a5809bu, 0xbf62fcc2u, 0x7dc55cf8u, 0x2e9e5ee6u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 113 /* milli weight units */
}
,[LEFT_PAD_LOW_16_32] =
{ .tag = JET
, .jet = simplicity_left_pad_low_16_32
, .cmr = {{0x4ffd6cb3u, 0x40230582u, 0x1dd89970u, 0xd722d1c1u, 0x3f1ff773u, 0x9fd5f34bu, 0xa16c7365u, 0x3b044718u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 65 /* milli weight units */
}
,[LEFT_PAD_LOW_16_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_16_64
, .cmr = {{0xbe3eb85cu, 0x5f199153u, 0xfb1c4613u, 0x5c04facfu, 0xdbc6f1b7u, 0x8c2bb7aeu, 0x75f155bcu, 0x3ea08a8bu}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 68 /* milli weight units */
}
,[LEFT_PAD_LOW_1_16] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_16
, .cmr = {{0xddd0153eu, 0xf312f28du, 0x642cd94cu, 0xb36f3297u, 0x75b00da8u, 0x8fccc4ceu, 0xa1bae89bu, 0xad13be6bu}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 59 /* milli weight units */
}
,[LEFT_PAD_LOW_1_32] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_32
, .cmr = {{0xbc9d3114u, 0x35467bc0u, 0x8b1008e5u, 0x47aa7a07u, 0xe83b1514u, 0x6861a9e9u, 0xb5413be3u, 0x1b82b6b5u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 47 /* milli weight units */
}
,[LEFT_PAD_LOW_1_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_64
, .cmr = {{0x8bc62f93u, 0x60894e48u, 0xa4732c95u, 0x769c8faau, 0xe9568f9du, 0xe8e8a200u, 0x836bd4e5u, 0x0b02cd84u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 46 /* milli weight units */
}
,[LEFT_PAD_LOW_1_8] =
{ .tag = JET
, .jet = simplicity_left_pad_low_1_8
, .cmr = {{0xf66cd7a4u, 0x2b320f97u, 0xc19f2d54u, 0x16cde087u, 0x253a2791u, 0x2965d55bu, 0x65712ad8u, 0x09b83cfdu}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 48 /* milli weight units */
}
,[LEFT_PAD_LOW_32_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_32_64
, .cmr = {{0xa33a07b9u, 0xbcf945f6u, 0x4f072b8bu, 0x9c914839u, 0xa585bfa9u, 0xf3425b14u, 0x7754ab55u, 0xa8ba6c0fu}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 62 /* milli weight units */
}
,[LEFT_PAD_LOW_8_16] =
{ .tag = JET
, .jet = simplicity_left_pad_low_8_16
, .cmr = {{0x2a516a79u, 0x3f97c45fu, 0xeaebb1ccu, 0x961a156du, 0x80354928u, 0x79789d6eu, 0xdc9b57e7u, 0x2f11e5b5u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 56 /* milli weight units */
}
,[LEFT_PAD_LOW_8_32] =
{ .tag = JET
, .jet = simplicity_left_pad_low_8_32
, .cmr = {{0x1aa2e4d0u, 0x4bd69055u, 0x123dd6aau, 0xfe27f5f7u, 0xf47c3b30u, 0x90c3a827u, 0x2973fe2fu, 0x75165a5du}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 75 /* milli weight units */
}
,[LEFT_PAD_LOW_8_64] =
{ .tag = JET
, .jet = simplicity_left_pad_low_8_64
, .cmr = {{0xb652e0aeu, 0xdd0f4f66u, 0xf6a1cd4bu, 0xebf875ffu, 0x7bbb2dd9u, 0x9b065b2du, 0xb5b5b590u, 0x5361614du}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 116 /* milli weight units */
}
,[LEFT_ROTATE_16] =
{ .tag = JET
, .jet = simplicity_left_rotate_16
, .cmr = {{0x8a12ff6au, 0x4bf23715u, 0xdd3b766bu, 0x9967c715u, 0x8bf3ed74u, 0xb3dce730u, 0xaffcf466u, 0x16478ecbu}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 88 /* milli weight units */
}
,[LEFT_ROTATE_32] =
{ .tag = JET
, .jet = simplicity_left_rotate_32
, .cmr = {{0x2fcb5217u, 0x2fd49c36u, 0x217deae0u, 0xc2371432u, 0x1f69f5f1u, 0x3f6e94b2u, 0xbdfe4b74u, 0x88697fd5u}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 62 /* milli weight units */
}
,[LEFT_ROTATE_64] =
{ .tag = JET
, .jet = simplicity_left_rotate_64
, .cmr = {{0x72ccd6c4u, 0xe5fdf68au, 0xd33b6d58u, 0xfb372be4u, 0xf1b80eefu, 0x701f9db7u, 0xe5ed859bu, 0x96b36209u}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 68 /* milli weight units */
}
,[LEFT_ROTATE_8] =
{ .tag = JET
, .jet = simplicity_left_rotate_8
, .cmr = {{0x1aaec9f3u, 0xb75d89f8u, 0x2a649845u, 0x8c4483cbu, 0x9a784489u, 0x05f3bb39u, 0xfc083f14u, 0xddccdc9bu}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 66 /* milli weight units */
}
,[LEFT_SHIFT_16] =
{ .tag = JET
, .jet = simplicity_left_shift_16
, .cmr = {{0x37ac6387u, 0x21ab097au, 0x9602ba4du, 0xc92e19b5u, 0xa185b232u, 0x9f1aa600u, 0xcb9c1561u, 0x5a0081f8u}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 109 /* milli weight units */
}
,[LEFT_SHIFT_32] =
{ .tag = JET
, .jet = simplicity_left_shift_32
, .cmr = {{0x8e3c473bu, 0x2867f154u, 0x73b3632du, 0xbfdd9977u, 0x5551ef5fu, 0x9dba475eu, 0x9cf09075u, 0x8070f0bfu}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 79 /* milli weight units */
}
,[LEFT_SHIFT_64] =
{ .tag = JET
, .jet = simplicity_left_shift_64
, .cmr = {{0x5049f404u, 0xd173299au, 0x3aee04cbu, 0xc2462cb3u, 0x4c8069c1u, 0xb6db7fedu, 0x0e388ff6u, 0xd467a086u}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 70 /* milli weight units */
}
,[LEFT_SHIFT_8] =
{ .tag = JET
, .jet = simplicity_left_shift_8
, .cmr = {{0x832f636eu, 0x63446cefu, 0xba8df3a4u, 0x6efbb361u, 0x59c18854u, 0x567768adu, 0xc9b8db8au, 0x07492a58u}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 72 /* milli weight units */
}
,[LEFT_SHIFT_WITH_16] =
{ .tag = JET
, .jet = simplicity_left_shift_with_16
, .cmr = {{0xe64762b1u, 0xc5e6144au, 0x7181eaafu, 0x4dd9d9b3u, 0xaa43aad9u, 0x55158198u, 0xee2090ebu, 0xd9e4bb0du}}
, .sourceIx = ty_pbpw4w16
, .targetIx = ty_w16
, .cost = 72 /* milli weight units */
}
,[LEFT_SHIFT_WITH_32] =
{ .tag = JET
, .jet = simplicity_left_shift_with_32
, .cmr = {{0x6476ba89u, 0x95f83b5eu, 0xe1ebc22cu, 0xb416f558u, 0x157f2e57u, 0x699a5cafu, 0x84291ff3u, 0xfc1483c1u}}
, .sourceIx = ty_pbpw8w32
, .targetIx = ty_w32
, .cost = 87 /* milli weight units */
}
,[LEFT_SHIFT_WITH_64] =
{ .tag = JET
, .jet = simplicity_left_shift_with_64
, .cmr = {{0x06b8fe67u, 0xcfc58632u, 0x2397af02u, 0x4fde2911u, 0xf7ae87a0u, 0x6abc6c59u, 0x30934097u, 0x15691c19u}}
, .sourceIx = ty_pbpw8w64
, .targetIx = ty_w64
, .cost = 97 /* milli weight units */
}
,[LEFT_SHIFT_WITH_8] =
{ .tag = JET
, .jet = simplicity_left_shift_with_8
, .cmr = {{0xb1ac9c68u, 0x2358c45bu, 0xabf40695u, 0x56fe6e37u, 0x5b4554deu, 0x9e10c591u, 0xc1483984u, 0x47ac180eu}}
, .sourceIx = ty_pbpw4w8
, .targetIx = ty_w8
, .cost = 104 /* milli weight units */
}
,[LEFTMOST_16_1] =
{ .tag = JET
, .jet = simplicity_leftmost_16_1
, .cmr = {{0x5bff4cb5u, 0x587605d5u, 0xfd059d77u, 0x33490d7du, 0xd22d278bu, 0x599e06d3u, 0xb5db6d79u, 0xf3c923bdu}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 68 /* milli weight units */
}
,[LEFTMOST_16_2] =
{ .tag = JET
, .jet = simplicity_leftmost_16_2
, .cmr = {{0x536db486u, 0xb12227e5u, 0xb09d6febu, 0xd2776b1au, 0xbbc67499u, 0x96aa783eu, 0xd7e53744u, 0x6bbf151bu}}
, .sourceIx = ty_w16
, .targetIx = ty_w2
, .cost = 58 /* milli weight units */
}
,[LEFTMOST_16_4] =
{ .tag = JET
, .jet = simplicity_leftmost_16_4
, .cmr = {{0xf2321367u, 0x496d1a77u, 0xeea05e95u, 0xe3b807d3u, 0xba5f0513u, 0x6ce0912au, 0xe717c83au, 0x0261b2e1u}}
, .sourceIx = ty_w16
, .targetIx = ty_w4
, .cost = 51 /* milli weight units */
}
,[LEFTMOST_16_8] =
{ .tag = JET
, .jet = simplicity_leftmost_16_8
, .cmr = {{0x24148ef3u, 0x0ad43ebeu, 0xc5637283u, 0x22c3ce11u, 0x79aed7a7u, 0x8216d799u, 0x888bf18bu, 0x39570671u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 62 /* milli weight units */
}
,[LEFTMOST_32_1] =
{ .tag = JET
, .jet = simplicity_leftmost_32_1
, .cmr = {{0xb92e15ecu, 0x5da07ee8u, 0xed397cb9u, 0xf60a4c5du, 0xa8386293u, 0x1a907359u, 0xd27caeb6u, 0x0e60ef8au}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 53 /* milli weight units */
}
,[LEFTMOST_32_16] =
{ .tag = JET
, .jet = simplicity_leftmost_32_16
, .cmr = {{0xadb027b2u, 0x06567358u, 0x5326c01cu, 0x3be2faebu, 0x386349e2u, 0x9009b657u, 0x6ee53a85u, 0x5512cc67u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 63 /* milli weight units */
}
,[LEFTMOST_32_2] =
{ .tag = JET
, .jet = simplicity_leftmost_32_2
, .cmr = {{0xb75b31c5u, 0x59123d3du, 0x63359859u, 0x32b8b1b2u, 0x664ee597u, 0xafb15fd1u, 0xa499d007u, 0xcff2755cu}}
, .sourceIx = ty_w32
, .targetIx = ty_w2
, .cost = 62 /* milli weight units */
}
,[LEFTMOST_32_4] =
{ .tag = JET
, .jet = simplicity_leftmost_32_4
, .cmr = {{0xcb757e47u, 0x1e9d9a40u, 0x771dd1cfu, 0x3c1bf5d2u, 0x3c17ed68u, 0xcdbdb22du, 0xada17a73u, 0xa7b407b2u}}
, .sourceIx = ty_w32
, .targetIx = ty_w4
, .cost = 61 /* milli weight units */
}
,[LEFTMOST_32_8] =
{ .tag = JET
, .jet = simplicity_leftmost_32_8
, .cmr = {{0xbfc534b4u, 0x9e06006eu, 0x19f3b68eu, 0x0a02391cu, 0x149f9a34u, 0xf43ee36bu, 0x9f1d79a7u, 0x9c9a9e4du}}
, .sourceIx = ty_w32
, .targetIx = ty_w8
, .cost = 60 /* milli weight units */
}
,[LEFTMOST_64_1] =
{ .tag = JET
, .jet = simplicity_leftmost_64_1
, .cmr = {{0x1b1d4e92u, 0x384b8b15u, 0x9ba0d806u, 0x558b5494u, 0xe3614eedu, 0xe03c946cu, 0xeaf141f3u, 0x6f01c79bu}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 65 /* milli weight units */
}
,[LEFTMOST_64_16] =
{ .tag = JET
, .jet = simplicity_leftmost_64_16
, .cmr = {{0x0debdc1au, 0xa0433034u, 0x42e18fe0u, 0x3d8a99d2u, 0xbe6bb8a8u, 0x691aba19u, 0x566259e3u, 0x6760f7f9u}}
, .sourceIx = ty_w64
, .targetIx = ty_w16
, .cost = 62 /* milli weight units */
}
,[LEFTMOST_64_2] =
{ .tag = JET
, .jet = simplicity_leftmost_64_2
, .cmr = {{0x839ecfa3u, 0x18705c25u, 0x3d0c52ffu, 0x27b90464u, 0x923d8c0eu, 0x55a82c0du, 0x16240239u, 0x7f365378u}}
, .sourceIx = ty_w64
, .targetIx = ty_w2
, .cost = 61 /* milli weight units */
}
,[LEFTMOST_64_32] =
{ .tag = JET
, .jet = simplicity_leftmost_64_32
, .cmr = {{0x929197a9u, 0x642861a7u, 0x7bd66258u, 0x051197beu, 0x86ff08e6u, 0x28e30f7eu, 0xfcbd2c4du, 0xfecf9bddu}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 77 /* milli weight units */
}
,[LEFTMOST_64_4] =
{ .tag = JET
, .jet = simplicity_leftmost_64_4
, .cmr = {{0x02bd1645u, 0xd575f04bu, 0x3cbbaa6du, 0x8ca986efu, 0x1c8cd0ffu, 0xe1658903u, 0x939db764u, 0x562a2647u}}
, .sourceIx = ty_w64
, .targetIx = ty_w4
, .cost = 80 /* milli weight units */
}
,[LEFTMOST_64_8] =
{ .tag = JET
, .jet = simplicity_leftmost_64_8
, .cmr = {{0x3558b31bu, 0x3b6e8f9au, 0x288fdc72u, 0xf24602beu, 0x05581910u, 0x71a54a99u, 0xfa03a025u, 0x34f88005u}}
, .sourceIx = ty_w64
, .targetIx = ty_w8
, .cost = 54 /* milli weight units */
}
,[LEFTMOST_8_1] =
{ .tag = JET
, .jet = simplicity_leftmost_8_1
, .cmr = {{0x2865efd4u, 0x2983cbe3u, 0xf816373au, 0xb8a882f1u, 0x8317194du, 0xc1aba38du, 0xa0304b8cu, 0x144b1da4u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 54 /* milli weight units */
}
,[LEFTMOST_8_2] =
{ .tag = JET
, .jet = simplicity_leftmost_8_2
, .cmr = {{0x51964cb0u, 0x7405a8d2u, 0x3d218774u, 0x1a9ed304u, 0xbcb469d9u, 0xac9f5d92u, 0x55825cfdu, 0xa3da07c0u}}
, .sourceIx = ty_w8
, .targetIx = ty_w2
, .cost = 71 /* milli weight units */
}
,[LEFTMOST_8_4] =
{ .tag = JET
, .jet = simplicity_leftmost_8_4
, .cmr = {{0x883c94f8u, 0xa26cdab7u, 0xbc5cd631u, 0xe52255a8u, 0x5ef6e070u, 0x766457f6u, 0x321e2ccbu, 0x119d9b2bu}}
, .sourceIx = ty_w8
, .targetIx = ty_w4
, .cost = 65 /* milli weight units */
}
,[LINEAR_COMBINATION_1] =
{ .tag = JET
, .jet = simplicity_linear_combination_1
, .cmr = {{0x3410a9eeu, 0x333df8c8u, 0xa01c1411u, 0x5b544327u, 0xe324e287u, 0xaa1107e0u, 0x1955bd20u, 0x506ea987u}}
, .sourceIx = ty_ppw256pw512w256w256
, .targetIx = ty_pw512w256
, .cost = 85743 /* milli weight units */
}
,[LINEAR_VERIFY_1] =
{ .tag = JET
, .jet = simplicity_linear_verify_1
, .cmr = {{0xdc66d331u, 0xc17f3fddu, 0xa3994698u, 0x1b39b357u, 0xd0555c35u, 0x62ecae02u, 0xaa2dad16u, 0x3e6c9a2eu}}
, .sourceIx = ty_pppw256w512w256w512
, .targetIx = ty_u
, .cost = 43579 /* milli weight units */
}
,[LOCK_TIME] =
{ .tag = JET
, .jet = simplicity_bitcoin_lock_time
, .cmr = {{0x9ae0acc3u, 0x7bc20447u, 0x79b07c3du, 0x4602a5fdu, 0xe8bc33f8u, 0x79f66b73u, 0x9b10f01au, 0xeb1154ecu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 66 /* milli weight units */
}
,[LOW_1] =
{ .tag = JET
, .jet = simplicity_low_1
, .cmr = {{0xfe6214f9u, 0x67156dcdu, 0xe6dd49fdu, 0xc55efb86u, 0x5069feabu, 0xfff0fe93u, 0x1dba8531u, 0x34eed130u}}
, .sourceIx = ty_u
, .targetIx = ty_b
, .cost = 40 /* milli weight units */
}
,[LOW_16] =
{ .tag = JET
, .jet = simplicity_low_16
, .cmr = {{0x7493cf69u, 0x8a4882e5u, 0xc3579d06u, 0x518e7ecau, 0x2b8428f6u, 0x2e2b5138u, 0x02abe622u, 0x170c20feu}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 60 /* milli weight units */
}
,[LOW_32] =
{ .tag = JET
, .jet = simplicity_low_32
, .cmr = {{0x362d66a4u, 0xf0aeb965u, 0x84a56757u, 0x8271b1f7u, 0xbbfcc2deu, 0x0dcf9579u, 0x6b6f7a82u, 0x6b2a8af7u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 52 /* milli weight units */
}
,[LOW_64] =
{ .tag = JET
, .jet = simplicity_low_64
, .cmr = {{0x973323bcu, 0x2b92e428u, 0x04d2e4f5u, 0x8b86f65bu, 0x56f91deeu, 0xb4810eabu, 0x8a1deda9u, 0x697a0872u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 50 /* milli weight units */
}
,[LOW_8] =
{ .tag = JET
, .jet = simplicity_low_8
, .cmr = {{0xcd1a8558u, 0xef99a322u, 0x60217a76u, 0x49ff5140u, 0xda69da70u, 0x0672690bu, 0x27917b07u, 0xd7c14c67u}}
, .sourceIx = ty_u
, .targetIx = ty_w8
, .cost = 45 /* milli weight units */
}
,[LT_16] =
{ .tag = JET
, .jet = simplicity_lt_16
, .cmr = {{0x04aca87eu, 0x3e17f805u, 0xa21cf291u, 0x7aee9957u, 0xb950b2dbu, 0x5d7ae5c8u, 0x26d4ac2eu, 0xc97b5a52u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 83 /* milli weight units */
}
,[LT_32] =
{ .tag = JET
, .jet = simplicity_lt_32
, .cmr = {{0x23a0a5c1u, 0x97747e3au, 0x9579e90eu, 0x0f22f84au, 0x29bfb5f0u, 0x7b84b59bu, 0x26688a0cu, 0xd59dfebdu}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 89 /* milli weight units */
}
,[LT_64] =
{ .tag = JET
, .jet = simplicity_lt_64
, .cmr = {{0xd299901cu, 0x7b5b3a59u, 0xffc8dd09u, 0x545a3238u, 0x24b779a9u, 0x9b2d1a2fu, 0x87452d9eu, 0x4befaf30u}}
, .sourceIx = ty_w128
, .targetIx = ty_b
, .cost = 71 /* milli weight units */
}
,[LT_8] =
{ .tag = JET
, .jet = simplicity_lt_8
, .cmr = {{0xdd94413bu, 0x529c298cu, 0x1696e9fbu, 0x08e66767u, 0xb3f8337au, 0xc02e44b0u, 0x68e94014u, 0xf7c41f2au}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 86 /* milli weight units */
}
,[MAJ_1] =
{ .tag = JET
, .jet = simplicity_maj_1
, .cmr = {{0x0e6fb40fu, 0xe31a3a52u, 0x6b44cf0bu, 0x7c7936c7u, 0x77cbba89u, 0x65a72552u, 0x32a7cf53u, 0xa922885au}}
, .sourceIx = ty_pbw2
, .targetIx = ty_b
, .cost = 54 /* milli weight units */
}
,[MAJ_16] =
{ .tag = JET
, .jet = simplicity_maj_16
, .cmr = {{0x38669ce5u, 0xe1e17147u, 0x5400731bu, 0xeeb60bcau, 0xfad66604u, 0xc9394016u, 0x0cd71288u, 0x35559342u}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 85 /* milli weight units */
}
,[MAJ_32] =
{ .tag = JET
, .jet = simplicity_maj_32
, .cmr = {{0x5554349bu, 0x584f5c38u, 0x72c7f4f2u, 0x57829e2au, 0xe822d823u, 0x424ceb95u, 0x98f08318u, 0x586a8807u}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 73 /* milli weight units */
}
,[MAJ_64] =
{ .tag = JET
, .jet = simplicity_maj_64
, .cmr = {{0x734903bau, 0xefb71d5eu, 0xa41648ffu, 0x43eee698u, 0x94e063b3u, 0x88ea422fu, 0x96aede19u, 0x3ceab839u}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 79 /* milli weight units */
}
,[MAJ_8] =
{ .tag = JET
, .jet = simplicity_maj_8
, .cmr = {{0xba47a399u, 0xdc9435e1u, 0x8e080a4eu, 0x18af7c65u, 0x7fd39f7cu, 0xe7d6052eu, 0x46902311u, 0xb078d585u}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 64 /* milli weight units */
}
,[MAX_16] =
{ .tag = JET
, .jet = simplicity_max_16
, .cmr = {{0xaa552374u, 0x6cabfaf5u, 0x668e9e07u, 0x37e56b06u, 0x062251d7u, 0xe80ab9b9u, 0x106d8f17u, 0x2dc84dd6u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 80 /* milli weight units */
}
,[MAX_32] =
{ .tag = JET
, .jet = simplicity_max_32
, .cmr = {{0x6922965du, 0x144345c9u, 0x13ecb30bu, 0x5ed47e88u, 0xdae35c12u, 0x21f26aa9u, 0x2dd5a5f6u, 0x15dbdb53u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 70 /* milli weight units */
}
,[MAX_64] =
{ .tag = JET
, .jet = simplicity_max_64
, .cmr = {{0x8a9be907u, 0xb6a4c30au, 0xbcc0f22du, 0x013074c2u, 0xd56bb081u, 0xf2621857u, 0xd538cc97u, 0x131e4409u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 75 /* milli weight units */
}
,[MAX_8] =
{ .tag = JET
, .jet = simplicity_max_8
, .cmr = {{0xb4bf9323u, 0x4022e860u, 0xfe76c0b5u, 0x360e8b36u, 0xff81ee67u, 0x05b593acu, 0xdf655ac6u, 0xe6d7aebau}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 79 /* milli weight units */
}
,[MEDIAN_16] =
{ .tag = JET
, .jet = simplicity_median_16
, .cmr = {{0x17e2e87fu, 0x0760f4fbu, 0x3c9fd0beu, 0xd000d739u, 0x73ab60f5u, 0xe6c2c1fau, 0xb17f9b23u, 0xee6aca48u}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 80 /* milli weight units */
}
,[MEDIAN_32] =
{ .tag = JET
, .jet = simplicity_median_32
, .cmr = {{0x1160ae8eu, 0xa8d30f9au, 0x2233c48eu, 0x731240f8u, 0x4493b828u, 0xb55793e2u, 0xf4042a19u, 0x82ac26a5u}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 77 /* milli weight units */
}
,[MEDIAN_64] =
{ .tag = JET
, .jet = simplicity_median_64
, .cmr = {{0xc8737364u, 0x9e7e4050u, 0xbb73337eu, 0x08eb5de4u, 0x5228ab86u, 0xad4e1f41u, 0x91e5202au, 0xa6afa0c5u}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 89 /* milli weight units */
}
,[MEDIAN_8] =
{ .tag = JET
, .jet = simplicity_median_8
, .cmr = {{0xc3b4e089u, 0x8a21bde9u, 0x4daed37au, 0x20adf90cu, 0x8be5691au, 0x03b6a1e5u, 0x56385d42u, 0xeb19022bu}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 77 /* milli weight units */
}
,[MIN_16] =
{ .tag = JET
, .jet = simplicity_min_16
, .cmr = {{0x5fd0051eu, 0xdb3719a6u, 0x45b272a0u, 0x2108efbbu, 0x3d9bc0f6u, 0x0621bf5au, 0x5babe116u, 0xd555d578u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 83 /* milli weight units */
}
,[MIN_32] =
{ .tag = JET
, .jet = simplicity_min_32
, .cmr = {{0xd80782a2u, 0xb5d86ab6u, 0xb9c9c3fbu, 0x778a3473u, 0xf600b185u, 0xfe1925eeu, 0x9fc2e877u, 0x7ed26601u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 96 /* milli weight units */
}
,[MIN_64] =
{ .tag = JET
, .jet = simplicity_min_64
, .cmr = {{0xc5c09d50u, 0x1338e9a5u, 0x12cf8976u, 0xca4b32b9u, 0x2480bef6u, 0xaeb29d36u, 0xd590d35bu, 0xf9f9ece1u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 82 /* milli weight units */
}
,[MIN_8] =
{ .tag = JET
, .jet = simplicity_min_8
, .cmr = {{0x81d21e12u, 0x81423881u, 0x802c0e0cu, 0x7d22bd34u, 0xd26bd12au, 0x4c4f1b70u, 0x68e7e183u, 0x820848e9u}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 78 /* milli weight units */
}
,[MODULO_16] =
{ .tag = JET
, .jet = simplicity_modulo_16
, .cmr = {{0xb6b87cfau, 0xb67e5519u, 0xf1c998dau, 0x479437bbu, 0x79e674f7u, 0x15e9a2e5u, 0x38eec5ecu, 0x18e18ea5u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 85 /* milli weight units */
}
,[MODULO_32] =
{ .tag = JET
, .jet = simplicity_modulo_32
, .cmr = {{0x8d486e83u, 0x1654f38au, 0x32da35ebu, 0x7bb655a6u, 0xed694dbfu, 0xa058957du, 0x9f5cbfccu, 0x5792c65bu}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 81 /* milli weight units */
}
,[MODULO_64] =
{ .tag = JET
, .jet = simplicity_modulo_64
, .cmr = {{0x14df20d9u, 0x3dfdefe2u, 0x559bac50u, 0xed38193bu, 0xd78bd63fu, 0x929d86fbu, 0x4f29a7c5u, 0xaf3242adu}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 71 /* milli weight units */
}
,[MODULO_8] =
{ .tag = JET
, .jet = simplicity_modulo_8
, .cmr = {{0x2c758a7cu, 0x0f59e800u, 0xe94f3dc5u, 0xa001bf8eu, 0xd9435f75u, 0xa2d96930u, 0xc57eaab0u, 0xcd80af5cu}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 85 /* milli weight units */
}
,[MULTIPLY_16] =
{ .tag = JET
, .jet = simplicity_multiply_16
, .cmr = {{0x75bd41f2u, 0xd2b339f0u, 0x69bfdfd8u, 0x02d61e6cu, 0xa8e3bad6u, 0xfb6d95b6u, 0x72095b93u, 0x345f047fu}}
, .sourceIx = ty_w32
, .targetIx = ty_w32
, .cost = 79 /* milli weight units */
}
,[MULTIPLY_32] =
{ .tag = JET
, .jet = simplicity_multiply_32
, .cmr = {{0x84cbe6ceu, 0x87037992u, 0x13877c1bu, 0xd505c764u, 0x34336900u, 0x2e502c43u, 0xd97f3d57u, 0x772d6c87u}}
, .sourceIx = ty_w64
, .targetIx = ty_w64
, .cost = 78 /* milli weight units */
}
,[MULTIPLY_64] =
{ .tag = JET
, .jet = simplicity_multiply_64
, .cmr = {{0x92987b80u, 0x1b92f679u, 0xeb961368u, 0x8444a178u, 0x8750a850u, 0x6e03a921u, 0x8c21ecc7u, 0x2082dc6au}}
, .sourceIx = ty_w128
, .targetIx = ty_w128
, .cost = 72 /* milli weight units */
}
,[MULTIPLY_8] =
{ .tag = JET
, .jet = simplicity_multiply_8
, .cmr = {{0x764cab71u, 0xdb9459a7u, 0x696d944au, 0x50095b1au, 0xebdfd928u, 0x4bdb7496u, 0xa7b30241u, 0xccba3eceu}}
, .sourceIx = ty_w16
, .targetIx = ty_w16
, .cost = 79 /* milli weight units */
}
,[NEGATE_16] =
{ .tag = JET
, .jet = simplicity_negate_16
, .cmr = {{0xe760ee40u, 0x29c34f89u, 0x7406ffdeu, 0xa5558486u, 0x62e89c98u, 0x3e6070bdu, 0x0272ad0fu, 0xa342efa3u}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw16
, .cost = 69 /* milli weight units */
}
,[NEGATE_32] =
{ .tag = JET
, .jet = simplicity_negate_32
, .cmr = {{0x8495b740u, 0x09ad07c9u, 0x302a25aeu, 0x56c3e973u, 0x3f00c2bau, 0xa410eac4u, 0xa58e75dbu, 0x83af1d22u}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw32
, .cost = 56 /* milli weight units */
}
,[NEGATE_64] =
{ .tag = JET
, .jet = simplicity_negate_64
, .cmr = {{0x34e89fafu, 0x345afd5eu, 0x7b290014u, 0x52fc5fc2u, 0xe3783af7u, 0xf2101643u, 0xbd76706au, 0x6fc3f36au}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw64
, .cost = 56 /* milli weight units */
}
,[NEGATE_8] =
{ .tag = JET
, .jet = simplicity_negate_8
, .cmr = {{0xe81be0b1u, 0x5c671ab8u, 0xdf1f4869u, 0xc57f1111u, 0x18cb6683u, 0x54975c63u, 0x66ecb2b8u, 0xbb7c15cfu}}
, .sourceIx = ty_w8
, .targetIx = ty_pbw8
, .cost = 69 /* milli weight units */
}
,[NUM_INPUTS] =
{ .tag = JET
, .jet = simplicity_bitcoin_num_inputs
, .cmr = {{0x5c5ac4ffu, 0x6da56cb3u, 0x72b23266u, 0x6e8334b9u, 0xe2cfb0dcu, 0xb418f161u, 0xbff149e8u, 0x4ec92c3eu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 74 /* milli weight units */
}
,[NUM_OUTPUTS] =
{ .tag = JET
, .jet = simplicity_bitcoin_num_outputs
, .cmr = {{0x98a1cca7u, 0x05dfcfafu, 0xd3a69e9au, 0xdc05ba47u, 0xe1fefa6au, 0x29f34286u, 0x2048e496u, 0x8648c3d7u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 68 /* milli weight units */
}
,[ONE_16] =
{ .tag = JET
, .jet = simplicity_one_16
, .cmr = {{0x2e5e3d95u, 0xe4531688u, 0x8e4f3709u, 0xef832b9fu, 0xd9e15f30u, 0x719bf55fu, 0xc2e0e09au, 0x3657d882u}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 45 /* milli weight units */
}
,[ONE_32] =
{ .tag = JET
, .jet = simplicity_one_32
, .cmr = {{0x06426b85u, 0x3c1bcb33u, 0x8aedbe1fu, 0x89a6d9b7u, 0xa3da038cu, 0xd00a4471u, 0x18369349u, 0x669e2976u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 45 /* milli weight units */
}
,[ONE_64] =
{ .tag = JET
, .jet = simplicity_one_64
, .cmr = {{0xab1d2cd9u, 0x9678da3cu, 0x128d39adu, 0x9fe6ffa9u, 0x55c16e5eu, 0xf2c25bb4u, 0x31831559u, 0x6951f427u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 45 /* milli weight units */
}
,[ONE_8] =
{ .tag = JET
, .jet = simplicity_one_8
, .cmr = {{0x3cc5f523u, 0xd6a6355du, 0xc924ee0au, 0xc1f5fe2cu, 0x521275e3u, 0xaa9f21d3u, 0x1b082db2u, 0xac230d9du}}
, .sourceIx = ty_u
, .targetIx = ty_w8
, .cost = 46 /* milli weight units */
}
,[OR_1] =
{ .tag = JET
, .jet = simplicity_or_1
, .cmr = {{0xc4659643u, 0x69fca209u, 0x7f83530cu, 0x87bcbc90u, 0xc306579du, 0x9f3bfeddu, 0xf4a172a4u, 0xea0b58ecu}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 56 /* milli weight units */
}
,[OR_16] =
{ .tag = JET
, .jet = simplicity_or_16
, .cmr = {{0x5a985e04u, 0x3b85273bu, 0x90f90e20u, 0xf82b7532u, 0x3351cf2au, 0x4e62a7f9u, 0xcb2f0596u, 0x402e9e28u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 78 /* milli weight units */
}
,[OR_32] =
{ .tag = JET
, .jet = simplicity_or_32
, .cmr = {{0x3552383au, 0x57ffb48du, 0x63a0337au, 0xf0dd6efau, 0xb6b46c5du, 0xe1720e42u, 0x0bdd1c82u, 0x276bc9a9u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 80 /* milli weight units */
}
,[OR_64] =
{ .tag = JET
, .jet = simplicity_or_64
, .cmr = {{0x51a173dau, 0xdca01ac6u, 0xf62e75d5u, 0xcd3522f0u, 0x9fde62b1u, 0x1513e068u, 0x422852a4u, 0x9167b606u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 71 /* milli weight units */
}
,[OR_8] =
{ .tag = JET
, .jet = simplicity_or_8
, .cmr = {{0x79efbdcbu, 0x537bebcbu, 0x188d1116u, 0xb78a109bu, 0xffbc2a6cu, 0xe3d1f870u, 0x154a7956u, 0x091b342fu}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 81 /* milli weight units */
}
,[OUTPOINT_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_outpoint_hash
, .cmr = {{0x3a1ae90eu, 0x167fb40du, 0x6e13b451u, 0xad67410du, 0x8dd991c8u, 0x7d6a4a59u, 0xcc76c63fu, 0x3b9e5e56u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256pw256w32
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1788 /* milli weight units */
}
,[OUTPUT_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_output_hash
, .cmr = {{0x91211fc6u, 0x011a6493u, 0x00c6bee9u, 0x4fdd48a9u, 0x7fa2a9b6u, 0xf284be01u, 0x5d462d17u, 0xde664ac3u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 822 /* milli weight units */
}
,[OUTPUT_SCRIPT_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_output_script_hash
, .cmr = {{0xbdfdb231u, 0xf4f1a62cu, 0x9d7b0393u, 0x1e7f19a4u, 0x546af234u, 0x754cbf70u, 0x059fdd42u, 0xbbbc4126u}}
, .sourceIx = ty_w32
, .targetIx = ty_mw256
, .cost = 135 /* milli weight units */
}
,[OUTPUT_SCRIPTS_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_output_scripts_hash
, .cmr = {{0xff20bc43u, 0x65e71707u, 0x571c6e17u, 0x38e1ed32u, 0x6f7c351du, 0xe13022aeu, 0xa3d6406bu, 0x8aee8e3bu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 123 /* milli weight units */
}
,[OUTPUT_VALUE] =
{ .tag = JET
, .jet = simplicity_bitcoin_output_value
, .cmr = {{0x933643b6u, 0xc5a6220au, 0xbbca6f35u, 0x09feff6du, 0x13efa6c9u, 0xfae95924u, 0x575364f2u, 0xb164d2bcu}}
, .sourceIx = ty_w32
, .targetIx = ty_mw64
, .cost = 82 /* milli weight units */
}
,[OUTPUT_VALUES_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_output_values_hash
, .cmr = {{0x22899379u, 0x0057066fu, 0x2016971du, 0xf55e6f67u, 0xd252efb6u, 0xdaabd0fcu, 0x566a8d21u, 0x56efbbfcu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 119 /* milli weight units */
}
,[OUTPUTS_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_outputs_hash
, .cmr = {{0xf2eb6d0fu, 0x018e6f15u, 0xe35baa82u, 0xe57e14feu, 0x343796f2u, 0x196826beu, 0xd7c78755u, 0x98d6641du}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 117 /* milli weight units */
}
,[PARSE_LOCK] =
{ .tag = JET
, .jet = simplicity_parse_lock
, .cmr = {{0x3db84535u, 0xfa3d90efu, 0x0b581e22u, 0xb61d2127u, 0x844b2116u, 0xe84f814au, 0x5cbac52du, 0xf515f2d2u}}
, .sourceIx = ty_w32
, .targetIx = ty_sw32w32
, .cost = 82 /* milli weight units */
}
,[PARSE_SEQUENCE] =
{ .tag = JET
, .jet = simplicity_parse_sequence
, .cmr = {{0x38b2533fu, 0x5fede869u, 0xbaa17069u, 0x83df4c89u, 0xd62d5f90u, 0x800b47eau, 0xb2111331u, 0x1a5aaec9u}}
, .sourceIx = ty_w32
, .targetIx = ty_msw16w16
, .cost = 93 /* milli weight units */
}
,[POINT_VERIFY_1] =
{ .tag = JET
, .jet = simplicity_point_verify_1
, .cmr = {{0xbe2a9890u, 0xf1d5b615u, 0x147f8241u, 0xe0609b5cu, 0xac01ece0u, 0xa3f92368u, 0x67b2bfdeu, 0xa1b8044eu}}
, .sourceIx = ty_pppw256pbw256w256pbw256
, .targetIx = ty_u
, .cost = 41394 /* milli weight units */
}
,[RIGHT_EXTEND_16_32] =
{ .tag = JET
, .jet = simplicity_right_extend_16_32
, .cmr = {{0xdbf18d87u, 0xa7892139u, 0xa388e9a9u, 0x83c48992u, 0xac35a845u, 0x56ee0defu, 0xc1dadf0cu, 0x5f471a26u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 73 /* milli weight units */
}
,[RIGHT_EXTEND_16_64] =
{ .tag = JET
, .jet = simplicity_right_extend_16_64
, .cmr = {{0xd011acc7u, 0x94e3c478u, 0x9accd0d5u, 0xfe4997d3u, 0x34d91f08u, 0x31a1eb35u, 0x04b4cb2du, 0xdf4797afu}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 70 /* milli weight units */
}
,[RIGHT_EXTEND_32_64] =
{ .tag = JET
, .jet = simplicity_right_extend_32_64
, .cmr = {{0xa5aa5db1u, 0xe535e723u, 0x2ad36dafu, 0xba6d5a20u, 0x0d54eb85u, 0x3b75dc70u, 0xa594ed64u, 0xaa6bd9abu}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 62 /* milli weight units */
}
,[RIGHT_EXTEND_8_16] =
{ .tag = JET
, .jet = simplicity_right_extend_8_16
, .cmr = {{0x8106d58au, 0x8066ee6eu, 0x15e55ca5u, 0x2cb7afd8u, 0xe3277587u, 0xbfd7dec0u, 0xbe37d406u, 0x742a3931u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 63 /* milli weight units */
}
,[RIGHT_EXTEND_8_32] =
{ .tag = JET
, .jet = simplicity_right_extend_8_32
, .cmr = {{0xdfa4bafau, 0x432a5338u, 0xd374deb6u, 0xb724b7f6u, 0xeae55861u, 0xfe731d43u, 0x048aa304u, 0xd1f7f9a2u}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 69 /* milli weight units */
}
,[RIGHT_EXTEND_8_64] =
{ .tag = JET
, .jet = simplicity_right_extend_8_64
, .cmr = {{0x620a3703u, 0x8b6fa127u, 0x495f0b46u, 0x496f6435u, 0xdd2dad7eu, 0xf0c0fd2cu, 0xd65f54dcu, 0x185e997bu}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 141 /* milli weight units */
}
,[RIGHT_PAD_HIGH_16_32] =
{ .tag = JET
, .jet = simplicity_right_pad_high_16_32
, .cmr = {{0x2b6abc38u, 0x321a7c54u, 0x2fb16974u, 0x621ced80u, 0x880db519u, 0xbb486093u, 0x426e8ce1u, 0x8e0169b1u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 66 /* milli weight units */
}
,[RIGHT_PAD_HIGH_16_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_16_64
, .cmr = {{0xad90d8ffu, 0xa57450b3u, 0xb5e90962u, 0x25349ed8u, 0xf072e101u, 0x7293f392u, 0xef854e03u, 0x19abc934u}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 81 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_16] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_16
, .cmr = {{0x288158b1u, 0xc910877bu, 0x7eea3dfcu, 0xf2b2b788u, 0x922808b6u, 0xd6fa75f8u, 0x96771904u, 0x8b141249u}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 114 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_32] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_32
, .cmr = {{0xee2ad77fu, 0x668d3d6au, 0x2e68506eu, 0x4904cf50u, 0xa08460e1u, 0xd2b86a81u, 0xe14e41f8u, 0xda4cddf2u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 220 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_64
, .cmr = {{0x3d6a7fe6u, 0x9a11642au, 0xced6842bu, 0x89aa1bb8u, 0x413e3990u, 0x63cc1678u, 0x6af7c033u, 0xdad58b95u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 313 /* milli weight units */
}
,[RIGHT_PAD_HIGH_1_8] =
{ .tag = JET
, .jet = simplicity_right_pad_high_1_8
, .cmr = {{0x2844bdfdu, 0x6aba29dfu, 0x03f93aa6u, 0xaeb21c06u, 0x4028db05u, 0xff77d8d9u, 0x1cfdcdefu, 0xb190c5bdu}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 73 /* milli weight units */
}
,[RIGHT_PAD_HIGH_32_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_32_64
, .cmr = {{0xb432e532u, 0x1ae1714cu, 0xe19529d8u, 0x5f24ff89u, 0x87910ebcu, 0xf015f87fu, 0x15bbed55u, 0xf0a0e892u}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 62 /* milli weight units */
}
,[RIGHT_PAD_HIGH_8_16] =
{ .tag = JET
, .jet = simplicity_right_pad_high_8_16
, .cmr = {{0x6f2d96c9u, 0x5413ca9au, 0xa8cc550fu, 0x2573e166u, 0x9956d607u, 0x692cf1cau, 0x6dc76d2fu, 0x2b4a3ac8u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 75 /* milli weight units */
}
,[RIGHT_PAD_HIGH_8_32] =
{ .tag = JET
, .jet = simplicity_right_pad_high_8_32
, .cmr = {{0xdf2c7f92u, 0x9900a449u, 0x01e6ff65u, 0x276a951au, 0xeb95df25u, 0x0b139714u, 0xd4195404u, 0xd77898edu}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 81 /* milli weight units */
}
,[RIGHT_PAD_HIGH_8_64] =
{ .tag = JET
, .jet = simplicity_right_pad_high_8_64
, .cmr = {{0x79c01da3u, 0xe60b9c69u, 0x35ce3e15u, 0x98b17840u, 0xaf82dcb0u, 0xddc63aefu, 0x4a06e7f9u, 0xca5d2741u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 118 /* milli weight units */
}
,[RIGHT_PAD_LOW_16_32] =
{ .tag = JET
, .jet = simplicity_right_pad_low_16_32
, .cmr = {{0x6f201027u, 0xcc759802u, 0x30a07085u, 0x9c3e3802u, 0x36a1cb10u, 0xe61a01aau, 0x1f6d231du, 0x15142f25u}}
, .sourceIx = ty_w16
, .targetIx = ty_w32
, .cost = 62 /* milli weight units */
}
,[RIGHT_PAD_LOW_16_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_16_64
, .cmr = {{0xb86e1f0bu, 0xfec65598u, 0xd0a3d1ecu, 0x960305b9u, 0x6745673eu, 0x1b16bf32u, 0x7a716805u, 0x83d71d90u}}
, .sourceIx = ty_w16
, .targetIx = ty_w64
, .cost = 98 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_16] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_16
, .cmr = {{0x052a6499u, 0xc93ee6bcu, 0x1ae657f8u, 0x5fd4d4feu, 0x677abceeu, 0x540d1340u, 0x33542e9au, 0xb60a63ddu}}
, .sourceIx = ty_b
, .targetIx = ty_w16
, .cost = 60 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_32] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_32
, .cmr = {{0x5b70d428u, 0x960e95ccu, 0x40d51846u, 0xf53a4d0au, 0x35c9015du, 0x1500b6bcu, 0x849b7283u, 0x5e2bd440u}}
, .sourceIx = ty_b
, .targetIx = ty_w32
, .cost = 47 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_64
, .cmr = {{0x44efeb87u, 0xca2ad7fdu, 0x4b73f163u, 0x07c7f059u, 0x02656f35u, 0x090fb0a4u, 0x326c6489u, 0x88ae1d39u}}
, .sourceIx = ty_b
, .targetIx = ty_w64
, .cost = 57 /* milli weight units */
}
,[RIGHT_PAD_LOW_1_8] =
{ .tag = JET
, .jet = simplicity_right_pad_low_1_8
, .cmr = {{0x9340398bu, 0xcc8ea83eu, 0xc840be72u, 0x9dbb8b81u, 0x207824eeu, 0x875d1582u, 0x59d6dad2u, 0x0a83930cu}}
, .sourceIx = ty_b
, .targetIx = ty_w8
, .cost = 48 /* milli weight units */
}
,[RIGHT_PAD_LOW_32_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_32_64
, .cmr = {{0x693e2810u, 0x1e04fda4u, 0x3b97e611u, 0xf0fe9800u, 0x0e14302eu, 0x5dcd6ed6u, 0x5eee42e3u, 0x4014242fu}}
, .sourceIx = ty_w32
, .targetIx = ty_w64
, .cost = 74 /* milli weight units */
}
,[RIGHT_PAD_LOW_8_16] =
{ .tag = JET
, .jet = simplicity_right_pad_low_8_16
, .cmr = {{0x096b25c3u, 0xc8415f04u, 0xd8832743u, 0xeb2f8456u, 0xd5f0a644u, 0x913d3ec5u, 0x9d34f455u, 0x2501fa20u}}
, .sourceIx = ty_w8
, .targetIx = ty_w16
, .cost = 62 /* milli weight units */
}
,[RIGHT_PAD_LOW_8_32] =
{ .tag = JET
, .jet = simplicity_right_pad_low_8_32
, .cmr = {{0xfc7f5722u, 0xa62aa220u, 0x18cc81cdu, 0x00a9326cu, 0x7fe9c63au, 0xbce2bda4u, 0xc0e66a3fu, 0x47c67c53u}}
, .sourceIx = ty_w8
, .targetIx = ty_w32
, .cost = 69 /* milli weight units */
}
,[RIGHT_PAD_LOW_8_64] =
{ .tag = JET
, .jet = simplicity_right_pad_low_8_64
, .cmr = {{0xa5bb7d5eu, 0xfca0e48du, 0x9d80c502u, 0x7115b485u, 0x781051e0u, 0xef46e4d6u, 0x08317a1cu, 0x4261bc46u}}
, .sourceIx = ty_w8
, .targetIx = ty_w64
, .cost = 98 /* milli weight units */
}
,[RIGHT_ROTATE_16] =
{ .tag = JET
, .jet = simplicity_right_rotate_16
, .cmr = {{0x482ea7e1u, 0x214501d9u, 0x3c9ad16fu, 0xa8b97bf5u, 0xb384fc2bu, 0x54789b8cu, 0xd9e784ccu, 0xd0eb9d57u}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 67 /* milli weight units */
}
,[RIGHT_ROTATE_32] =
{ .tag = JET
, .jet = simplicity_right_rotate_32
, .cmr = {{0x0941b6eeu, 0xea9af819u, 0x5b028afcu, 0x0bd2a534u, 0x218bf90du, 0x1a0e373du, 0x74741854u, 0x0b726d73u}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 77 /* milli weight units */
}
,[RIGHT_ROTATE_64] =
{ .tag = JET
, .jet = simplicity_right_rotate_64
, .cmr = {{0x444dbbc3u, 0xdd2a11a5u, 0xc7b0439fu, 0xdba99ac7u, 0x4a11b8eeu, 0xb2db301eu, 0x243ea891u, 0x22907152u}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 64 /* milli weight units */
}
,[RIGHT_ROTATE_8] =
{ .tag = JET
, .jet = simplicity_right_rotate_8
, .cmr = {{0x7265a30cu, 0x2e836e65u, 0x544aba91u, 0x1b64d18fu, 0xa69b1765u, 0x45856c77u, 0xc4f0d76fu, 0xc3f58351u}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 72 /* milli weight units */
}
,[RIGHT_SHIFT_16] =
{ .tag = JET
, .jet = simplicity_right_shift_16
, .cmr = {{0xcd57a3d3u, 0xab2d92d4u, 0xf0865504u, 0x3a8b8bb6u, 0x738981fau, 0xe6da0134u, 0xb4dedaceu, 0x5f008860u}}
, .sourceIx = ty_pw4w16
, .targetIx = ty_w16
, .cost = 60 /* milli weight units */
}
,[RIGHT_SHIFT_32] =
{ .tag = JET
, .jet = simplicity_right_shift_32
, .cmr = {{0xd6b326b1u, 0xa32357a3u, 0x32807d3fu, 0xa1b156c2u, 0x8b1622f7u, 0x38def126u, 0x81467f34u, 0x9bd3494bu}}
, .sourceIx = ty_pw8w32
, .targetIx = ty_w32
, .cost = 69 /* milli weight units */
}
,[RIGHT_SHIFT_64] =
{ .tag = JET
, .jet = simplicity_right_shift_64
, .cmr = {{0xb2095f2du, 0x47335d5fu, 0x98c85434u, 0xa2faf5b0u, 0xf75cf899u, 0x012a34bbu, 0xcd0a14cbu, 0xedb61107u}}
, .sourceIx = ty_pw8w64
, .targetIx = ty_w64
, .cost = 68 /* milli weight units */
}
,[RIGHT_SHIFT_8] =
{ .tag = JET
, .jet = simplicity_right_shift_8
, .cmr = {{0x4b2b1aa2u, 0xef732173u, 0x170d621au, 0x38deb261u, 0xe473c07cu, 0x558b055au, 0x25a86e4eu, 0x321afc04u}}
, .sourceIx = ty_pw4w8
, .targetIx = ty_w8
, .cost = 63 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_16] =
{ .tag = JET
, .jet = simplicity_right_shift_with_16
, .cmr = {{0x14b77685u, 0x47b3d3f4u, 0x7ee5c2b8u, 0x0d9bdae2u, 0xaec1f9c6u, 0x594ed312u, 0x7b12645au, 0xdcf59754u}}
, .sourceIx = ty_pbpw4w16
, .targetIx = ty_w16
, .cost = 83 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_32] =
{ .tag = JET
, .jet = simplicity_right_shift_with_32
, .cmr = {{0x327b6e98u, 0xa6fd340cu, 0x60cf83aau, 0x64993311u, 0x4cb8d84fu, 0x590e0121u, 0x3a261001u, 0x2b4607eau}}
, .sourceIx = ty_pbpw8w32
, .targetIx = ty_w32
, .cost = 78 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_64] =
{ .tag = JET
, .jet = simplicity_right_shift_with_64
, .cmr = {{0x062fa74au, 0xf3476e59u, 0x387be08eu, 0x6949a005u, 0x43bc84a2u, 0xb689ea39u, 0xad6eed7fu, 0x756785d4u}}
, .sourceIx = ty_pbpw8w64
, .targetIx = ty_w64
, .cost = 72 /* milli weight units */
}
,[RIGHT_SHIFT_WITH_8] =
{ .tag = JET
, .jet = simplicity_right_shift_with_8
, .cmr = {{0x141be47eu, 0x967b2fd7u, 0xc7126c5au, 0xdf2dfe47u, 0x315bbc10u, 0x53bbe605u, 0xb38898dbu, 0xed49f227u}}
, .sourceIx = ty_pbpw4w8
, .targetIx = ty_w8
, .cost = 71 /* milli weight units */
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
, .cmr = {{0xc18b9fddu, 0x340a267au, 0xc16d4f39u, 0xee754356u, 0x52aaca52u, 0x5650b51au, 0x45879804u, 0x8e627d51u}}
, .sourceIx = ty_w16
, .targetIx = ty_w2
, .cost = 65 /* milli weight units */
}
,[RIGHTMOST_16_4] =
{ .tag = JET
, .jet = simplicity_rightmost_16_4
, .cmr = {{0xc6c53fa7u, 0x1e230cf0u, 0x585158f4u, 0x70588bacu, 0x5c518f84u, 0xf9fc2386u, 0x52f175fbu, 0x6ea18c11u}}
, .sourceIx = ty_w16
, .targetIx = ty_w4
, .cost = 72 /* milli weight units */
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
, .cmr = {{0x1c442369u, 0xfb81f611u, 0xd328010bu, 0x864bccb7u, 0xf35ed477u, 0xdfa38555u, 0x74c13564u, 0xcdbdb860u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 70 /* milli weight units */
}
,[RIGHTMOST_32_16] =
{ .tag = JET
, .jet = simplicity_rightmost_32_16
, .cmr = {{0xadd2c339u, 0x0d9af7c2u, 0x4a159a37u, 0xd69d4484u, 0xd2c24a2cu, 0xb5b0eb2du, 0x3c493d98u, 0x12acfd74u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 56 /* milli weight units */
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
, .cmr = {{0x84fa5a54u, 0xf7729f9du, 0x68994beau, 0xb93ae79bu, 0x8c4a10d5u, 0xb7ae9727u, 0xaa1716e5u, 0x7d033b74u}}
, .sourceIx = ty_w32
, .targetIx = ty_w4
, .cost = 57 /* milli weight units */
}
,[RIGHTMOST_32_8] =
{ .tag = JET
, .jet = simplicity_rightmost_32_8
, .cmr = {{0x7d3805d3u, 0xc78c4eeau, 0x91e3d35eu, 0xfdd47eedu, 0xd421af84u, 0xd2191032u, 0x9332a0b5u, 0x487fab63u}}
, .sourceIx = ty_w32
, .targetIx = ty_w8
, .cost = 55 /* milli weight units */
}
,[RIGHTMOST_64_1] =
{ .tag = JET
, .jet = simplicity_rightmost_64_1
, .cmr = {{0xd3b164c5u, 0xdc66cc7eu, 0xf9234fedu, 0xe4dc7f0du, 0xa5cd71c1u, 0xc1d4cad6u, 0x0fb4ec57u, 0x3e2b8a75u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 61 /* milli weight units */
}
,[RIGHTMOST_64_16] =
{ .tag = JET
, .jet = simplicity_rightmost_64_16
, .cmr = {{0xeae43478u, 0xf9f2f452u, 0xefac15eeu, 0xe60f8b52u, 0x53d80a2du, 0x32129b4eu, 0x5ba38300u, 0xad9852fdu}}
, .sourceIx = ty_w64
, .targetIx = ty_w16
, .cost = 63 /* milli weight units */
}
,[RIGHTMOST_64_2] =
{ .tag = JET
, .jet = simplicity_rightmost_64_2
, .cmr = {{0x9cd4a98bu, 0xbdb8a335u, 0x85c00f47u, 0xd6adab7au, 0xf54286fbu, 0x8ae60f72u, 0x3011fb84u, 0xc0ee78f9u}}
, .sourceIx = ty_w64
, .targetIx = ty_w2
, .cost = 65 /* milli weight units */
}
,[RIGHTMOST_64_32] =
{ .tag = JET
, .jet = simplicity_rightmost_64_32
, .cmr = {{0x7f2420aeu, 0x5b0f5a3fu, 0x6f2e60b6u, 0x1f8a415cu, 0x088b94b2u, 0x1c1a62a3u, 0xfdaac749u, 0xdbdf4c71u}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 64 /* milli weight units */
}
,[RIGHTMOST_64_4] =
{ .tag = JET
, .jet = simplicity_rightmost_64_4
, .cmr = {{0xe265552au, 0x24fbcdecu, 0x0583d718u, 0x3e48ebc2u, 0xff6d3165u, 0x57bac591u, 0x5c03cb23u, 0x35d23295u}}
, .sourceIx = ty_w64
, .targetIx = ty_w4
, .cost = 57 /* milli weight units */
}
,[RIGHTMOST_64_8] =
{ .tag = JET
, .jet = simplicity_rightmost_64_8
, .cmr = {{0x98cd95f9u, 0x5d46641bu, 0x049e77bfu, 0x90eea598u, 0xadf29ee5u, 0x00e65072u, 0x87548bb1u, 0xcdaf784du}}
, .sourceIx = ty_w64
, .targetIx = ty_w8
, .cost = 49 /* milli weight units */
}
,[RIGHTMOST_8_1] =
{ .tag = JET
, .jet = simplicity_rightmost_8_1
, .cmr = {{0x0876fcd4u, 0x698591f3u, 0x31910157u, 0x4ce153fcu, 0xdfe94f58u, 0x1aac5e75u, 0xf3cd7446u, 0xdf56f3c7u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 65 /* milli weight units */
}
,[RIGHTMOST_8_2] =
{ .tag = JET
, .jet = simplicity_rightmost_8_2
, .cmr = {{0xb9f7b290u, 0xafe7f189u, 0xe32aebf2u, 0xcc4ddca9u, 0x6bb00764u, 0xc7be2887u, 0xdce054d0u, 0x9e38c353u}}
, .sourceIx = ty_w8
, .targetIx = ty_w2
, .cost = 63 /* milli weight units */
}
,[RIGHTMOST_8_4] =
{ .tag = JET
, .jet = simplicity_rightmost_8_4
, .cmr = {{0xf28e9af5u, 0xaf4c9ccau, 0x4b43cc6au, 0xdf9d9d8du, 0x169c87c5u, 0x559f9f3cu, 0xcac8f235u, 0x2b629f18u}}
, .sourceIx = ty_w8
, .targetIx = ty_w4
, .cost = 56 /* milli weight units */
}
,[SCALAR_ADD] =
{ .tag = JET
, .jet = simplicity_scalar_add
, .cmr = {{0x11ddbebau, 0xebf42180u, 0xa0b7eddfu, 0xfdc48ec7u, 0x511330fbu, 0x3315fa65u, 0xd58aff66u, 0xb9caf2d4u}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 778 /* milli weight units */
}
,[SCALAR_INVERT] =
{ .tag = JET
, .jet = simplicity_scalar_invert
, .cmr = {{0xa6392725u, 0xbb2dadbbu, 0x1e76df2du, 0xec57df55u, 0xc3fcc577u, 0x3b62218au, 0xec55a75eu, 0x14f3d60du}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 3178 /* milli weight units */
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
, .cmr = {{0x4a61672au, 0xcec48877u, 0x56de1db6u, 0x0421a12bu, 0x901a858au, 0x6ee6352eu, 0x559d4ce5u, 0x973352beu}}
, .sourceIx = ty_w512
, .targetIx = ty_w256
, .cost = 793 /* milli weight units */
}
,[SCALAR_MULTIPLY_LAMBDA] =
{ .tag = JET
, .jet = simplicity_scalar_multiply_lambda
, .cmr = {{0x49ea9c3fu, 0xb1d8ff52u, 0xd2db0346u, 0x9fdfe850u, 0x503fddebu, 0x45e16d26u, 0xe8928addu, 0x25870e91u}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 567 /* milli weight units */
}
,[SCALAR_NEGATE] =
{ .tag = JET
, .jet = simplicity_scalar_negate
, .cmr = {{0x1dbf8b49u, 0x1ec66580u, 0x3f633330u, 0xd3ffb0e7u, 0x81e67c18u, 0x01ac9d49u, 0xbbf43589u, 0xabf782bfu}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 516 /* milli weight units */
}
,[SCALAR_NORMALIZE] =
{ .tag = JET
, .jet = simplicity_scalar_normalize
, .cmr = {{0x4633180eu, 0xa02c4df7u, 0x819d3d54u, 0xa401734fu, 0x965b31acu, 0xc784054eu, 0xbfb73168u, 0x16b029ecu}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 500 /* milli weight units */
}
,[SCALAR_SQUARE] =
{ .tag = JET
, .jet = simplicity_scalar_square
, .cmr = {{0x8a279e6fu, 0x613aa9e9u, 0x34f2f2a3u, 0x43c0d329u, 0x1c3670e2u, 0x97ddae20u, 0x529e8250u, 0x69efea0eu}}
, .sourceIx = ty_w256
, .targetIx = ty_w256
, .cost = 571 /* milli weight units */
}
,[SCALE] =
{ .tag = JET
, .jet = simplicity_scale
, .cmr = {{0x126e2212u, 0x5bac80b9u, 0x9b7b7343u, 0xb4e5e586u, 0x60821610u, 0x5d4de6f7u, 0x94add34eu, 0x23b195cau}}
, .sourceIx = ty_pw256pw512w256
, .targetIx = ty_pw512w256
, .cost = 73548 /* milli weight units */
}
,[SCRIPT_CMR] =
{ .tag = JET
, .jet = simplicity_bitcoin_script_cmr
, .cmr = {{0xa8a4a622u, 0x10b5e495u, 0x0e253424u, 0x7c7411d1u, 0xc8ff2286u, 0x5b5456bbu, 0xb21638e9u, 0x14f5e528u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 122 /* milli weight units */
}
,[SHA_256_BLOCK] =
{ .tag = JET
, .jet = simplicity_sha_256_block
, .cmr = {{0x4535f3e1u, 0xab9f1b75u, 0x7a069137u, 0xe1d5b1cau, 0xad8e31f7u, 0x8dc5fbd0u, 0x734649f9u, 0x40a7fc96u}}
, .sourceIx = ty_pw256w512
, .targetIx = ty_w256
, .cost = 765 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_1] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_1
, .cmr = {{0x9a4711b8u, 0xc5690e58u, 0x7e5f79e6u, 0x8d6eca04u, 0x7458aa63u, 0xb8bc9ee5u, 0x68086a4au, 0x1b56d834u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w8
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 664 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_128] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_128
, .cmr = {{0x1cb1db8au, 0x055b3197u, 0xacf0f08cu, 0xe9c635adu, 0xd695b60fu, 0x234b18e0u, 0xb323c937u, 0xb0385aeau}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w1Ki
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1778 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_16] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_16
, .cmr = {{0xe0845475u, 0xebb90140u, 0xfa4e01afu, 0x8a943599u, 0x1ad87af9u, 0x8c08aeceu, 0x110e99cbu, 0xcecdee79u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w128
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 781 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_2] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_2
, .cmr = {{0x7d69138fu, 0x1c942beeu, 0x2fdf600cu, 0xe44b36ffu, 0x97839dc2u, 0xbbdafbd5u, 0xfab4dfbcu, 0x3c976f29u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w16
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 674 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_256] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_256
, .cmr = {{0x4f5c29d5u, 0x3686c060u, 0x62b38324u, 0xf8aff17eu, 0xc556a295u, 0xff098b10u, 0xe705dd22u, 0xe13bc3c9u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w2Ki
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 2894 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_32] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_32
, .cmr = {{0xd57b67b1u, 0x74e78e38u, 0xf9bca8e0u, 0x7add61c7u, 0x53e2c156u, 0xd8e9832au, 0xa6620455u, 0x00f51a80u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w256
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 928 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_4] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_4
, .cmr = {{0x95da3299u, 0x3f5c7d00u, 0x83064cdfu, 0xf1bec3b9u, 0x36c63833u, 0x7adec547u, 0x487af232u, 0xd69fdf65u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w32
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 656 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_512] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_512
, .cmr = {{0x4acb163au, 0xa48f09d5u, 0xf26d2b2au, 0xb188a6c6u, 0xb6c4aedfu, 0x23c91900u, 0x1c02ee15u, 0xb337a96eu}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w4Ki
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 5161 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_64] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_64
, .cmr = {{0x52e53ec5u, 0x770f9be4u, 0x069aeefcu, 0xb21322b1u, 0x3ab6e394u, 0x1fdc2c85u, 0xf4b41be6u, 0x7d38ea7eu}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w512
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1220 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_8] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_8
, .cmr = {{0xc26b28afu, 0xe5e866d8u, 0x4616814du, 0x1a13fb86u, 0x30b9e84eu, 0x5d781556u, 0xc6d8236eu, 0xfb45dff9u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256w64
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 694 /* milli weight units */
}
,[SHA_256_CTX_8_ADD_BUFFER_511] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_add_buffer_511
, .cmr = {{0xad699046u, 0x48a8238du, 0x00d85163u, 0xfce81963u, 0xa0047ab5u, 0x82be97a4u, 0x14006559u, 0x79cfdd28u}}
, .sourceIx = ty_pppmw256pmw128pmw64pmw32pmw16mw8pw64w256pmw2Kipmw1Kipmw512pmw256pmw128pmw64pmw32pmw16mw8
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 5137 /* milli weight units */
}
,[SHA_256_CTX_8_FINALIZE] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_finalize
, .cmr = {{0x8e45bdc3u, 0x87d4edfau, 0x733525f3u, 0xab19e42bu, 0x58ecb1b5u, 0xf6dccf94u, 0xedbf5995u, 0x8ae3e116u}}
, .sourceIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .targetIx = ty_w256
, .cost = 833 /* milli weight units */
}
,[SHA_256_CTX_8_INIT] =
{ .tag = JET
, .jet = simplicity_sha_256_ctx_8_init
, .cmr = {{0x635f6405u, 0x848685c0u, 0x11febd41u, 0xfaac874bu, 0xbbf5b24du, 0x5fb12fedu, 0xbcb6cbffu, 0x95a0f366u}}
, .sourceIx = ty_u
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 123 /* milli weight units */
}
,[SHA_256_IV] =
{ .tag = JET
, .jet = simplicity_sha_256_iv
, .cmr = {{0x12e45937u, 0x51c9463bu, 0x562503c1u, 0x40d78b3bu, 0x757a1f4fu, 0x16321d28u, 0x62d32543u, 0x8538971bu}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 92 /* milli weight units */
}
,[SIG_ALL_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_sig_all_hash
, .cmr = {{0x0978b9e5u, 0x0b9e8e09u, 0x8b27f2b8u, 0xb59de54fu, 0x62ba7c13u, 0x33df3bedu, 0x221e2662u, 0x6805bc55u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 120 /* milli weight units */
}
,[SOME_1] =
{ .tag = JET
, .jet = simplicity_some_1
, .cmr = {{0x15ca4e4bu, 0x82c2f91bu, 0x9a792992u, 0xcdc1b292u, 0xab86a2d2u, 0x939c9a64u, 0xb50be60bu, 0xda6ab4cau}}
, .sourceIx = ty_b
, .targetIx = ty_b
, .cost = 60 /* milli weight units */
}
,[SOME_16] =
{ .tag = JET
, .jet = simplicity_some_16
, .cmr = {{0xa9dfbbeau, 0xb59df72au, 0x45fc3fc7u, 0xac581ec8u, 0xda713f2fu, 0x8103f787u, 0xaa1cee4eu, 0x0ba64866u}}
, .sourceIx = ty_w16
, .targetIx = ty_b
, .cost = 52 /* milli weight units */
}
,[SOME_32] =
{ .tag = JET
, .jet = simplicity_some_32
, .cmr = {{0x4633a397u, 0x742ef482u, 0xbe2fa3fbu, 0x6410ec79u, 0xc3738365u, 0x69fbbcb1u, 0xf948ec32u, 0x487378b7u}}
, .sourceIx = ty_w32
, .targetIx = ty_b
, .cost = 49 /* milli weight units */
}
,[SOME_64] =
{ .tag = JET
, .jet = simplicity_some_64
, .cmr = {{0x1dc245acu, 0x6f5b422bu, 0xd1886ef5u, 0x144c4dc7u, 0x2c967315u, 0x5966076cu, 0xd839681du, 0x9ec7f8f5u}}
, .sourceIx = ty_w64
, .targetIx = ty_b
, .cost = 62 /* milli weight units */
}
,[SOME_8] =
{ .tag = JET
, .jet = simplicity_some_8
, .cmr = {{0x33afb9c6u, 0x454e590eu, 0xc13ed75eu, 0x1b7d9c3au, 0x3de6752bu, 0xcc7c1d4cu, 0xb363fa51u, 0x828bcb74u}}
, .sourceIx = ty_w8
, .targetIx = ty_b
, .cost = 57 /* milli weight units */
}
,[SUBTRACT_16] =
{ .tag = JET
, .jet = simplicity_subtract_16
, .cmr = {{0x4e06ec31u, 0x376222e2u, 0x5e27d015u, 0x9dc1c071u, 0x4a44ca6au, 0xacf9505cu, 0xaad280e9u, 0x73fb5cabu}}
, .sourceIx = ty_w32
, .targetIx = ty_pbw16
, .cost = 93 /* milli weight units */
}
,[SUBTRACT_32] =
{ .tag = JET
, .jet = simplicity_subtract_32
, .cmr = {{0xb9c0f36eu, 0x7522a8d9u, 0x49050d51u, 0x6a05ce20u, 0x3a1f9a9eu, 0x372fd263u, 0xde38b0e9u, 0x03134198u}}
, .sourceIx = ty_w64
, .targetIx = ty_pbw32
, .cost = 87 /* milli weight units */
}
,[SUBTRACT_64] =
{ .tag = JET
, .jet = simplicity_subtract_64
, .cmr = {{0x1cdb5c74u, 0xadd102f5u, 0x0f938ed8u, 0x86f496e5u, 0xbab2755cu, 0x3c484e88u, 0x87903d2fu, 0x6a57f3aau}}
, .sourceIx = ty_w128
, .targetIx = ty_pbw64
, .cost = 125 /* milli weight units */
}
,[SUBTRACT_8] =
{ .tag = JET
, .jet = simplicity_subtract_8
, .cmr = {{0x4f2117a0u, 0xe81059ffu, 0x0cd64d84u, 0x886542e5u, 0x75ea8d6eu, 0xc03108fdu, 0x0b508b39u, 0x208cd0efu}}
, .sourceIx = ty_w16
, .targetIx = ty_pbw8
, .cost = 96 /* milli weight units */
}
,[SWU] =
{ .tag = JET
, .jet = simplicity_swu
, .cmr = {{0x00f51f4fu, 0x4bece790u, 0x03ecad48u, 0x1a125af7u, 0x176e4de9u, 0x8c339242u, 0x5cb91466u, 0x26c13b3bu}}
, .sourceIx = ty_w256
, .targetIx = ty_w512
, .cost = 32780 /* milli weight units */
}
,[TAP_ENV_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_tap_env_hash
, .cmr = {{0x19d9944cu, 0x4d457c70u, 0xbabc45ccu, 0xcbd573b3u, 0x9a51d0c9u, 0x9115b412u, 0x783b4900u, 0x82fd0e58u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 120 /* milli weight units */
}
,[TAPDATA_INIT] =
{ .tag = JET
, .jet = simplicity_tapdata_init
, .cmr = {{0xa4d022efu, 0x5cf467bcu, 0xa0325e46u, 0x3fcace7cu, 0xbdd64ff8u, 0xf71c5c7fu, 0x63e60784u, 0xaa0ac486u}}
, .sourceIx = ty_u
, .targetIx = ty_ppmw256pmw128pmw64pmw32pmw16mw8pw64w256
, .cost = 1233 /* milli weight units */
}
,[TAPLEAF_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_tapleaf_hash
, .cmr = {{0x0c0716feu, 0x5d978ea8u, 0xe0c75adcu, 0x8210d660u, 0x062e3da0u, 0x6f1a6661u, 0x317927d3u, 0xb84b5073u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 116 /* milli weight units */
}
,[TAPLEAF_VERSION] =
{ .tag = JET
, .jet = simplicity_bitcoin_tapleaf_version
, .cmr = {{0xe2ecb1cbu, 0x0ed4ed93u, 0x483545fdu, 0x8a62f8aau, 0x10175349u, 0xffcc5ad3u, 0xde7f3484u, 0xea1f103fu}}
, .sourceIx = ty_u
, .targetIx = ty_w8
, .cost = 66 /* milli weight units */
}
,[TAPPATH] =
{ .tag = JET
, .jet = simplicity_bitcoin_tappath
, .cmr = {{0x99e8211eu, 0x8c1be6d9u, 0xca98d3d4u, 0x3d914256u, 0x7e06a840u, 0x22393303u, 0xfab0b57du, 0x394074e8u}}
, .sourceIx = ty_w8
, .targetIx = ty_mw256
, .cost = 76 /* milli weight units */
}
,[TAPPATH_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_tappath_hash
, .cmr = {{0x0211546du, 0x0778e787u, 0x141ece65u, 0xdf6cd1dbu, 0x31388fc1u, 0x421968c8u, 0xcfd8d759u, 0x26db47b3u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 123 /* milli weight units */
}
,[TOTAL_INPUT_VALUE] =
{ .tag = JET
, .jet = simplicity_bitcoin_total_input_value
, .cmr = {{0x81287a15u, 0x89ddc217u, 0x9926b706u, 0x5e9b2677u, 0xb7fb099bu, 0x95f947a7u, 0xdd590cddu, 0x4ccf4a56u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 69 /* milli weight units */
}
,[TOTAL_OUTPUT_VALUE] =
{ .tag = JET
, .jet = simplicity_bitcoin_total_output_value
, .cmr = {{0xba032f3eu, 0x62f8fcb0u, 0x4b0429a2u, 0x53b5ec57u, 0xe4c87ae2u, 0xe951e145u, 0xd7650e7fu, 0x0c63e555u}}
, .sourceIx = ty_u
, .targetIx = ty_w64
, .cost = 71 /* milli weight units */
}
,[TRANSACTION_ID] =
{ .tag = JET
, .jet = simplicity_bitcoin_transaction_id
, .cmr = {{0x5c49ea98u, 0x6b328be2u, 0xa4e0b315u, 0xb6b3fef2u, 0x3c1f6856u, 0xbbc3b056u, 0xc99cf59fu, 0xbd546e65u}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 122 /* milli weight units */
}
,[TX_HASH] =
{ .tag = JET
, .jet = simplicity_bitcoin_tx_hash
, .cmr = {{0x54e53c99u, 0x93abd55du, 0x1f8523d2u, 0xbb217b32u, 0xe6fe861fu, 0x84c986b7u, 0xee8bdc68u, 0x8106874au}}
, .sourceIx = ty_u
, .targetIx = ty_w256
, .cost = 119 /* milli weight units */
}
,[TX_IS_FINAL] =
{ .tag = JET
, .jet = simplicity_bitcoin_tx_is_final
, .cmr = {{0x7b0e4f4cu, 0xa8e5af61u, 0xa1d3454eu, 0x11ef9ab6u, 0x88706121u, 0x1c0090ebu, 0xa9553da2u, 0xe45d8473u}}
, .sourceIx = ty_u
, .targetIx = ty_b
, .cost = 62 /* milli weight units */
}
,[TX_LOCK_DISTANCE] =
{ .tag = JET
, .jet = simplicity_bitcoin_tx_lock_distance
, .cmr = {{0x8c0b0c44u, 0x2bc20978u, 0xfa4c2b3du, 0x52a7fe7cu, 0x3f86e3d2u, 0x1b42130eu, 0xbb8dba45u, 0xe0f5e64bu}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 72 /* milli weight units */
}
,[TX_LOCK_DURATION] =
{ .tag = JET
, .jet = simplicity_bitcoin_tx_lock_duration
, .cmr = {{0x09b18d5cu, 0x2f08402bu, 0xbcf8c31eu, 0xa43435efu, 0xffd9ea0eu, 0xf8758f76u, 0xbb27272bu, 0x4d60dc62u}}
, .sourceIx = ty_u
, .targetIx = ty_w16
, .cost = 66 /* milli weight units */
}
,[TX_LOCK_HEIGHT] =
{ .tag = JET
, .jet = simplicity_bitcoin_tx_lock_height
, .cmr = {{0x449f61dfu, 0x1a7bad8du, 0x9e0a9667u, 0x14227e57u, 0x1007f93cu, 0x517b805du, 0x616510e8u, 0xff62172bu}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 72 /* milli weight units */
}
,[TX_LOCK_TIME] =
{ .tag = JET
, .jet = simplicity_bitcoin_tx_lock_time
, .cmr = {{0x31df363fu, 0x17b2cfc9u, 0x7a1f9372u, 0xc14a3258u, 0x64d7cb13u, 0xaa8d5215u, 0xfb253d33u, 0x10917762u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 72 /* milli weight units */
}
,[VERIFY] =
{ .tag = JET
, .jet = simplicity_verify
, .cmr = {{0xcdca2a05u, 0xe52cefa5u, 0x9dc7a5b0u, 0xdae22098u, 0xfb896e39u, 0x13bfdd44u, 0x6b594e1fu, 0x9250783eu}}
, .sourceIx = ty_b
, .targetIx = ty_u
, .cost = 44 /* milli weight units */
}
,[VERSION] =
{ .tag = JET
, .jet = simplicity_bitcoin_version
, .cmr = {{0x83735864u, 0x00b6790bu, 0x46ab0410u, 0x523cf01eu, 0xb74d10fau, 0xf48a3accu, 0x86c4c51du, 0x06a52c49u}}
, .sourceIx = ty_u
, .targetIx = ty_w32
, .cost = 78 /* milli weight units */
}
,[XOR_1] =
{ .tag = JET
, .jet = simplicity_xor_1
, .cmr = {{0x8c4e4e6eu, 0xbf4630b2u, 0x9b5a57eau, 0x79f0c9afu, 0x6bff54c4u, 0xd2d769bfu, 0x51594774u, 0xa52b99c9u}}
, .sourceIx = ty_w2
, .targetIx = ty_b
, .cost = 60 /* milli weight units */
}
,[XOR_16] =
{ .tag = JET
, .jet = simplicity_xor_16
, .cmr = {{0xd9f0af3fu, 0xe3fd247cu, 0x1df34a25u, 0x2713b2e9u, 0x33a945a5u, 0x6719487fu, 0x8ed7f563u, 0xea861ab5u}}
, .sourceIx = ty_w32
, .targetIx = ty_w16
, .cost = 73 /* milli weight units */
}
,[XOR_32] =
{ .tag = JET
, .jet = simplicity_xor_32
, .cmr = {{0xd5ae2712u, 0xedeaf676u, 0x520fa3bau, 0x0f40bf4au, 0x1657437eu, 0xffbd9986u, 0xd06ae81bu, 0x29a4f98cu}}
, .sourceIx = ty_w64
, .targetIx = ty_w32
, .cost = 77 /* milli weight units */
}
,[XOR_64] =
{ .tag = JET
, .jet = simplicity_xor_64
, .cmr = {{0xc4df1ccfu, 0x333edebdu, 0xd40dea9au, 0x0e6cbb83u, 0x0631e83au, 0x94bb779fu, 0xe6007bc6u, 0xcb53a544u}}
, .sourceIx = ty_w128
, .targetIx = ty_w64
, .cost = 68 /* milli weight units */
}
,[XOR_8] =
{ .tag = JET
, .jet = simplicity_xor_8
, .cmr = {{0x4ab14a81u, 0x4a39528au, 0x80fdb430u, 0x589ba450u, 0x104b9c72u, 0x09aa2fe2u, 0x85cd60c0u, 0x9043114au}}
, .sourceIx = ty_w16
, .targetIx = ty_w8
, .cost = 80 /* milli weight units */
}
,[XOR_XOR_1] =
{ .tag = JET
, .jet = simplicity_xor_xor_1
, .cmr = {{0x18b9446au, 0x4166a3feu, 0xe2bcb254u, 0x5bb90118u, 0xdcf0e8f8u, 0x86a1076du, 0x4c386006u, 0x0cde1a51u}}
, .sourceIx = ty_pbw2
, .targetIx = ty_b
, .cost = 50 /* milli weight units */
}
,[XOR_XOR_16] =
{ .tag = JET
, .jet = simplicity_xor_xor_16
, .cmr = {{0x946cde87u, 0x2e30e650u, 0x9daff405u, 0xf0e0fefeu, 0x275547b4u, 0x0eb20384u, 0xafe9a863u, 0x60fc80efu}}
, .sourceIx = ty_pw16w32
, .targetIx = ty_w16
, .cost = 82 /* milli weight units */
}
,[XOR_XOR_32] =
{ .tag = JET
, .jet = simplicity_xor_xor_32
, .cmr = {{0x6527df67u, 0xa50d148du, 0xb4fc8feeu, 0xc7845564u, 0x99a8c7f0u, 0xfa7d28e6u, 0x278e997fu, 0x4959be39u}}
, .sourceIx = ty_pw32w64
, .targetIx = ty_w32
, .cost = 82 /* milli weight units */
}
,[XOR_XOR_64] =
{ .tag = JET
, .jet = simplicity_xor_xor_64
, .cmr = {{0xf162f9e6u, 0x5663a69au, 0xc5f92a5eu, 0xb52c0332u, 0x392edd1eu, 0xd1ba355eu, 0x6f19406eu, 0xabe3f6edu}}
, .sourceIx = ty_pw64w128
, .targetIx = ty_w64
, .cost = 80 /* milli weight units */
}
,[XOR_XOR_8] =
{ .tag = JET
, .jet = simplicity_xor_xor_8
, .cmr = {{0xe06d694cu, 0x5b407ddau, 0xd7aa1f88u, 0x0716bcb7u, 0x0acdba75u, 0x85ca4009u, 0x9a0a0a61u, 0xf3ad2db5u}}
, .sourceIx = ty_pw8w16
, .targetIx = ty_w8
, .cost = 86 /* milli weight units */
}
