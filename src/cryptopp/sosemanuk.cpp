// sosemanuk.cpp - written and placed in the public domain by Wei Dai

// use "cl /EP /P /DCRYPTOPP_GENERATE_X64_MASM sosemanuk.cpp" to generate MASM code

#include "pch.h"
#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4702 4731)
#endif

#ifndef CRYPTOPP_GENERATE_X64_MASM

#include "sosemanuk.h"
#include "serpentp.h"
#include "secblock.h"
#include "misc.h"
#include "cpu.h"

NAMESPACE_BEGIN(CryptoPP)

void SosemanukPolicy::CipherSetKey(const NameValuePairs &params, const byte *userKey, size_t keylen)
{
	CRYPTOPP_UNUSED(params);
	Serpent_KeySchedule(m_key, 24, userKey, keylen);
}

void SosemanukPolicy::CipherResynchronize(byte *keystreamBuffer, const byte *iv, size_t length)
{
	CRYPTOPP_UNUSED(keystreamBuffer), CRYPTOPP_UNUSED(iv), CRYPTOPP_UNUSED(length);
	assert(length==16);

	word32 a, b, c, d, e;
	
	typedef BlockGetAndPut<word32, LittleEndian> Block;
	Block::Get(iv)(a)(b)(c)(d);

	const word32 *k = m_key;
	unsigned int i=1;

	do
	{
		beforeS0(KX); beforeS0(S0); afterS0(LT);
		afterS0(KX); afterS0(S1); afterS1(LT);
		if (i == 3)	// after 18th round
		{
			m_state[4] = b;
			m_state[5] = e;
			m_state[10] = c;
			m_state[11] = a;
		}
		afterS1(KX); afterS1(S2); afterS2(LT);
		afterS2(KX); afterS2(S3); afterS3(LT);
		if (i == 2)	// after 12th round
		{
			m_state[6] = c;
			m_state[7] = d;
			m_state[8] = b;
			m_state[9] = e;
		}
		afterS3(KX); afterS3(S4); afterS4(LT);
		afterS4(KX); afterS4(S5); afterS5(LT);
		afterS5(KX); afterS5(S6); afterS6(LT);
		afterS6(KX); afterS6(S7); afterS7(LT);

		if (i == 3)
			break;

		++i;
		c = b;
		b = e;
		e = d;
		d = a;
		a = e;
		k += 32;
	}
	while (true);

	afterS7(KX);

	m_state[0] = a;
	m_state[1] = b;
	m_state[2] = e;
	m_state[3] = d;

#define XMUX(c, x, y)   (x ^ (y & (0 - (c & 1))))
	m_state[11] += XMUX(m_state[10], m_state[1], m_state[8]);
	m_state[10] = rotlFixed(m_state[10] * 0x54655307, 7);
}

extern "C" {
word32 s_sosemanukMulTables[512] = {
#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64) && !defined(CRYPTOPP_DISABLE_SOSEMANUK_ASM)
	0x00000000, 0xE19FCF12, 0x6B973724, 0x8A08F836, 
	0xD6876E48, 0x3718A15A, 0xBD10596C, 0x5C8F967E, 
	0x05A7DC90, 0xE4381382, 0x6E30EBB4, 0x8FAF24A6, 
	0xD320B2D8, 0x32BF7DCA, 0xB8B785FC, 0x59284AEE, 
	0x0AE71189, 0xEB78DE9B, 0x617026AD, 0x80EFE9BF, 
	0xDC607FC1, 0x3DFFB0D3, 0xB7F748E5, 0x566887F7, 
	0x0F40CD19, 0xEEDF020B, 0x64D7FA3D, 0x8548352F, 
	0xD9C7A351, 0x38586C43, 0xB2509475, 0x53CF5B67, 
	0x146722BB, 0xF5F8EDA9, 0x7FF0159F, 0x9E6FDA8D, 
	0xC2E04CF3, 0x237F83E1, 0xA9777BD7, 0x48E8B4C5, 
	0x11C0FE2B, 0xF05F3139, 0x7A57C90F, 0x9BC8061D, 
	0xC7479063, 0x26D85F71, 0xACD0A747, 0x4D4F6855, 
	0x1E803332, 0xFF1FFC20, 0x75170416, 0x9488CB04, 
	0xC8075D7A, 0x29989268, 0xA3906A5E, 0x420FA54C, 
	0x1B27EFA2, 0xFAB820B0, 0x70B0D886, 0x912F1794, 
	0xCDA081EA, 0x2C3F4EF8, 0xA637B6CE, 0x47A879DC, 
	0x28CE44DF, 0xC9518BCD, 0x435973FB, 0xA2C6BCE9, 
	0xFE492A97, 0x1FD6E585, 0x95DE1DB3, 0x7441D2A1, 
	0x2D69984F, 0xCCF6575D, 0x46FEAF6B, 0xA7616079, 
	0xFBEEF607, 0x1A713915, 0x9079C123, 0x71E60E31, 
	0x22295556, 0xC3B69A44, 0x49BE6272, 0xA821AD60, 
	0xF4AE3B1E, 0x1531F40C, 0x9F390C3A, 0x7EA6C328, 
	0x278E89C6, 0xC61146D4, 0x4C19BEE2, 0xAD8671F0, 
	0xF109E78E, 0x1096289C, 0x9A9ED0AA, 0x7B011FB8, 
	0x3CA96664, 0xDD36A976, 0x573E5140, 0xB6A19E52, 
	0xEA2E082C, 0x0BB1C73E, 0x81B93F08, 0x6026F01A, 
	0x390EBAF4, 0xD89175E6, 0x52998DD0, 0xB30642C2, 
	0xEF89D4BC, 0x0E161BAE, 0x841EE398, 0x65812C8A, 
	0x364E77ED, 0xD7D1B8FF, 0x5DD940C9, 0xBC468FDB, 
	0xE0C919A5, 0x0156D6B7, 0x8B5E2E81, 0x6AC1E193, 
	0x33E9AB7D, 0xD276646F, 0x587E9C59, 0xB9E1534B, 
	0xE56EC535, 0x04F10A27, 0x8EF9F211, 0x6F663D03, 
	0x50358817, 0xB1AA4705, 0x3BA2BF33, 0xDA3D7021, 
	0x86B2E65F, 0x672D294D, 0xED25D17B, 0x0CBA1E69, 
	0x55925487, 0xB40D9B95, 0x3E0563A3, 0xDF9AACB1, 
	0x83153ACF, 0x628AF5DD, 0xE8820DEB, 0x091DC2F9, 
	0x5AD2999E, 0xBB4D568C, 0x3145AEBA, 0xD0DA61A8, 
	0x8C55F7D6, 0x6DCA38C4, 0xE7C2C0F2, 0x065D0FE0, 
	0x5F75450E, 0xBEEA8A1C, 0x34E2722A, 0xD57DBD38, 
	0x89F22B46, 0x686DE454, 0xE2651C62, 0x03FAD370, 
	0x4452AAAC, 0xA5CD65BE, 0x2FC59D88, 0xCE5A529A, 
	0x92D5C4E4, 0x734A0BF6, 0xF942F3C0, 0x18DD3CD2, 
	0x41F5763C, 0xA06AB92E, 0x2A624118, 0xCBFD8E0A, 
	0x97721874, 0x76EDD766, 0xFCE52F50, 0x1D7AE042, 
	0x4EB5BB25, 0xAF2A7437, 0x25228C01, 0xC4BD4313, 
	0x9832D56D, 0x79AD1A7F, 0xF3A5E249, 0x123A2D5B, 
	0x4B1267B5, 0xAA8DA8A7, 0x20855091, 0xC11A9F83, 
	0x9D9509FD, 0x7C0AC6EF, 0xF6023ED9, 0x179DF1CB, 
	0x78FBCCC8, 0x996403DA, 0x136CFBEC, 0xF2F334FE, 
	0xAE7CA280, 0x4FE36D92, 0xC5EB95A4, 0x24745AB6, 
	0x7D5C1058, 0x9CC3DF4A, 0x16CB277C, 0xF754E86E, 
	0xABDB7E10, 0x4A44B102, 0xC04C4934, 0x21D38626, 
	0x721CDD41, 0x93831253, 0x198BEA65, 0xF8142577, 
	0xA49BB309, 0x45047C1B, 0xCF0C842D, 0x2E934B3F, 
	0x77BB01D1, 0x9624CEC3, 0x1C2C36F5, 0xFDB3F9E7, 
	0xA13C6F99, 0x40A3A08B, 0xCAAB58BD, 0x2B3497AF, 
	0x6C9CEE73, 0x8D032161, 0x070BD957, 0xE6941645, 
	0xBA1B803B, 0x5B844F29, 0xD18CB71F, 0x3013780D, 
	0x693B32E3, 0x88A4FDF1, 0x02AC05C7, 0xE333CAD5, 
	0xBFBC5CAB, 0x5E2393B9, 0xD42B6B8F, 0x35B4A49D, 
	0x667BFFFA, 0x87E430E8, 0x0DECC8DE, 0xEC7307CC, 
	0xB0FC91B2, 0x51635EA0, 0xDB6BA696, 0x3AF46984, 
	0x63DC236A, 0x8243EC78, 0x084B144E, 0xE9D4DB5C, 
	0xB55B4D22, 0x54C48230, 0xDECC7A06, 0x3F53B514,
#else
	0x00000000, 0xE19FCF13, 0x6B973726, 0x8A08F835,
	0xD6876E4C, 0x3718A15F, 0xBD10596A, 0x5C8F9679,
	0x05A7DC98, 0xE438138B, 0x6E30EBBE, 0x8FAF24AD,
	0xD320B2D4, 0x32BF7DC7, 0xB8B785F2, 0x59284AE1,
	0x0AE71199, 0xEB78DE8A, 0x617026BF, 0x80EFE9AC,
	0xDC607FD5, 0x3DFFB0C6, 0xB7F748F3, 0x566887E0,
	0x0F40CD01, 0xEEDF0212, 0x64D7FA27, 0x85483534,
	0xD9C7A34D, 0x38586C5E, 0xB250946B, 0x53CF5B78,
	0x1467229B, 0xF5F8ED88, 0x7FF015BD, 0x9E6FDAAE,
	0xC2E04CD7, 0x237F83C4, 0xA9777BF1, 0x48E8B4E2,
	0x11C0FE03, 0xF05F3110, 0x7A57C925, 0x9BC80636,
	0xC747904F, 0x26D85F5C, 0xACD0A769, 0x4D4F687A,
	0x1E803302, 0xFF1FFC11, 0x75170424, 0x9488CB37,
	0xC8075D4E, 0x2998925D, 0xA3906A68, 0x420FA57B,
	0x1B27EF9A, 0xFAB82089, 0x70B0D8BC, 0x912F17AF,
	0xCDA081D6, 0x2C3F4EC5, 0xA637B6F0, 0x47A879E3,
	0x28CE449F, 0xC9518B8C, 0x435973B9, 0xA2C6BCAA,
	0xFE492AD3, 0x1FD6E5C0, 0x95DE1DF5, 0x7441D2E6,
	0x2D699807, 0xCCF65714, 0x46FEAF21, 0xA7616032,
	0xFBEEF64B, 0x1A713958, 0x9079C16D, 0x71E60E7E,
	0x22295506, 0xC3B69A15, 0x49BE6220, 0xA821AD33,
	0xF4AE3B4A, 0x1531F459, 0x9F390C6C, 0x7EA6C37F,
	0x278E899E, 0xC611468D, 0x4C19BEB8, 0xAD8671AB,
	0xF109E7D2, 0x109628C1, 0x9A9ED0F4, 0x7B011FE7,
	0x3CA96604, 0xDD36A917, 0x573E5122, 0xB6A19E31,
	0xEA2E0848, 0x0BB1C75B, 0x81B93F6E, 0x6026F07D,
	0x390EBA9C, 0xD891758F, 0x52998DBA, 0xB30642A9,
	0xEF89D4D0, 0x0E161BC3, 0x841EE3F6, 0x65812CE5,
	0x364E779D, 0xD7D1B88E, 0x5DD940BB, 0xBC468FA8,
	0xE0C919D1, 0x0156D6C2, 0x8B5E2EF7, 0x6AC1E1E4,
	0x33E9AB05, 0xD2766416, 0x587E9C23, 0xB9E15330,
	0xE56EC549, 0x04F10A5A, 0x8EF9F26F, 0x6F663D7C,
	0x50358897, 0xB1AA4784, 0x3BA2BFB1, 0xDA3D70A2,
	0x86B2E6DB, 0x672D29C8, 0xED25D1FD, 0x0CBA1EEE,
	0x5592540F, 0xB40D9B1C, 0x3E056329, 0xDF9AAC3A,
	0x83153A43, 0x628AF550, 0xE8820D65, 0x091DC276,
	0x5AD2990E, 0xBB4D561D, 0x3145AE28, 0xD0DA613B,
	0x8C55F742, 0x6DCA3851, 0xE7C2C064, 0x065D0F77,
	0x5F754596, 0xBEEA8A85, 0x34E272B0, 0xD57DBDA3,
	0x89F22BDA, 0x686DE4C9, 0xE2651CFC, 0x03FAD3EF,
	0x4452AA0C, 0xA5CD651F, 0x2FC59D2A, 0xCE5A5239,
	0x92D5C440, 0x734A0B53, 0xF942F366, 0x18DD3C75,
	0x41F57694, 0xA06AB987, 0x2A6241B2, 0xCBFD8EA1,
	0x977218D8, 0x76EDD7CB, 0xFCE52FFE, 0x1D7AE0ED,
	0x4EB5BB95, 0xAF2A7486, 0x25228CB3, 0xC4BD43A0,
	0x9832D5D9, 0x79AD1ACA, 0xF3A5E2FF, 0x123A2DEC,
	0x4B12670D, 0xAA8DA81E, 0x2085502B, 0xC11A9F38,
	0x9D950941, 0x7C0AC652, 0xF6023E67, 0x179DF174,
	0x78FBCC08, 0x9964031B, 0x136CFB2E, 0xF2F3343D,
	0xAE7CA244, 0x4FE36D57, 0xC5EB9562, 0x24745A71,
	0x7D5C1090, 0x9CC3DF83, 0x16CB27B6, 0xF754E8A5,
	0xABDB7EDC, 0x4A44B1CF, 0xC04C49FA, 0x21D386E9,
	0x721CDD91, 0x93831282, 0x198BEAB7, 0xF81425A4,
	0xA49BB3DD, 0x45047CCE, 0xCF0C84FB, 0x2E934BE8,
	0x77BB0109, 0x9624CE1A, 0x1C2C362F, 0xFDB3F93C,
	0xA13C6F45, 0x40A3A056, 0xCAAB5863, 0x2B349770,
	0x6C9CEE93, 0x8D032180, 0x070BD9B5, 0xE69416A6,
	0xBA1B80DF, 0x5B844FCC, 0xD18CB7F9, 0x301378EA,
	0x693B320B, 0x88A4FD18, 0x02AC052D, 0xE333CA3E,
	0xBFBC5C47, 0x5E239354, 0xD42B6B61, 0x35B4A472,
	0x667BFF0A, 0x87E43019, 0x0DECC82C, 0xEC73073F,
	0xB0FC9146, 0x51635E55, 0xDB6BA660, 0x3AF46973,
	0x63DC2392, 0x8243EC81, 0x084B14B4, 0xE9D4DBA7,
	0xB55B4DDE, 0x54C482CD, 0xDECC7AF8, 0x3F53B5EB,
#endif
	0x00000000, 0x180F40CD, 0x301E8033, 0x2811C0FE,
	0x603CA966, 0x7833E9AB, 0x50222955, 0x482D6998,
	0xC078FBCC, 0xD877BB01, 0xF0667BFF, 0xE8693B32,
	0xA04452AA, 0xB84B1267, 0x905AD299, 0x88559254,
	0x29F05F31, 0x31FF1FFC, 0x19EEDF02, 0x01E19FCF,
	0x49CCF657, 0x51C3B69A, 0x79D27664, 0x61DD36A9,
	0xE988A4FD, 0xF187E430, 0xD99624CE, 0xC1996403,
	0x89B40D9B, 0x91BB4D56, 0xB9AA8DA8, 0xA1A5CD65,
	0x5249BE62, 0x4A46FEAF, 0x62573E51, 0x7A587E9C,
	0x32751704, 0x2A7A57C9, 0x026B9737, 0x1A64D7FA,
	0x923145AE, 0x8A3E0563, 0xA22FC59D, 0xBA208550,
	0xF20DECC8, 0xEA02AC05, 0xC2136CFB, 0xDA1C2C36,
	0x7BB9E153, 0x63B6A19E, 0x4BA76160, 0x53A821AD,
	0x1B854835, 0x038A08F8, 0x2B9BC806, 0x339488CB,
	0xBBC11A9F, 0xA3CE5A52, 0x8BDF9AAC, 0x93D0DA61,
	0xDBFDB3F9, 0xC3F2F334, 0xEBE333CA, 0xF3EC7307,
	0xA492D5C4, 0xBC9D9509, 0x948C55F7, 0x8C83153A,
	0xC4AE7CA2, 0xDCA13C6F, 0xF4B0FC91, 0xECBFBC5C,
	0x64EA2E08, 0x7CE56EC5, 0x54F4AE3B, 0x4CFBEEF6,
	0x04D6876E, 0x1CD9C7A3, 0x34C8075D, 0x2CC74790,
	0x8D628AF5, 0x956DCA38, 0xBD7C0AC6, 0xA5734A0B,
	0xED5E2393, 0xF551635E, 0xDD40A3A0, 0xC54FE36D,
	0x4D1A7139, 0x551531F4, 0x7D04F10A, 0x650BB1C7,
	0x2D26D85F, 0x35299892, 0x1D38586C, 0x053718A1,
	0xF6DB6BA6, 0xEED42B6B, 0xC6C5EB95, 0xDECAAB58,
	0x96E7C2C0, 0x8EE8820D, 0xA6F942F3, 0xBEF6023E,
	0x36A3906A, 0x2EACD0A7, 0x06BD1059, 0x1EB25094,
	0x569F390C, 0x4E9079C1, 0x6681B93F, 0x7E8EF9F2,
	0xDF2B3497, 0xC724745A, 0xEF35B4A4, 0xF73AF469,
	0xBF179DF1, 0xA718DD3C, 0x8F091DC2, 0x97065D0F,
	0x1F53CF5B, 0x075C8F96, 0x2F4D4F68, 0x37420FA5,
	0x7F6F663D, 0x676026F0, 0x4F71E60E, 0x577EA6C3,
	0xE18D0321, 0xF98243EC, 0xD1938312, 0xC99CC3DF,
	0x81B1AA47, 0x99BEEA8A, 0xB1AF2A74, 0xA9A06AB9,
	0x21F5F8ED, 0x39FAB820, 0x11EB78DE, 0x09E43813,
	0x41C9518B, 0x59C61146, 0x71D7D1B8, 0x69D89175,
	0xC87D5C10, 0xD0721CDD, 0xF863DC23, 0xE06C9CEE,
	0xA841F576, 0xB04EB5BB, 0x985F7545, 0x80503588,
	0x0805A7DC, 0x100AE711, 0x381B27EF, 0x20146722,
	0x68390EBA, 0x70364E77, 0x58278E89, 0x4028CE44,
	0xB3C4BD43, 0xABCBFD8E, 0x83DA3D70, 0x9BD57DBD,
	0xD3F81425, 0xCBF754E8, 0xE3E69416, 0xFBE9D4DB,
	0x73BC468F, 0x6BB30642, 0x43A2C6BC, 0x5BAD8671,
	0x1380EFE9, 0x0B8FAF24, 0x239E6FDA, 0x3B912F17,
	0x9A34E272, 0x823BA2BF, 0xAA2A6241, 0xB225228C,
	0xFA084B14, 0xE2070BD9, 0xCA16CB27, 0xD2198BEA,
	0x5A4C19BE, 0x42435973, 0x6A52998D, 0x725DD940,
	0x3A70B0D8, 0x227FF015, 0x0A6E30EB, 0x12617026,
	0x451FD6E5, 0x5D109628, 0x750156D6, 0x6D0E161B,
	0x25237F83, 0x3D2C3F4E, 0x153DFFB0, 0x0D32BF7D,
	0x85672D29, 0x9D686DE4, 0xB579AD1A, 0xAD76EDD7,
	0xE55B844F, 0xFD54C482, 0xD545047C, 0xCD4A44B1,
	0x6CEF89D4, 0x74E0C919, 0x5CF109E7, 0x44FE492A,
	0x0CD320B2, 0x14DC607F, 0x3CCDA081, 0x24C2E04C,
	0xAC977218, 0xB49832D5, 0x9C89F22B, 0x8486B2E6,
	0xCCABDB7E, 0xD4A49BB3, 0xFCB55B4D, 0xE4BA1B80,
	0x17566887, 0x0F59284A, 0x2748E8B4, 0x3F47A879,
	0x776AC1E1, 0x6F65812C, 0x477441D2, 0x5F7B011F,
	0xD72E934B, 0xCF21D386, 0xE7301378, 0xFF3F53B5,
	0xB7123A2D, 0xAF1D7AE0, 0x870CBA1E, 0x9F03FAD3,
	0x3EA637B6, 0x26A9777B, 0x0EB8B785, 0x16B7F748,
	0x5E9A9ED0, 0x4695DE1D, 0x6E841EE3, 0x768B5E2E,
	0xFEDECC7A, 0xE6D18CB7, 0xCEC04C49, 0xD6CF0C84,
	0x9EE2651C, 0x86ED25D1, 0xAEFCE52F, 0xB6F3A5E2
};
}

#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64) && !defined(CRYPTOPP_DISABLE_SOSEMANUK_ASM)
unsigned int SosemanukPolicy::GetAlignment() const
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && !defined(CRYPTOPP_DISABLE_SOSEMANUK_ASM)
#ifdef __INTEL_COMPILER
	if (HasSSE2() && !IsP4())	// Intel compiler produces faster code for this algorithm on the P4
#else
	if (HasSSE2())
#endif
		return 16;
	else
#endif
		return GetAlignmentOf<word32>();
}

unsigned int SosemanukPolicy::GetOptimalBlockSize() const
{
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && !defined(CRYPTOPP_DISABLE_SOSEMANUK_ASM)
#ifdef __INTEL_COMPILER
	if (HasSSE2() && !IsP4())	// Intel compiler produces faster code for this algorithm on the P4
#else
	if (HasSSE2())
#endif
		return 4*BYTES_PER_ITERATION;
	else
#endif
		return BYTES_PER_ITERATION;
}
#endif

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
extern "C" {
void Sosemanuk_OperateKeystream(size_t iterationCount, const byte *input, byte *output, word32 *state);
}
#endif

void SosemanukPolicy::OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount)
{
#endif	// #ifdef CRYPTOPP_GENERATE_X64_MASM

#ifdef CRYPTOPP_X64_MASM_AVAILABLE
	Sosemanuk_OperateKeystream(iterationCount, input, output, m_state.data());
	return;
#endif

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE && !defined(CRYPTOPP_DISABLE_SOSEMANUK_ASM)
#ifdef CRYPTOPP_GENERATE_X64_MASM
		ALIGN   8
	Sosemanuk_OperateKeystream	PROC FRAME
		rex_push_reg rsi
		push_reg rdi
		alloc_stack(80*4*2+12*4+8*WORD_SZ + 2*16+8)
		save_xmm128 xmm6, 02f0h
		save_xmm128 xmm7, 0300h
		.endprolog
		mov		rdi, r8
		mov		rax, r9
#else
#ifdef __INTEL_COMPILER
	if (HasSSE2() && !IsP4())	// Intel compiler produces faster code for this algorithm on the P4
#else
	if (HasSSE2())
#endif
	{
#ifdef __GNUC__
	#if CRYPTOPP_BOOL_X64
		FixedSizeAlignedSecBlock<byte, 80*4*2+12*4+8*WORD_SZ> workspace;
	#endif
		__asm__ __volatile__
		(
		INTEL_NOPREFIX
		AS_PUSH_IF86(	bx)
#else
		word32 *state = m_state;
		AS2(	mov		WORD_REG(ax), state)
		AS2(	mov		WORD_REG(di), output)
		AS2(	mov		WORD_REG(dx), input)
		AS2(	mov		WORD_REG(cx), iterationCount)
#endif
#endif	// #ifdef CRYPTOPP_GENERATE_X64_MASM

#if defined(__GNUC__) && CRYPTOPP_BOOL_X64
	#define SSE2_workspace %5
#else
	#define SSE2_workspace WORD_REG(sp)
#endif

#define SSE2_output			WORD_PTR [SSE2_workspace+1*WORD_SZ]
#define SSE2_input			WORD_PTR [SSE2_workspace+2*WORD_SZ]
#define SSE2_wordsLeft		WORD_PTR [SSE2_workspace+3*WORD_SZ]
#define SSE2_diEnd			WORD_PTR [SSE2_workspace+4*WORD_SZ]
#define SSE2_pMulTables		WORD_PTR [SSE2_workspace+5*WORD_SZ]
#define SSE2_state			WORD_PTR [SSE2_workspace+6*WORD_SZ]
#define SSE2_wordsLeft2		WORD_PTR [SSE2_workspace+7*WORD_SZ]
#define SSE2_stateCopy		SSE2_workspace + 8*WORD_SZ
#define	SSE2_uvStart		SSE2_stateCopy + 12*4

#if (CRYPTOPP_BOOL_X86) && !defined(CRYPTOPP_DISABLE_SOSEMANUK_ASM)
		AS_PUSH_IF86(	bp)
		AS2(	mov		AS_REG_6, esp)
		AS2(	and		esp, -16)
		AS2(	sub		esp, 80*4*2+12*4+8*WORD_SZ)	// 80 v's, 80 u's, 12 state, 8 locals
		AS2(	mov		[esp], AS_REG_6)
#endif
		AS2(	mov		SSE2_output, WORD_REG(di))
		AS2(	mov		SSE2_input, WORD_REG(dx))
		AS2(	mov		SSE2_state, WORD_REG(ax))
#ifndef _MSC_VER
		AS2(	mov		SSE2_pMulTables, WORD_REG(si))
#endif
		AS2(	lea		WORD_REG(cx), [4*WORD_REG(cx)+WORD_REG(cx)])
		AS2(	lea		WORD_REG(si), [4*WORD_REG(cx)])
		AS2(	mov		SSE2_wordsLeft, WORD_REG(si))
		AS2(	movdqa	xmm0, [WORD_REG(ax)+0*16])		// copy state to stack to save a register
		AS2(	movdqa	[SSE2_stateCopy+0*16], xmm0)
		AS2(	movdqa	xmm0, [WORD_REG(ax)+1*16])
		AS2(	movdqa	[SSE2_stateCopy+1*16], xmm0)
		AS2(	movq	xmm0, QWORD PTR [WORD_REG(ax)+2*16])
		AS2(	movq	QWORD PTR [SSE2_stateCopy+2*16], xmm0)
		AS2(	psrlq	xmm0, 32)
		AS2(	movd	AS_REG_6d, xmm0)				// s(9)
		AS2(	mov		ecx, [WORD_REG(ax)+10*4])
		AS2(	mov		edx, [WORD_REG(ax)+11*4])
		AS2(	pcmpeqb	xmm7, xmm7)				// all ones

#define s(i)	SSE2_stateCopy + ASM_MOD(i,10)*4
#define u(j)	WORD_REG(di) + (ASM_MOD(j,4)*20 + (j/4)) * 4
#define v(j)	WORD_REG(di) + (ASM_MOD(j,4)*20 + (j/4)) * 4 + 80*4

#define R10 ecx
#define R11 edx
#define R20 edx
#define R21 ecx
// workaround bug in GAS 2.15
#define R20r WORD_REG(dx)
#define R21r WORD_REG(cx)

#define SSE2_STEP(i, j)	\
	AS2(	mov		eax, [s(i+0)])\
	AS2(	mov		[v(i)], eax)\
	AS2(	rol		eax, 8)\
	AS2(	lea		AS_REG_7, [AS_REG_6 + R2##j##r])\
	AS2(	xor		AS_REG_7d, R1##j)\
	AS2(	mov		[u(i)], AS_REG_7d)\
	AS2(	mov		AS_REG_7d, 1)\
	AS2(	and		AS_REG_7d, R2##j)\
	AS1(	neg		AS_REG_7d)\
	AS2(	and		AS_REG_7d, AS_REG_6d)\
	AS2(	xor		AS_REG_6d, eax)\
	AS2(	movzx	eax, al)\
	AS2(	xor		AS_REG_6d, [WORD_REG(si)+WORD_REG(ax)*4])\
	AS2(	mov		eax, [s(i+3)])\
	AS2(	xor		AS_REG_7d, [s(i+2)])\
	AS2(	add		R1##j, AS_REG_7d)\
	AS2(	movzx	AS_REG_7d, al)\
	AS2(	shr		eax, 8)\
	AS2(	xor		AS_REG_6d, [WORD_REG(si)+1024+AS_REG_7*4])\
	AS2(	xor		AS_REG_6d, eax)\
	AS2(	imul	R2##j, AS_HEX(54655307))\
	AS2(	rol		R2##j, 7)\
	AS2(	mov		[s(i+0)], AS_REG_6d)\

		ASL(2)	// outer loop, each iteration of this processes 80 words
		AS2(	lea		WORD_REG(di), [SSE2_uvStart])	// start of v and u
		AS2(	mov		WORD_REG(ax), 80)
		AS2(	cmp		WORD_REG(si), 80)
		AS2(	cmovg	WORD_REG(si), WORD_REG(ax))
		AS2(	mov		SSE2_wordsLeft2, WORD_REG(si))
		AS2(	lea		WORD_REG(si), [WORD_REG(di)+WORD_REG(si)])		// use to end first inner loop
		AS2(	mov		SSE2_diEnd, WORD_REG(si))
#ifdef _MSC_VER
		AS2(	lea		WORD_REG(si), s_sosemanukMulTables)
#else
		AS2(	mov		WORD_REG(si), SSE2_pMulTables)
#endif

		ASL(0)	// first inner loop, 20 words each, 4 iterations
		SSE2_STEP(0, 0)
		SSE2_STEP(1, 1)
		SSE2_STEP(2, 0)
		SSE2_STEP(3, 1)
		SSE2_STEP(4, 0)
		SSE2_STEP(5, 1)
		SSE2_STEP(6, 0)
		SSE2_STEP(7, 1)
		SSE2_STEP(8, 0)
		SSE2_STEP(9, 1)
		SSE2_STEP(10, 0)
		SSE2_STEP(11, 1)
		SSE2_STEP(12, 0)
		SSE2_STEP(13, 1)
		SSE2_STEP(14, 0)
		SSE2_STEP(15, 1)
		SSE2_STEP(16, 0)
		SSE2_STEP(17, 1)
		SSE2_STEP(18, 0)
		SSE2_STEP(19, 1)
		// loop
		AS2(	add		WORD_REG(di), 5*4)
		AS2(	cmp		WORD_REG(di), SSE2_diEnd)
		ASJ(	jne,	0, b)

		AS2(	mov		WORD_REG(ax), SSE2_input)
		AS2(	mov		AS_REG_7, SSE2_output)
		AS2(	lea		WORD_REG(di), [SSE2_uvStart])		// start of v and u
		AS2(	mov		WORD_REG(si), SSE2_wordsLeft2)

		ASL(1)	// second inner loop, 16 words each, 5 iterations
		AS2(	movdqa	xmm0, [WORD_REG(di)+0*20*4])
		AS2(	movdqa	xmm2, [WORD_REG(di)+2*20*4])
		AS2(	movdqa	xmm3, [WORD_REG(di)+3*20*4])
		AS2(	movdqa	xmm1, [WORD_REG(di)+1*20*4])
		// S2
		AS2(	movdqa	xmm4, xmm0)
		AS2(	pand	xmm0, xmm2)
		AS2(    pxor	xmm0, xmm3)
		AS2(    pxor	xmm2, xmm1)
 		AS2(	pxor	xmm2, xmm0)
 		AS2(	por		xmm3, xmm4)
 		AS2(	pxor	xmm3, xmm1)
 		AS2(	pxor	xmm4, xmm2)
 		AS2(	movdqa	xmm1, xmm3)
 		AS2(	por		xmm3, xmm4)
 		AS2(	pxor	xmm3, xmm0)
 		AS2(	pand	xmm0, xmm1)
 		AS2(	pxor	xmm4, xmm0)
 		AS2(	pxor	xmm1, xmm3)
 		AS2(	pxor	xmm1, xmm4)
		AS2(	pxor	xmm4, xmm7)
		// xor with v
		AS2(	pxor	xmm2, [WORD_REG(di)+80*4])
		AS2(	pxor	xmm3, [WORD_REG(di)+80*5])
		AS2(	pxor	xmm1, [WORD_REG(di)+80*6])
		AS2(	pxor	xmm4, [WORD_REG(di)+80*7])
		// exit loop early if less than 16 words left to output
		// this is necessary because block size is 20 words, and we output 16 words in each iteration of this loop
		AS2(	cmp		WORD_REG(si), 16)
		ASJ(	jl,		4, f)
		// unpack
		AS2(	movdqa		xmm6, xmm2)
		AS2(	punpckldq	xmm2, xmm3)
		AS2(	movdqa		xmm5, xmm1)
		AS2(	punpckldq	xmm1, xmm4)
		AS2(	movdqa		xmm0, xmm2)
		AS2(	punpcklqdq	xmm2, xmm1)
		AS2(	punpckhqdq	xmm0, xmm1)
		AS2(	punpckhdq	xmm6, xmm3)
		AS2(	punpckhdq	xmm5, xmm4)
		AS2(	movdqa		xmm3, xmm6)
		AS2(	punpcklqdq	xmm6, xmm5)
		AS2(	punpckhqdq	xmm3, xmm5)

		// output keystream
		AS_XMM_OUTPUT4(SSE2_Sosemanuk_Output, WORD_REG(ax), AS_REG_7, 2,0,6,3, 1, 0,1,2,3, 4)

		// loop
		AS2(	add		WORD_REG(di), 4*4)
		AS2(	sub		WORD_REG(si), 16)
		ASJ(	jnz,	1, b)

		// outer loop
		AS2(	mov		WORD_REG(si), SSE2_wordsLeft)
		AS2(	sub		WORD_REG(si), 80)
		ASJ(	jz,		6, f)
		AS2(	mov		SSE2_wordsLeft, WORD_REG(si))
		AS2(	mov		SSE2_input, WORD_REG(ax))
		AS2(	mov		SSE2_output, AS_REG_7)
		ASJ(	jmp,	2, b)

		ASL(4)	// final output of less than 16 words
		AS2(	test	WORD_REG(ax), WORD_REG(ax))
		ASJ(	jz,		5, f)
		AS2(	movd	xmm0, dword ptr [WORD_REG(ax)+0*4])
		AS2(	pxor	xmm2, xmm0)
		AS2(	movd	xmm0, dword ptr [WORD_REG(ax)+1*4])
		AS2(	pxor	xmm3, xmm0)
		AS2(	movd	xmm0, dword ptr [WORD_REG(ax)+2*4])
		AS2(	pxor	xmm1, xmm0)
		AS2(	movd	xmm0, dword ptr [WORD_REG(ax)+3*4])
		AS2(	pxor	xmm4, xmm0)
		AS2(	add		WORD_REG(ax), 16)
		ASL(5)
		AS2(	movd	dword ptr [AS_REG_7+0*4], xmm2)
		AS2(	movd	dword ptr [AS_REG_7+1*4], xmm3)
		AS2(	movd	dword ptr [AS_REG_7+2*4], xmm1)
		AS2(	movd	dword ptr [AS_REG_7+3*4], xmm4)
		AS2(	sub		WORD_REG(si), 4)
		ASJ(	jz,		6, f)
		AS2(	add		AS_REG_7, 16)
		AS2(	psrldq	xmm2, 4)
		AS2(	psrldq	xmm3, 4)
		AS2(	psrldq	xmm1, 4)
		AS2(	psrldq	xmm4, 4)
		ASJ(	jmp,	4, b)

		ASL(6)	// save state
		AS2(	mov		AS_REG_6, SSE2_state)
		AS2(	movdqa	xmm0, [SSE2_stateCopy+0*16])
		AS2(	movdqa	[AS_REG_6+0*16], xmm0)
		AS2(	movdqa	xmm0, [SSE2_stateCopy+1*16])
		AS2(	movdqa	[AS_REG_6+1*16], xmm0)
		AS2(	movq	xmm0, QWORD PTR [SSE2_stateCopy+2*16])
		AS2(	movq	QWORD PTR [AS_REG_6+2*16], xmm0)
		AS2(	mov		[AS_REG_6+10*4], ecx)
		AS2(	mov		[AS_REG_6+11*4], edx)

		AS_POP_IF86(	sp)
		AS_POP_IF86(	bp)

#ifdef __GNUC__
		AS_POP_IF86(	bx)
		ATT_PREFIX
			:
			: "a" (m_state.m_ptr), "c" (iterationCount), "S" (s_sosemanukMulTables), "D" (output), "d" (input)
	#if CRYPTOPP_BOOL_X64
			, "r" (workspace.m_ptr)
			: "memory", "cc", "%r9", "%r10", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"
	#else
			: "memory", "cc"
	#endif
		);
#endif
#ifdef CRYPTOPP_GENERATE_X64_MASM
	movdqa	xmm6, [rsp + 02f0h]
	movdqa	xmm7, [rsp + 0300h]
	add		rsp, 80*4*2+12*4+8*WORD_SZ + 2*16+8
	pop		rdi
	pop		rsi
	ret
	Sosemanuk_OperateKeystream ENDP
#else
	}
	else
#endif
#endif
#ifndef CRYPTOPP_GENERATE_X64_MASM
	{
#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64) && !defined(CRYPTOPP_DISABLE_SOSEMANUK_ASM)
#define MUL_A(x)    (x = rotlFixed(x, 8), x ^ s_sosemanukMulTables[byte(x)])
#else
#define MUL_A(x)    (((x) << 8) ^ s_sosemanukMulTables[(x) >> 24])
#endif

#define DIV_A(x)    (((x) >> 8) ^ s_sosemanukMulTables[256 + byte(x)])

#define r1(i) ((i%2) ? reg2 : reg1)
#define r2(i) ((i%2) ? reg1 : reg2)

#define STEP(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, v, u)	\
		u = (s##x9 + r2(x0)) ^ r1(x0);\
		v = s##x0;\
		s##x0 = MUL_A(s##x0) ^ DIV_A(s##x3) ^ s##x9;\
		r1(x0) += XMUX(r2(x0), s##x2, s##x9);\
		r2(x0) = rotlFixed(r2(x0) * 0x54655307, 7);\

#define SOSEMANUK_OUTPUT(x)	\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 0, u2 ^ v0);\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 1, u3 ^ v1);\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 2, u1 ^ v2);\
	CRYPTOPP_KEYSTREAM_OUTPUT_WORD(x, LITTLE_ENDIAN_ORDER, 3, u4 ^ v3);

#define OUTPUT4	\
	S2(0, u0, u1, u2, u3, u4);\
	CRYPTOPP_KEYSTREAM_OUTPUT_SWITCH(SOSEMANUK_OUTPUT, 4*4);

	word32 s0 = m_state[0];
	word32 s1 = m_state[1];
	word32 s2 = m_state[2];
	word32 s3 = m_state[3];
	word32 s4 = m_state[4];
	word32 s5 = m_state[5];
	word32 s6 = m_state[6];
	word32 s7 = m_state[7];
	word32 s8 = m_state[8];
	word32 s9 = m_state[9];
	word32 reg1 = m_state[10];
	word32 reg2 = m_state[11];
	word32 u0, u1, u2, u3, u4, v0, v1, v2, v3;

	do
	{
		STEP(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, v0, u0)
		STEP(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, v1, u1)
		STEP(2, 3, 4, 5, 6, 7, 8, 9, 0, 1, v2, u2)
		STEP(3, 4, 5, 6, 7, 8, 9, 0, 1, 2, v3, u3)
		OUTPUT4
		STEP(4, 5, 6, 7, 8, 9, 0, 1, 2, 3, v0, u0)
		STEP(5, 6, 7, 8, 9, 0, 1, 2, 3, 4, v1, u1)
		STEP(6, 7, 8, 9, 0, 1, 2, 3, 4, 5, v2, u2)
		STEP(7, 8, 9, 0, 1, 2, 3, 4, 5, 6, v3, u3)
		OUTPUT4
		STEP(8, 9, 0, 1, 2, 3, 4, 5, 6, 7, v0, u0)
		STEP(9, 0, 1, 2, 3, 4, 5, 6, 7, 8, v1, u1)
		STEP(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, v2, u2)
		STEP(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, v3, u3)
		OUTPUT4
		STEP(2, 3, 4, 5, 6, 7, 8, 9, 0, 1, v0, u0)
		STEP(3, 4, 5, 6, 7, 8, 9, 0, 1, 2, v1, u1)
		STEP(4, 5, 6, 7, 8, 9, 0, 1, 2, 3, v2, u2)
		STEP(5, 6, 7, 8, 9, 0, 1, 2, 3, 4, v3, u3)
		OUTPUT4
		STEP(6, 7, 8, 9, 0, 1, 2, 3, 4, 5, v0, u0)
		STEP(7, 8, 9, 0, 1, 2, 3, 4, 5, 6, v1, u1)
		STEP(8, 9, 0, 1, 2, 3, 4, 5, 6, 7, v2, u2)
		STEP(9, 0, 1, 2, 3, 4, 5, 6, 7, 8, v3, u3)
		OUTPUT4
	}
	while (--iterationCount);

	m_state[0] = s0;
	m_state[1] = s1;
	m_state[2] = s2;
	m_state[3] = s3;
	m_state[4] = s4;
	m_state[5] = s5;
	m_state[6] = s6;
	m_state[7] = s7;
	m_state[8] = s8;
	m_state[9] = s9;
	m_state[10] = reg1;
	m_state[11] = reg2;
	}
}

NAMESPACE_END

#endif // #ifndef CRYPTOPP_GENERATE_X64_MASM
