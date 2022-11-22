/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2012 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of configuration of prime elliptic curves over quadratic
 * extensions.
 *
 * @ingroup epx
 */

#include "relic_core.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/* See ep/relic_ep_param.c for discussion of MAP_U parameters. */

/**
 * Parameters for a pairing-friendly prime curve over a quadratic extension.
 */
#if defined(EP_ENDOM) && FP_PRIME == 158
/** @{ */
#define BN_P158_A0		"0"
#define BN_P158_A1		"0"
#define BN_P158_B0		"4"
#define BN_P158_B1		"240000006ED000007FE9C000419FEC800CA035C6"
#define BN_P158_X0		"172C0A466DAFB4ACF48C9BDD0C12A435CB36CE6C"
#define BN_P158_X1		"0CE0287269D7E317EB91AF3DCD27CC373114299E"
#define BN_P158_Y0		"19A185D6B6241576480E965463B4A6A66875C184"
#define BN_P158_Y1		"074866EA7BD0AB4C67C77F70E0467F1FF32D800D"
#define BN_P158_R		"240000006ED000007FE96000419F59800C9FFD81"
#define BN_P158_H		"240000006ED000007FEA200041A07F800CA06E0D"
#define BN_P158_MAPU0 "0"
#define BN_P158_MAPU1 "-1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 254
/** @{ */
#define BN_P254_A0		"0"
#define BN_P254_A1		"0"
#define BN_P254_B0		"1"
#define BN_P254_B1		"2523648240000001BA344D80000000086121000000000013A700000000000012"
#define BN_P254_X0		"061A10BB519EB62FEB8D8C7E8C61EDB6A4648BBB4898BF0D91EE4224C803FB2B"
#define BN_P254_X1		"0516AAF9BA737833310AA78C5982AA5B1F4D746BAE3784B70D8C34C1E7D54CF3"
#define BN_P254_Y0		"021897A06BAF93439A90E096698C822329BD0AE6BDBE09BD19F0E07891CD2B9A"
#define BN_P254_Y1		"0EBB2B0E7C8B15268F6D4456F5F38D37B09006FFD739C9578A2D1AEC6B3ACE9B"
#define BN_P254_R		"2523648240000001BA344D8000000007FF9F800000000010A10000000000000D"
#define BN_P254_H		"2523648240000001BA344D8000000008C2A2800000000016AD00000000000019"
#define BN_P254_MAPU0 "0"
#define BN_P254_MAPU1 "-1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 256
/** @{ */
#define BN_P256_A0		"0"
#define BN_P256_A1		"0"
#define BN_P256_B0		"4"
#define BN_P256_B1		"B64000000000FF2F2200000085FD5480B0001F44B6B88BF142BC818F95E3E6AE"
#define BN_P256_X0		"0C77AE4A1D6E145166739CF23DAFACA9DD396E9046424FC5479BD57692904538"
#define BN_P256_X1		"8D1705B45D9EAAD78A9198FD8D76E2013D1BC119B4D95721A8D32F819A544F51"
#define BN_P256_Y0		"A906E963E4988478E458A4959EF7D61B570358814E28A04EF9B8C794064D73A7"
#define BN_P256_Y1		"A033144CA161E3E3271624B3F0CC1CE607ACD2CBCE9E9253C732CF3E1016DEE7"
#define BN_P256_R		"B64000000000FF2F2200000085FD547FD8001F44B6B7F4B7C2BC818F7B6BEF99"
#define BN_P256_H		"B64000000000FF2F2200000085FD548188001F44B6B9232AC2BC818FB05BDDC5"
#define BN_P256_MAPU0 "0"
#define BN_P256_MAPU1 "-1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 256
/** @{ */
#define SM9_P256_A0		"0"
#define SM9_P256_A1		"0"
#define SM9_P256_B0		"0"
#define SM9_P256_B1		"5"
#define SM9_P256_X0		"3722755292130B08D2AAB97FD34EC120EE265948D19C17ABF9B7213BAF82D65B"
#define SM9_P256_X1		"85AEF3D078640C98597B6027B441A01FF1DD2C190F5E93C454806C11D8806141"
#define SM9_P256_Y0		"A7CF28D519BE3DA65F3170153D278FF247EFBA98A71A08116215BBA5C999A7C7"
#define SM9_P256_Y1		"17509B092E845C1266BA0D262CBEE6ED0736A96FA347C8BD856DC76B84EBEB96"
#define SM9_P256_R		"B640000002A3A6F1D603AB4FF58EC74449F2934B18EA8BEEE56EE19CD69ECF25"
#define SM9_P256_H		"B640000002A3A6F1D603AB4FF58EC745F9F2934B1C0B51C8E57054B2F003BBD5"
#define SM9_P256_MAPU0 "-1"
#define SM9_P256_MAPU1 "0"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 381
/** @{ */
#define B12_P381_A0		"0"
#define B12_P381_A1		"0"
#define B12_P381_B0		"4"
#define B12_P381_B1		"4"
#define B12_P381_X0		"024AA2B2F08F0A91260805272DC51051C6E47AD4FA403B02B4510B647AE3D1770BAC0326A805BBEFD48056C8C121BDB8"
#define B12_P381_X1		"13E02B6052719F607DACD3A088274F65596BD0D09920B61AB5DA61BBDC7F5049334CF11213945D57E5AC7D055D042B7E"
#define B12_P381_Y0		"0CE5D527727D6E118CC9CDC6DA2E351AADFD9BAA8CBDD3A76D429A695160D12C923AC9CC3BACA289E193548608B82801"
#define B12_P381_Y1		"0606C4A02EA734CC32ACD2B02BC28B99CB3E287E85A763AF267492AB572E99AB3F370D275CEC1DA1AAA9075FF05F79BE"
#define B12_P381_R		"73EDA753299D7D483339D80809A1D80553BDA402FFFE5BFEFFFFFFFF00000001"
#define B12_P381_H		"5D543A95414E7F1091D50792876A202CD91DE4547085ABAA68A205B2E5A7DDFA628F1CB4D9E82EF21537E293A6691AE1616EC6E786F0C70CF1C38E31C7238E5"
#if defined(EP_CTMAP)
#define B12_P381_ISO_A0 "0"
#define B12_P381_ISO_A1 "F0"
#define B12_P381_ISO_B0 "3F4"
#define B12_P381_ISO_B1 "3F4"
#define B12_P381_ISO_XN "5c759507e8e333ebb5b7a9a47d7ed8532c52d39fd3a042a88b58423c50ae15d5c2638e343d9c71c6238aaaaaaaa97d6,5c759507e8e333ebb5b7a9a47d7ed8532c52d39fd3a042a88b58423c50ae15d5c2638e343d9c71c6238aaaaaaaa97d6;0,11560bf17baa99bc32126fced787c88f984f87adf7ae0c7f9a208c6b4f20a4181472aaa9cb8d555526a9ffffffffc71a;11560bf17baa99bc32126fced787c88f984f87adf7ae0c7f9a208c6b4f20a4181472aaa9cb8d555526a9ffffffffc71e,8ab05f8bdd54cde190937e76bc3e447cc27c3d6fbd7063fcd104635a790520c0a395554e5c6aaaa9354ffffffffe38d;171d6541fa38ccfaed6dea691f5fb614cb14b4e7f4e810aa22d6108f142b85757098e38d0f671c7188e2aaaaaaaa5ed1,0"
#define B12_P381_ISO_XD "0,1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa63;c,1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa9f;1,0"
#define B12_P381_ISO_YN "1530477c7ab4113b59a4c18b076d11930f7da5d4a07f649bf54439d87d27e500fc8c25ebf8c92f6812cfc71c71c6d706,1530477c7ab4113b59a4c18b076d11930f7da5d4a07f649bf54439d87d27e500fc8c25ebf8c92f6812cfc71c71c6d706;0,5c759507e8e333ebb5b7a9a47d7ed8532c52d39fd3a042a88b58423c50ae15d5c2638e343d9c71c6238aaaaaaaa97be;11560bf17baa99bc32126fced787c88f984f87adf7ae0c7f9a208c6b4f20a4181472aaa9cb8d555526a9ffffffffc71c,8ab05f8bdd54cde190937e76bc3e447cc27c3d6fbd7063fcd104635a790520c0a395554e5c6aaaa9354ffffffffe38f;124c9ad43b6cf79bfbf7043de3811ad0761b0f37a1e26286b0e977c69aa274524e79097a56dc4bd9e1b371c71c718b10,0"
#define B12_P381_ISO_YD "1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffa8fb,1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffa8fb;0,1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffa9d3;12,1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa99;1,0"
#define B12_P381_MAPU0 "-2"
#define B12_P381_MAPU1 "-1"
#else /* !defined(EP_CTMAP) */
#define B12_P381_MAPU0 "0"
#define B12_P381_MAPU1 "1"
#endif
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 382
/** @{ */
#define BN_P382_A0		"0"
#define BN_P382_A1		"0"
#define BN_P382_B0		"1"
#define BN_P382_B1		"24009015183F94892D996CC179C6D1666F82CEFBE47879BBA6E58DBE43002A0609480097801382BE004E000000000012"
#define BN_P382_X0		"124D78021D2F75CC7C4CA67A4A0E97BB67A89C66B61C2C06600760B0D7531624F2DD78AEF890A5FC0EB7F8C6FD6FC24C"
#define BN_P382_X1		"0DD2BB64502028D5DED81EAC950BF96D0140DBA681B99152F14A0AC190AC93E4A7EA979A355367B30D23B6E8CAD6F394"
#define BN_P382_Y0		"16C2749A07848E85C9E128D710796AE465DBD5DDC7B285FA02777E378E9FFF0A42E2128C3167B231602B72553DF79669"
#define BN_P382_Y1		"09CAC2932F3C894CCE129ECBB49AFD4BFF94C10B5DF37AB469E3455F16EFDD304721F689BFF864A92ACB3F4DF52678ED"
#define BN_P382_R		"24009015183F94892D996CC179C6D1666F82CEFBE47879BB46E4CDA2E2E2281D08DC008E80108252004200000000000D"
#define BN_P382_H		"24009015183F94892D996CC179C6D1666F82CEFBE47879BC06E64DD9A31E2BEF09B400A08016832A005A000000000019"
#define BN_P382_MAPU0 "0"
#define BN_P382_MAPU1 "-1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 383
/** @{ */
#define B12_P383_A0		"0"
#define B12_P383_A1		"0"
#define B12_P383_B0		"4"
#define B12_P383_B1		"4"
#define B12_P383_X0		"2E4DCF4E55CBA1075B3325DD25C2B80BD907686E55E175F0B5D097E46E78B6B5F8E600B80D2EBD4E410BCCCBAE018976"
#define B12_P383_X1		"0BE0128C473D6A90202AC72A60E988CF7901242A2A4623A89253256CC11C1519D3C633897E071D8F466EA0BFFA5F9313"
#define B12_P383_Y0		"44900D049962DF1BBCB29872FDA90D0DFC2253033569F7D684080F6A5B4F669120A29F908FED5B64AAA9A56502405B39"
#define B12_P383_Y1		"1459B073246773B35CBCBFC9318AC47DDC1C284887D1DA4BE13593ACC0E45B748AF4115011A3EA730A7DB21A83562411"
#define B12_P383_R		"1002001800C00B809C04401C81698B381DE05F095A120D3973B2099EBFEBC0001"
#define B12_P383_H		"1C78E45562AB68EE641721902ED56C9B45EB1276A5C15CB19C7BF81F7A7B6F22053BAC3422240A3F4A7E3C25D470E7BABDB8A2E1E1793BF9CEBDD1BF2EFE30E5"
#define B12_P383_MAPU0 "0"
#define B12_P383_MAPU1 "1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 446
/** @{ */
#define BN_P446_A0		"0"
#define BN_P446_A1		"0"
#define BN_P446_B0		"10"
#define BN_P446_B1		"2400000000000000002400000002D00000000D800000021C0000001800000000870000000B0400000057C00000015C000000132000000066"
#define BN_P446_X0		"1DFCEBAE017EC74D18BFCF2CABB36B7B53B64AD3DE65B2E1F7991A38ADB90BE52FF2AC01B15EDDAAEB92DE6338A40F24A5052A3BBA1F755D"
#define BN_P446_X1		"4CC6E0E84FC2FE13FFCB9B6F716AE188D1532B57754CA4FBD9058E3B7C6419933E76D470BA8365E21DAB35662CD74C0A381020DF944CDD2"
#define BN_P446_Y0		"13043EE14F4BE8FBF314D15D49ACD7928DD6D12CF903D5485F8EDA2B343A2F8E43A61D9FF1FC74788DBA03B064498B143171A0885AD9EC37"
#define BN_P446_Y1		"91F93BEB46071DEDF410DC5A7662DD8B4BBC8BE5D3A8662009A4C2C0577F82A2337D208379F21C65F90FE1D90482CC48DEC83BFB8AD8E45"
#define BN_P446_R		"2400000000000000002400000002D00000000D800000021C00000017A0000000870000000AD400000054C000000156000000126000000061"
#define BN_P446_H		"2400000000000000002400000002D00000000D800000021C0000001860000000870000000B340000005AC00000016200000013E00000006D"
#define BN_P446_MAPU0 "0"
#define BN_P446_MAPU1 "1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 446
/** @{ */
#define B12_P446_A0		"0"
#define B12_P446_A1		"0"
#define B12_P446_B0		"1"
#define B12_P446_B1		"1"
#define B12_P446_X0		"2B416CB07EA30312C0465130F59E1598732EE5DEC92F52F521F9D90CEEDA7E5F97208A5EAEE7DA3F56CFCF3DF403C5E2D96E929C8C064CA6"
#define B12_P446_X1		"1378EBE7D0BE6234B9702E4361DD1432B2965B29F82169693815DB423EE5030F1C78A0137885E737D2C242C8C9CA2704347EB3F1F6D84C57"
#define B12_P446_Y0		"26392FFC4FB772DE4C4D2A8A61AE00E74FEF64D573EA3D06DF741E666688E83495610E4CE1B9C7EAB270A57C8F82D9B459F2392065EEEAF3"
#define B12_P446_Y1		"10ACDEBE52C1499796809283C38E100C4BA9FA0699696BF9F8647CB18EE632A4DDDAA771A78C031F868FAFA07E8642CCA633FE6F99B744C4"
#define B12_P446_R		"511B70539F27995B34995830FA4D04C98CCC4C050BC7BB9B0E8D8CA34610428001400040001"
#define B12_P446_H		"2DAEE3988A06F1BB3444D5780484639BF731D657742BF556901567CEFE5C3DD2555CB74C3560CED298D5147F8E24B79369C54C81E026DD55B51D6A75088593E6A92EFCE555594000638E5"
#define B12_P446_MAPU0 "0"
#define B12_P446_MAPU1 "1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 455
/** @{ */
#define B12_P455_A0		"0"
#define B12_P455_A1		"0"
#define B12_P455_B0		"5"
#define B12_P455_B1		"55555955557955572AA00E0F95B49203003F665E3A5B1D56234BD93954FCB314B8B3DB9994ACE86D1BA6C589556B2AA956AAA00001800002A6"
#define B12_P455_X0		"2EEC192F28D9F29B538E089E5A05014765EAC5E066D4A3083D247C009EE309392AA2D813C9FC798CB292D0655CD9F559C961B6F678109C5676"
#define B12_P455_X1		"26DD95EE22842C030FEB5D46259492EA7E051D604608D35F8F5AFB65B0FC18DADBC079D037D5F9E24634346181876FC2F9D501E9A060ED726D"
#define B12_P455_Y0		"29C445A3809D96184EB3E76193ABBE3688B68B4123ABCC9F31A81F42D2420B1B4D653673CEAC2A0543347EE3B1A56591F3496626F8763EEADB"
#define B12_P455_Y1		"24AC244C6F31FDAC1214CA62AB4DB7BA9B5F0D54D56A3D5C680044225C3AAF9815C272A15AA1D28FB6AC9EB7B0BE6916450794A617AFFB4EF9"
#define B12_P455_R		"10000080000380002E0000F10004F00025E000750001D1000A00000400001C00007FFFFC00001"
#define B12_P455_H		"1C71C8E38E4C71C8238726432AFAC33B9D05A24A43A2531F1F9DE9B4C688A0B7A2716C48868F7C7D716F8AB8D45A1B9B21F9EB90E83876E4E18AD09613EC7DCAF8B8E238558711C838C718E5"
#define B12_P455_MAPU0 "0"
#define B12_P455_MAPU1 "1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 511
/** @{ */
#define OT8_P511_A0		"0"
#define OT8_P511_A1		"1"
#define OT8_P511_B0		"0"
#define OT8_P511_B1		"0"
#define OT8_P511_X0		"09541B7BB446EBE58277E0183B448E09D567ACFAAA07F2D3C01967088544C6FA844B803CFC8C8A91D0DFFAB5F55B95372C5AB5DD38E13EE92DAA6882535A1244"
#define OT8_P511_X1		"332FBBD88DA3493CAF2F082C9C43E463523C8611AC52AB498F1D28D7844E42C67AF62A9BF0F4D9DDD38F79F51C9DBDB10735AC3CD69FF7867E27EBD65DD8D3EF"
#define OT8_P511_Y0		"2E298CA6C71CE0C6CABA9208E6350B73B0E8BF3EE7CC1777C64BD3680AC857D1823993C8877CBD0203CD3A9835A053F5549BDF7DC206EE1B40BA43A2BD59B793"
#define OT8_P511_Y1		"109CF99B6C312D74CEFF87959789AF53D231988B0E77FF424C8738C20EA91E7F634399A3899E101EBF5C6A0DBE2E40ACA8D0DBCE0F2C6A0300987BBABD9097DC"
#define OT8_P511_R		"100000000002AC000000002AD56000000131304C0000032F6D0B1000000000001"
#define OT8_P511_H		"1000000000080400000001D72B20000061916054001384B3D863F2EBD23CF44774A8836D060A488CB13701DF690F23AF2A5394A1F9EE0B564F725AD505A8F75463E3DBDF97FBE852B96A19E4477DB82D7C260034DEDA6C75853BB18EE3956002"
#define OT8_P511_MAPU0 "0"
#define OT8_P511_MAPU1 "1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 544
/** @{ */
#define GMT8_P544_A0		"0"
#define GMT8_P544_A1		"2"
#define GMT8_P544_B0		"0"
#define GMT8_P544_B1		"0"
#define GMT8_P544_X0		"1EE84A939A46EAE4287805E2FFABBCED3605B74F5FD7E545F390D2E3D34AD0C60851EA65A18CF92396B9318FCAB3205D79C8D03E928590F719E4086A1C42AF2D40EB6C3A"
#define GMT8_P544_X1		"065F721D63381C07D1BF556D65A3A82B0E969BA031FB3812F49709926D5159324DFB685048D4EAFC914F893A0D5382868D1D252619AA964149258DCC1A86B68775F36A82"
#define GMT8_P544_Y0		"9B80046683050F459F48D7CE4C83992D5B044718326AEE2DD86F3E5557BD240441A763BC40030F21FC3A64483B450330930BAFA62E7B573B565404A9DB2A5BFF7D6994D4"
#define GMT8_P544_Y1		"AB457A64E140799D3D9B77ECEA828957F3AEADB3278FEE32E9AD77F068DD2F95B469607E8CE3CE5705276F869CFB18270C0A8FC8D8AD0D17545F4DCCC9E599853A7BAB76"
#define GMT8_P544_R		"FF0060739E18D7594A978B0AB6AE4CE3DBFD52A9D00197603FFFDF0000000101"
#define GMT8_P544_H		"8A0A079FC7C3D421A4986FAAA10A6593F60BC94E32AEE8067E35551B86D59F0F61BD6A750917746236DD39CB4710BD44B7BE86F0A8E5324DFE802973060F04BF37C018705B78072182D139A8D2BD338A630511676FDAFD47C60DB5393F9DFC96089B1FFB60B78FB2"
#define GMT8_P544_MAPU0 "0"
#define GMT8_P544_MAPU1 "1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 638
/** @{ */
#define BN_P638_A0		"0"
#define BN_P638_A1		"0"
#define BN_P638_B0		"2"
#define BN_P638_B1		"23FFFFFDC000000D7FFFFFB8000001D3FFFFF942D000165E3FFF94870000D52FFFFDD0E00008DE55C00086520021E55BFFFFF51FFFF4EB800000004C80015ACDFFFFFFFFFFFFECE00000000000000066"
#define BN_P638_X0		"C6BA9612456EFF0E3CD291C9C1A9116FB5EEF4992E052BC5C5126F0F55F67A7D190ED74C3D6229BC3D2F645328C94554AA032352A4D7D667542F793C8FEA25AD39606CA97025AA6EF16BAC2438B1DD3"
#define BN_P638_X1		"17BE713D379D46F3D77CFF94B7226EFFB4AD01CC67A8BA712DECB3FE8FFE58A027A45523200BF6FDA534F3F59763A1F6A6461F5D2DCAC172774C0CD24BA091A37B42C6E89A1E92F3B12E3B5AFFC222BB"
#define BN_P638_Y0		"E4197B30E3A9DD98A75E6C4D2C6561B6B96083E943230D578E944E2354482212ADAAA94CA54FC4A29D6CA873EFFB27C4B61B9B822C3C217D388C6C5D04C821F1A3A8A13A37C9807323AE9CAEDD021EC"
#define BN_P638_Y1		"1A650343ACEF6895FE4EC59B49F40E043DEB05DEF170DFD71B44CAB9496E2EADD034EC0E9238544556902D2D51AB93D224DC757AD720F4DE8ED3BFA4E22DB0ECE92369F681543F23A908A9B319D5FAEF"
#define BN_P638_R		"23FFFFFDC000000D7FFFFFB8000001D3FFFFF942D000165E3FFF94870000D52FFFFDD0E00008DE55600086550021E555FFFFF54FFFF4EAC000000049800154D9FFFFFFFFFFFFEDA00000000000000061"
#define BN_P638_H		"23FFFFFDC000000D7FFFFFB8000001D3FFFFF942D000165E3FFF94870000D52FFFFDD0E00008DE562000864F0021E561FFFFF4EFFFF4EC400000004F800160C1FFFFFFFFFFFFEC20000000000000006D"
#define BN_P638_MAPU0 "0"
#define BN_P638_MAPU1 "1"
/** @} */
#endif

#if defined(EP_ENDOM) && FP_PRIME == 638
/** @{ */
#define B12_P638_A0		"0"
#define B12_P638_A1		"0"
#define B12_P638_B0		"4"
#define B12_P638_B1		"4"
#define B12_P638_X0		"3C57F8F05130D336804E30CAA7BB45E06244DFC0BA836056B036038703719449A42CCC7C34452B4EE2DCA3CBCE0B7637E14E9CA88BDEF105440FB3F84AA95C75DE0BA05686394492B8648BB71D5E7F39"
#define B12_P638_X1		"07B30040203566584002D6DBB49A3DA1D99ECA3CBCD113C07E0CF1FFB3FA4F87F034A034C86F56DB380F2810AC329ED8BD6FE0F4D5C1FA26949739AF82D3AAD4702D2186862B0293E16C5EDDDDA3C922"
#define B12_P638_Y0		"29ED1A1C4F3F5AFC64AB2BA97CFA4D17998061179331A1C34E024B7D82134C60A3569F644E4155753C48698C8A01C80C0C3CEC9E3BDE2E5E22D81BBB514FD24DE186FEBA69B82E88809BFCCE51A1840F"
#define B12_P638_Y1		"3795191221DB4917EEE4B7B85BC7D7CA0C60E82116064463FED0892BA82ACECF905E6DB8083C5F589F04DB80E3203C1B2BEB52ACDED6DF96FC515F36761E7152AEED13369A504FE38C4FF93860B89550"
#define B12_P638_R		"50F94035FF4000FFFFFFFFFFF9406BFDC0040000000000000035FB801DFFBFFFFFFFFFFFFFFF401BFF80000000000000000000FFC01"
#define B12_P638_H		"2D88688DBA18275F5801BFFD4DDE93725697788C46C7B4BC8050639BA17EA2158B6784CCACDDECE490643943E5376D29C71C96B894056CCCC13C3DC6AAAAAA0F89601DC2979B3721C71C71C8B38CB8AEFEEB9E1C71C71C71C4B9FED17AE51B8E38E38E38E38FC954E8C65"
#define B12_P638_MAPU0 "0"
#define B12_P638_MAPU1 "1"
/** @} */
#endif

/**
 * Assigns a set of ordinary elliptic curve parameters.
 *
 * @param[in] CURVE		- the curve parameters to assign.
 */
#define ASSIGN(CURVE)														\
	RLC_GET(str, CURVE##_A0, sizeof(CURVE##_A0));							\
	fp_read_str(a[0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_A1, sizeof(CURVE##_A1));							\
	fp_read_str(a[1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B0, sizeof(CURVE##_B0));							\
	fp_read_str(b[0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_B1, sizeof(CURVE##_B1));							\
	fp_read_str(b[1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_X0, sizeof(CURVE##_X0));							\
	fp_read_str(g->x[0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_X1, sizeof(CURVE##_X1));							\
	fp_read_str(g->x[1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_Y0, sizeof(CURVE##_Y0));							\
	fp_read_str(g->y[0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_Y1, sizeof(CURVE##_Y1));							\
	fp_read_str(g->y[1], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_R, sizeof(CURVE##_R));								\
	bn_read_str(r, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_H, sizeof(CURVE##_H));								\
	bn_read_str(h, str, strlen(str), 16);									\
	RLC_GET(str, CURVE##_MAPU0, sizeof(CURVE##_MAPU0));						\
	fp_read_str(u[0], str, strlen(str), 16);								\
	RLC_GET(str, CURVE##_MAPU1, sizeof(CURVE##_MAPU1));						\
	fp_read_str(u[1], str, strlen(str), 16);

#if defined(EP_CTMAP)

/**
 * Assigns the isogeny map parameters for hashing with SSWU map.
 *
 * @param[in] CURVE		- the curve parameters to assign.
 */
#define ASSIGNM(CURVE)														\
    ep2_curve_set_ctmap(CURVE##_ISO_A0, CURVE##_ISO_A1, CURVE##_ISO_B0,		\
                        CURVE##_ISO_B1, CURVE##_ISO_XN, CURVE##_ISO_XD,		\
						CURVE##_ISO_YN, CURVE##_ISO_YD)

#endif /* EP_CTMAP */

#if defined(EP_CTMAP)
/**
 * Reads a sequence of polynomial coefficients from semicolon separated string.
 *
 * @param[out] coeffs		- the resulting coefficients.
 * @param[in] str			- the input string.
 */
static int ep2_curve_get_coeffs(fp2_t *coeffs, const char *str) {
	int degree = 0;
	unsigned offset = 0;

	if (str[0] == '\0') {
		/* need nonzero strlen */
		RLC_THROW(ERR_NO_VALID);
		return 0;
	}
	for (; degree < RLC_EPX_CTMAP_MAX; ++degree) {
		/* first coeff */
		const char *end = strchr(str + offset, ',');
		if (end == NULL) {
			/* should not happen --- means there's no second coeff */
			RLC_THROW(ERR_NO_VALID);
		}
		unsigned len = end - str - offset;
		fp_read_str(coeffs[degree][0], str + offset, len, 16);
		offset += len + 1; /* move to after ',' */

		/* second coeff */
		end = strchr(str + offset, ';');
		if (end == NULL) {
			/* last one */
			fp_read_str(coeffs[degree][1], str + offset, strlen(str + offset), 16);
			break;
		}
		len = end - str - offset;
		fp_read_str(coeffs[degree][1], str + offset, len, 16);
		offset += len + 1; /* move to after ';' */
	}
	if (degree == RLC_EPX_CTMAP_MAX) {
		/* ran out of space before converting all coeffs */
		RLC_THROW(ERR_NO_VALID);
	}
	return degree;
}

/**
 * Configures a constant-time hash-to-curve function based on an isogeny map.
 *
 * @param[in] a0_str			- the string representing the 1st element of the 'a' coefficient.
 * @param[in] a1_str			- the string representing the 2nd element of the 'a' coefficient.
 * @param[in] b0_str			- the string representing the 1st element of the 'b' coefficient.
 * @param[in] b1_str			- the string representing the 2nd element of the 'b' coefficient.
 * @param[in] xn_str			- the string representing the x numerator coefficients.
 * @param[in] xd_str			- the string representing the x denominator coefficients.
 * @param[in] yn_str			- the string representing the y numerator coefficients.
 * @param[in] yd_str			- the string representing the y denominator coefficients.
 */
/* declaring this function inline suppresses unused function warnings */
static inline void ep2_curve_set_ctmap(const char *a0_str, const char *a1_str,
									   const char *b0_str, const char *b1_str,
									   const char *xn_str, const char *xd_str,
									   const char *yn_str, const char *yd_str) {
	iso2_t iso = ep2_curve_get_iso();

	/* coefficients of isogenous curve */
	fp_read_str(iso->a[0], a0_str, strlen(a0_str), 16);
	fp_read_str(iso->a[1], a1_str, strlen(a1_str), 16);
	fp_read_str(iso->b[0], b0_str, strlen(b0_str), 16);
	fp_read_str(iso->b[1], b1_str, strlen(b1_str), 16);

	/* isogeny map coeffs */
	iso->deg_xn = ep2_curve_get_coeffs(iso->xn, xn_str);
	iso->deg_xd = ep2_curve_get_coeffs(iso->xd, xd_str);
	iso->deg_yn = ep2_curve_get_coeffs(iso->yn, yn_str);
	iso->deg_yd = ep2_curve_get_coeffs(iso->yd, yd_str);
}
#endif /* EP_CTMAP */

/**
 * Precomputes constants used by the ep2_map function.
 */
static void ep2_curve_set_map(void) {
	bn_t t;
	bn_null(t);

	const int abNeq0 = (ep2_curve_opt_a() != RLC_ZERO) && (ep2_curve_opt_b() != RLC_ZERO);

	ctx_t *ctx = core_get();
	fp_t *c1 = ctx->ep2_map_c[0];
	fp_t *c2 = ctx->ep2_map_c[1];
	fp_t *c3 = ctx->ep2_map_c[2];
	fp_t *c4 = ctx->ep2_map_c[3];

	RLC_TRY {
		bn_new(t);

		if (ep2_curve_is_ctmap() || abNeq0) {
			/* SSWU map constants */
			/* constants 3 and 4 are a and b for the curve or isogeny */
#ifdef EP_CTMAP
			if (ep2_curve_is_ctmap()) {
				fp2_copy(c3, ctx->ep2_iso.a);
				fp2_copy(c4, ctx->ep2_iso.b);
			} else {
#endif
				fp2_copy(c3, ctx->ep2_a);
				fp2_copy(c4, ctx->ep2_b);
#ifdef EP_CTMAP
			}
#endif
			/* constant 1: -b / a */
			fp2_neg(c1, c3);     /* c1 = -a */
			fp2_inv(c1, c1);     /* c1 = -1 / a */
			fp2_mul(c1, c1, c4); /* c1 = -b / a */

			/* constant 2 is unused in this case */
		} else {
			/* SvdW map constants */
			/* constant 1: g(u) = u^3 + a * u + b */
			fp2_sqr(c1, ctx->ep2_map_u);
			fp2_add(c1, c1, ctx->ep2_a);
			fp2_mul(c1, c1, ctx->ep2_map_u);
			fp2_add(c1, c1, ctx->ep2_b);

			/* constant 2: -u / 2 */
			fp2_set_dig(c2, 2);
			fp2_neg(c2, c2);                 /* -2 */
			fp2_inv(c2, c2);                 /* -1 / 2 */
			fp2_mul(c2, c2, ctx->ep2_map_u); /* -u / 2 */

			/* constant 3: sqrt(-g(u) * (3 * u^2 + 4 * a)) */
			fp2_sqr(c3, ctx->ep2_map_u);    /* u^2 */
			fp2_mul_dig(c3, c3, 3);         /* 3 * u^2 */
			fp2_mul_dig(c4, ctx->ep2_a, 4); /* 4 * a */
			fp2_add(c4, c3, c4);            /* 3 * u^2 + 4 * a */
			fp2_neg(c4, c4);                /* -(3 * u^2 + 4 * a) */
			fp2_mul(c3, c4, c1);            /* -g(u) * (3 * u^2 + 4 * a) */
			if (!fp2_srt(c3, c3)) {
				RLC_THROW(ERR_NO_VALID);
			}
			/* make sure sgn0(c3) == 0 */
			const int c30_z = fp_is_zero(c3[0]);
			fp_prime_back(t, c3[0]);
			const int c30_n = bn_get_bit(t, 0);
			fp_prime_back(t, c3[1]);
			const int c31_n = bn_get_bit(t, 0);
			if (c30_n | (c30_z & c31_n)) {
				/* set sgn0(c3) == 0 */
				fp2_neg(c3, c3);
			}

			/* constant 4: -4 * g(u) / (3 * u^2 + 4 * a) */
			fp2_inv(c4, c4);        /* -1 / (3 * u^2 + 4 * a */
			fp2_mul(c4, c4, c1);    /* -g(u) / (3 * u^2 + 4 * a) */
			fp2_mul_dig(c4, c4, 4); /* -4 * g(u) / (3 * u^2 + 4 * a) */
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		bn_free(t);
	}
}

/**
 * Detects an optimization based on the curve coefficients.
 */
static void detect_opt(int *opt, fp2_t a) {
	fp2_t t;
	fp2_null(t);

	RLC_TRY {
		fp2_new(t);
		fp2_set_dig(t, 3);
		fp2_neg(t, t);

		if (fp2_cmp(a, t) == RLC_EQ) {
			*opt = RLC_MIN3;
		} else if (fp2_is_zero(a)) {
			*opt = RLC_ZERO;
		} else if (fp2_cmp_dig(a, 1) == RLC_EQ) {
			*opt = RLC_ONE;
		} else if (fp2_cmp_dig(a, 2) == RLC_EQ) {
			*opt = RLC_TWO;
		} else if ((fp_bits(a[0]) <= RLC_DIG) && fp_is_zero(a[1])) {
			*opt = RLC_TINY;
		} else {
			*opt = RLC_HUGE;
		}
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		fp2_free(t);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep2_curve_init(void) {
	ctx_t *ctx = core_get();

#ifdef EP_PRECO
	for (int i = 0; i < RLC_EP_TABLE; i++) {
		ctx->ep2_ptr[i] = &(ctx->ep2_pre[i]);
	}
#endif

#if ALLOC == DYNAMIC
	ep2_new(ctx->ep2_g);
	fp2_new(ctx->ep2_a);
	fp2_new(ctx->ep2_b);
	fp2_new(ctx->ep2_map_u);
	for (unsigned i = 0; i < 4; ++i) {
		fp2_new(ctx->ep2_map_c[i]);
	}
	fp2_new(ctx->ep2_frb[0]);
	fp2_new(ctx->ep2_frb[1]);
#endif

#ifdef EP_PRECO
#if ALLOC == DYNAMIC
	for (int i = 0; i < RLC_EP_TABLE; i++) {
		fp2_new(ctx->ep2_pre[i].x);
		fp2_new(ctx->ep2_pre[i].y);
		fp2_new(ctx->ep2_pre[i].z);
	}
#endif
#endif
	ep2_set_infty(ctx->ep2_g);
	bn_make(&(ctx->ep2_r), RLC_FP_DIGS);
	bn_make(&(ctx->ep2_h), RLC_FP_DIGS);

#ifdef EP_CTMAP
	iso2_t iso = ep2_curve_get_iso();
#if ALLOC == DYNAMIC
	fp2_new(iso->a);
	fp2_new(iso->b);
	for (unsigned i = 0; i < RLC_EPX_CTMAP_MAX; ++i) {
		/* XXX(rsw) this doesn't result in contiguous layout for
		 * coeffs of a given polynomial. Is this worth fixing? */
		fp2_new(iso->xn[i]);
		fp2_new(iso->xd[i]);
		fp2_new(iso->yn[i]);
		fp2_new(iso->yd[i]);
	}
#else
	(void)iso; /* suppress unused warning when ALLOC == AUTO */
#endif /* ALLOC */
#endif /* EP_CTMAP */
#undef EP_CURVE_INIT_FP2
}

void ep2_curve_clean(void) {
	ctx_t *ctx = core_get();
	if (ctx != NULL) {
#ifdef EP_PRECO
		for (int i = 0; i < RLC_EP_TABLE; i++) {
			fp2_free(ctx->ep2_pre[i].x);
			fp2_free(ctx->ep2_pre[i].y);
			fp2_free(ctx->ep2_pre[i].z);
		}
#endif
		bn_clean(&(ctx->ep2_r));
		bn_clean(&(ctx->ep2_h));

#ifdef EP_CTMAP
		iso2_t iso = ep2_curve_get_iso();
		(void)iso; /* suppress unused warning when ALLOC == AUTO */
		fp2_free(iso->a);
		fp2_free(iso->b);
		for (unsigned i = 0; i < RLC_EPX_CTMAP_MAX; ++i) {
			fp2_free(iso->xn[i]);
			fp2_free(iso->xd[i]);
			fp2_free(iso->yn[i]);
			fp2_free(iso->yd[i]);
		}
#endif
		ep2_free(ctx->ep2_g);
		fp2_free(ctx->ep2_a);
		fp2_free(ctx->ep2_b);
		fp2_free(ctx->ep2_map_u);
		for (unsigned i = 0; i < 4; ++i) {
			fp2_free(ctx->ep2_map_c[i]);
		}
		fp2_free(ctx->ep2_frb[0]);
		fp2_free(ctx->ep2_frb[1]);
	}
}

int ep2_curve_opt_a(void) {
	return core_get()->ep2_opt_a;
}

int ep2_curve_opt_b(void) {
	return core_get()->ep2_opt_b;
}

int ep2_curve_is_twist(void) {
	return core_get()->ep2_is_twist;
}

int ep2_curve_is_ctmap(void) {
	return core_get()->ep2_is_ctmap;
}

void ep2_curve_get_gen(ep2_t g) {
	ep2_copy(g, core_get()->ep2_g);
}

fp_t *ep2_curve_get_a(void) {
	return core_get()->ep2_a;
}

fp_t *ep2_curve_get_b(void) {
	return core_get()->ep2_b;
}

void ep2_curve_get_vs(bn_t *v) {
	bn_t x, t;

	bn_null(x);
	bn_null(t);

	RLC_TRY {
		bn_new(x);
		bn_new(t);

		fp_prime_get_par(x);
		bn_copy(v[1], x);
		bn_copy(v[2], x);
		bn_copy(v[3], x);

		/* t = 2x^2. */
		bn_sqr(t, x);
		bn_dbl(t, t);

		/* v0 = 2x^2 + 3x + 1. */
		bn_mul_dig(v[0], x, 3);
		bn_add_dig(v[0], v[0], 1);
		bn_add(v[0], v[0], t);

		/* v3 = -(2x^2 + x). */
		bn_add(v[3], v[3], t);
		bn_neg(v[3], v[3]);

		/* v1 = 12x^3 + 8x^2 + x, v2 = 6x^3 + 4x^2 + x. */
		bn_dbl(t, t);
		bn_add(v[2], v[2], t);
		bn_dbl(t, t);
		bn_add(v[1], v[1], t);
		bn_rsh(t, t, 2);
		bn_mul(t, t, x);
		bn_mul_dig(t, t, 3);
		bn_add(v[2], v[2], t);
		bn_dbl(t, t);
		bn_add(v[1], v[1], t);
	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(x);
		bn_free(t);
	}
}

void ep2_curve_get_ord(bn_t n) {
	ctx_t *ctx = core_get();
	if (ctx->ep2_is_twist) {
		ep_curve_get_ord(n);
	} else {
		bn_copy(n, &(ctx->ep2_r));
	}
}

void ep2_curve_get_cof(bn_t h) {
	bn_copy(h, &(core_get()->ep2_h));
}

iso2_t ep2_curve_get_iso() {
#ifdef EP_CTMAP
	return &core_get()->ep2_iso;
#else
	return NULL;
#endif /* EP_CTMAP */
}

#if defined(EP_PRECO)

ep2_t *ep2_curve_get_tab(void) {
#if ALLOC == AUTO
	return (ep2_t *)*(core_get()->ep2_ptr);
#else
	return core_get()->ep2_ptr;
#endif
}

#endif

void ep2_curve_set_twist(int type) {
	int ctmap = 0;
	char str[4 * RLC_FP_BYTES + 1];
	ctx_t *ctx = core_get();
	ep2_t g;
	fp2_t a, b, u;
	bn_t r, h;

	ep2_null(g);
	fp2_null(a);
	fp2_null(b);
	fp2_null(u);
	bn_null(r);
	bn_null(h);

	ctx->ep2_is_twist = 0;
	if (type == RLC_EP_MTYPE || type == RLC_EP_DTYPE) {
		ctx->ep2_is_twist = type;
	} else {
		return;
	}

	RLC_TRY {
		ep2_new(g);
		fp2_new(a);
		fp2_new(b);
		fp2_new(u);
		bn_new(r);
		bn_new(h);

		switch (ep_param_get()) {
#if FP_PRIME == 158
			case BN_P158:
				ASSIGN(BN_P158);
				break;
#elif FP_PRIME == 254
			case BN_P254:
				ASSIGN(BN_P254);
				break;
#elif FP_PRIME == 256
			case BN_P256:
				ASSIGN(BN_P256);
				break;
			case SM9_P256:
				ASSIGN(SM9_P256);
				break;
#elif FP_PRIME == 381
			case B12_P381:
				ASSIGN(B12_P381);
#if defined(EP_CTMAP)
				ctmap = 1;
				ASSIGNM(B12_P381);
#endif /* EP_CTMAP */
				break;
#elif FP_PRIME == 382
			case BN_P382:
				ASSIGN(BN_P382);
				break;
#elif FP_PRIME == 383
			case B12_P383:
				ASSIGN(B12_P383);
				break;
#elif FP_PRIME == 446
			case BN_P446:
				ASSIGN(BN_P446);
				break;
			case B12_P446:
				ASSIGN(B12_P446);
				break;
#elif FP_PRIME == 455
			case B12_P455:
				ASSIGN(B12_P455);
				break;
#elif FP_PRIME == 511
			case OT8_P511:
				ASSIGN(OT8_P511);
				break;
#elif FP_PRIME == 544
			case GMT8_P544:
				ASSIGN(GMT8_P544);
				break;
#elif FP_PRIME == 638
			case BN_P638:
				ASSIGN(BN_P638);
				break;
			case B12_P638:
				ASSIGN(B12_P638);
				break;
#endif
			default:
				(void)str;
				RLC_THROW(ERR_NO_VALID);
				break;
		}

		fp2_zero(g->z);
		fp2_set_dig(g->z, 1);
		g->coord = BASIC;

		ep2_copy(ctx->ep2_g, g);
		fp2_copy(ctx->ep2_a, a);
		fp2_copy(ctx->ep2_b, b);

		detect_opt(&(ctx->ep2_opt_a), ctx->ep2_a);
		detect_opt(&(ctx->ep2_opt_b), ctx->ep2_b);

		fp2_copy(ctx->ep2_map_u, u);
		bn_copy(&(ctx->ep2_r), r);
		bn_copy(&(ctx->ep2_h), h);
		ctx->ep2_is_ctmap = ctmap;
		/* I don't have a better place for this. */
		fp_prime_calc();

		fp_copy(ctx->ep2_frb[0][0], ctx->fp2_p1[1][0]);
		fp_copy(ctx->ep2_frb[0][1], ctx->fp2_p1[1][1]);
		fp_copy(ctx->ep2_frb[1][0], ctx->fp2_p1[2][0]);
		fp_copy(ctx->ep2_frb[1][1], ctx->fp2_p1[2][1]);
		if (type == RLC_EP_MTYPE) {
			fp2_inv(ctx->ep2_frb[0], ctx->ep2_frb[0]);
			fp2_inv(ctx->ep2_frb[1], ctx->ep2_frb[1]);
		}

		/* compute constants for hash-to-curve */
		ep2_curve_set_map();

#if defined(WITH_PC)
		/* Compute pairing generator. */
		pc_core_calc();
#endif

#if defined(EP_PRECO)
		ep2_mul_pre((ep2_t *)ep2_curve_get_tab(), ctx->ep2_g);
#endif
	}
	RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	}
	RLC_FINALLY {
		ep2_free(g);
		fp2_free(a);
		fp2_free(b);
		fp2_free(u);
		bn_free(r);
		bn_free(h);
	}
}

void ep2_curve_set(fp2_t a, fp2_t b, ep2_t g, bn_t r, bn_t h) {
	ctx_t *ctx = core_get();
	ctx->ep2_is_twist = 0;

	fp2_copy(ctx->ep2_a, a);
	fp2_copy(ctx->ep2_b, b);

	ep2_norm(ctx->ep2_g, g);
	bn_copy(&(ctx->ep2_r), r);
	bn_copy(&(ctx->ep2_h), h);

#if defined(EP_PRECO)
	ep2_mul_pre((ep2_t *)ep2_curve_get_tab(), ctx->ep2_g);
#endif
}
