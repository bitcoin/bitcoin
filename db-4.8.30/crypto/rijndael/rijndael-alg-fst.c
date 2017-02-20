/**
 * rijndael-alg-fst.c
 *
 * @version 3.0 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"

#include "crypto/rijndael/rijndael-alg-fst.h"

/*
Te0[x] = S [x].[02, 01, 01, 03];
Te1[x] = S [x].[03, 02, 01, 01];
Te2[x] = S [x].[01, 03, 02, 01];
Te3[x] = S [x].[01, 01, 03, 02];
Te4[x] = S [x].[01, 01, 01, 01];

Td0[x] = Si[x].[0e, 09, 0d, 0b];
Td1[x] = Si[x].[0b, 0e, 09, 0d];
Td2[x] = Si[x].[0d, 0b, 0e, 09];
Td3[x] = Si[x].[09, 0d, 0b, 0e];
Td4[x] = Si[x].[01, 01, 01, 01];
*/

static const u32 Te0[256] = {
    (u_int)0xc66363a5, (u_int)0xf87c7c84, (u_int)0xee777799, (u_int)0xf67b7b8d,
    (u_int)0xfff2f20d, (u_int)0xd66b6bbd, (u_int)0xde6f6fb1, (u_int)0x91c5c554,
    (u_int)0x60303050, (u_int)0x02010103, (u_int)0xce6767a9, (u_int)0x562b2b7d,
    (u_int)0xe7fefe19, (u_int)0xb5d7d762, (u_int)0x4dababe6, (u_int)0xec76769a,
    (u_int)0x8fcaca45, (u_int)0x1f82829d, (u_int)0x89c9c940, (u_int)0xfa7d7d87,
    (u_int)0xeffafa15, (u_int)0xb25959eb, (u_int)0x8e4747c9, (u_int)0xfbf0f00b,
    (u_int)0x41adadec, (u_int)0xb3d4d467, (u_int)0x5fa2a2fd, (u_int)0x45afafea,
    (u_int)0x239c9cbf, (u_int)0x53a4a4f7, (u_int)0xe4727296, (u_int)0x9bc0c05b,
    (u_int)0x75b7b7c2, (u_int)0xe1fdfd1c, (u_int)0x3d9393ae, (u_int)0x4c26266a,
    (u_int)0x6c36365a, (u_int)0x7e3f3f41, (u_int)0xf5f7f702, (u_int)0x83cccc4f,
    (u_int)0x6834345c, (u_int)0x51a5a5f4, (u_int)0xd1e5e534, (u_int)0xf9f1f108,
    (u_int)0xe2717193, (u_int)0xabd8d873, (u_int)0x62313153, (u_int)0x2a15153f,
    (u_int)0x0804040c, (u_int)0x95c7c752, (u_int)0x46232365, (u_int)0x9dc3c35e,
    (u_int)0x30181828, (u_int)0x379696a1, (u_int)0x0a05050f, (u_int)0x2f9a9ab5,
    (u_int)0x0e070709, (u_int)0x24121236, (u_int)0x1b80809b, (u_int)0xdfe2e23d,
    (u_int)0xcdebeb26, (u_int)0x4e272769, (u_int)0x7fb2b2cd, (u_int)0xea75759f,
    (u_int)0x1209091b, (u_int)0x1d83839e, (u_int)0x582c2c74, (u_int)0x341a1a2e,
    (u_int)0x361b1b2d, (u_int)0xdc6e6eb2, (u_int)0xb45a5aee, (u_int)0x5ba0a0fb,
    (u_int)0xa45252f6, (u_int)0x763b3b4d, (u_int)0xb7d6d661, (u_int)0x7db3b3ce,
    (u_int)0x5229297b, (u_int)0xdde3e33e, (u_int)0x5e2f2f71, (u_int)0x13848497,
    (u_int)0xa65353f5, (u_int)0xb9d1d168, (u_int)0x00000000, (u_int)0xc1eded2c,
    (u_int)0x40202060, (u_int)0xe3fcfc1f, (u_int)0x79b1b1c8, (u_int)0xb65b5bed,
    (u_int)0xd46a6abe, (u_int)0x8dcbcb46, (u_int)0x67bebed9, (u_int)0x7239394b,
    (u_int)0x944a4ade, (u_int)0x984c4cd4, (u_int)0xb05858e8, (u_int)0x85cfcf4a,
    (u_int)0xbbd0d06b, (u_int)0xc5efef2a, (u_int)0x4faaaae5, (u_int)0xedfbfb16,
    (u_int)0x864343c5, (u_int)0x9a4d4dd7, (u_int)0x66333355, (u_int)0x11858594,
    (u_int)0x8a4545cf, (u_int)0xe9f9f910, (u_int)0x04020206, (u_int)0xfe7f7f81,
    (u_int)0xa05050f0, (u_int)0x783c3c44, (u_int)0x259f9fba, (u_int)0x4ba8a8e3,
    (u_int)0xa25151f3, (u_int)0x5da3a3fe, (u_int)0x804040c0, (u_int)0x058f8f8a,
    (u_int)0x3f9292ad, (u_int)0x219d9dbc, (u_int)0x70383848, (u_int)0xf1f5f504,
    (u_int)0x63bcbcdf, (u_int)0x77b6b6c1, (u_int)0xafdada75, (u_int)0x42212163,
    (u_int)0x20101030, (u_int)0xe5ffff1a, (u_int)0xfdf3f30e, (u_int)0xbfd2d26d,
    (u_int)0x81cdcd4c, (u_int)0x180c0c14, (u_int)0x26131335, (u_int)0xc3ecec2f,
    (u_int)0xbe5f5fe1, (u_int)0x359797a2, (u_int)0x884444cc, (u_int)0x2e171739,
    (u_int)0x93c4c457, (u_int)0x55a7a7f2, (u_int)0xfc7e7e82, (u_int)0x7a3d3d47,
    (u_int)0xc86464ac, (u_int)0xba5d5de7, (u_int)0x3219192b, (u_int)0xe6737395,
    (u_int)0xc06060a0, (u_int)0x19818198, (u_int)0x9e4f4fd1, (u_int)0xa3dcdc7f,
    (u_int)0x44222266, (u_int)0x542a2a7e, (u_int)0x3b9090ab, (u_int)0x0b888883,
    (u_int)0x8c4646ca, (u_int)0xc7eeee29, (u_int)0x6bb8b8d3, (u_int)0x2814143c,
    (u_int)0xa7dede79, (u_int)0xbc5e5ee2, (u_int)0x160b0b1d, (u_int)0xaddbdb76,
    (u_int)0xdbe0e03b, (u_int)0x64323256, (u_int)0x743a3a4e, (u_int)0x140a0a1e,
    (u_int)0x924949db, (u_int)0x0c06060a, (u_int)0x4824246c, (u_int)0xb85c5ce4,
    (u_int)0x9fc2c25d, (u_int)0xbdd3d36e, (u_int)0x43acacef, (u_int)0xc46262a6,
    (u_int)0x399191a8, (u_int)0x319595a4, (u_int)0xd3e4e437, (u_int)0xf279798b,
    (u_int)0xd5e7e732, (u_int)0x8bc8c843, (u_int)0x6e373759, (u_int)0xda6d6db7,
    (u_int)0x018d8d8c, (u_int)0xb1d5d564, (u_int)0x9c4e4ed2, (u_int)0x49a9a9e0,
    (u_int)0xd86c6cb4, (u_int)0xac5656fa, (u_int)0xf3f4f407, (u_int)0xcfeaea25,
    (u_int)0xca6565af, (u_int)0xf47a7a8e, (u_int)0x47aeaee9, (u_int)0x10080818,
    (u_int)0x6fbabad5, (u_int)0xf0787888, (u_int)0x4a25256f, (u_int)0x5c2e2e72,
    (u_int)0x381c1c24, (u_int)0x57a6a6f1, (u_int)0x73b4b4c7, (u_int)0x97c6c651,
    (u_int)0xcbe8e823, (u_int)0xa1dddd7c, (u_int)0xe874749c, (u_int)0x3e1f1f21,
    (u_int)0x964b4bdd, (u_int)0x61bdbddc, (u_int)0x0d8b8b86, (u_int)0x0f8a8a85,
    (u_int)0xe0707090, (u_int)0x7c3e3e42, (u_int)0x71b5b5c4, (u_int)0xcc6666aa,
    (u_int)0x904848d8, (u_int)0x06030305, (u_int)0xf7f6f601, (u_int)0x1c0e0e12,
    (u_int)0xc26161a3, (u_int)0x6a35355f, (u_int)0xae5757f9, (u_int)0x69b9b9d0,
    (u_int)0x17868691, (u_int)0x99c1c158, (u_int)0x3a1d1d27, (u_int)0x279e9eb9,
    (u_int)0xd9e1e138, (u_int)0xebf8f813, (u_int)0x2b9898b3, (u_int)0x22111133,
    (u_int)0xd26969bb, (u_int)0xa9d9d970, (u_int)0x078e8e89, (u_int)0x339494a7,
    (u_int)0x2d9b9bb6, (u_int)0x3c1e1e22, (u_int)0x15878792, (u_int)0xc9e9e920,
    (u_int)0x87cece49, (u_int)0xaa5555ff, (u_int)0x50282878, (u_int)0xa5dfdf7a,
    (u_int)0x038c8c8f, (u_int)0x59a1a1f8, (u_int)0x09898980, (u_int)0x1a0d0d17,
    (u_int)0x65bfbfda, (u_int)0xd7e6e631, (u_int)0x844242c6, (u_int)0xd06868b8,
    (u_int)0x824141c3, (u_int)0x299999b0, (u_int)0x5a2d2d77, (u_int)0x1e0f0f11,
    (u_int)0x7bb0b0cb, (u_int)0xa85454fc, (u_int)0x6dbbbbd6, (u_int)0x2c16163a,
};
static const u32 Te1[256] = {
    (u_int)0xa5c66363, (u_int)0x84f87c7c, (u_int)0x99ee7777, (u_int)0x8df67b7b,
    (u_int)0x0dfff2f2, (u_int)0xbdd66b6b, (u_int)0xb1de6f6f, (u_int)0x5491c5c5,
    (u_int)0x50603030, (u_int)0x03020101, (u_int)0xa9ce6767, (u_int)0x7d562b2b,
    (u_int)0x19e7fefe, (u_int)0x62b5d7d7, (u_int)0xe64dabab, (u_int)0x9aec7676,
    (u_int)0x458fcaca, (u_int)0x9d1f8282, (u_int)0x4089c9c9, (u_int)0x87fa7d7d,
    (u_int)0x15effafa, (u_int)0xebb25959, (u_int)0xc98e4747, (u_int)0x0bfbf0f0,
    (u_int)0xec41adad, (u_int)0x67b3d4d4, (u_int)0xfd5fa2a2, (u_int)0xea45afaf,
    (u_int)0xbf239c9c, (u_int)0xf753a4a4, (u_int)0x96e47272, (u_int)0x5b9bc0c0,
    (u_int)0xc275b7b7, (u_int)0x1ce1fdfd, (u_int)0xae3d9393, (u_int)0x6a4c2626,
    (u_int)0x5a6c3636, (u_int)0x417e3f3f, (u_int)0x02f5f7f7, (u_int)0x4f83cccc,
    (u_int)0x5c683434, (u_int)0xf451a5a5, (u_int)0x34d1e5e5, (u_int)0x08f9f1f1,
    (u_int)0x93e27171, (u_int)0x73abd8d8, (u_int)0x53623131, (u_int)0x3f2a1515,
    (u_int)0x0c080404, (u_int)0x5295c7c7, (u_int)0x65462323, (u_int)0x5e9dc3c3,
    (u_int)0x28301818, (u_int)0xa1379696, (u_int)0x0f0a0505, (u_int)0xb52f9a9a,
    (u_int)0x090e0707, (u_int)0x36241212, (u_int)0x9b1b8080, (u_int)0x3ddfe2e2,
    (u_int)0x26cdebeb, (u_int)0x694e2727, (u_int)0xcd7fb2b2, (u_int)0x9fea7575,
    (u_int)0x1b120909, (u_int)0x9e1d8383, (u_int)0x74582c2c, (u_int)0x2e341a1a,
    (u_int)0x2d361b1b, (u_int)0xb2dc6e6e, (u_int)0xeeb45a5a, (u_int)0xfb5ba0a0,
    (u_int)0xf6a45252, (u_int)0x4d763b3b, (u_int)0x61b7d6d6, (u_int)0xce7db3b3,
    (u_int)0x7b522929, (u_int)0x3edde3e3, (u_int)0x715e2f2f, (u_int)0x97138484,
    (u_int)0xf5a65353, (u_int)0x68b9d1d1, (u_int)0x00000000, (u_int)0x2cc1eded,
    (u_int)0x60402020, (u_int)0x1fe3fcfc, (u_int)0xc879b1b1, (u_int)0xedb65b5b,
    (u_int)0xbed46a6a, (u_int)0x468dcbcb, (u_int)0xd967bebe, (u_int)0x4b723939,
    (u_int)0xde944a4a, (u_int)0xd4984c4c, (u_int)0xe8b05858, (u_int)0x4a85cfcf,
    (u_int)0x6bbbd0d0, (u_int)0x2ac5efef, (u_int)0xe54faaaa, (u_int)0x16edfbfb,
    (u_int)0xc5864343, (u_int)0xd79a4d4d, (u_int)0x55663333, (u_int)0x94118585,
    (u_int)0xcf8a4545, (u_int)0x10e9f9f9, (u_int)0x06040202, (u_int)0x81fe7f7f,
    (u_int)0xf0a05050, (u_int)0x44783c3c, (u_int)0xba259f9f, (u_int)0xe34ba8a8,
    (u_int)0xf3a25151, (u_int)0xfe5da3a3, (u_int)0xc0804040, (u_int)0x8a058f8f,
    (u_int)0xad3f9292, (u_int)0xbc219d9d, (u_int)0x48703838, (u_int)0x04f1f5f5,
    (u_int)0xdf63bcbc, (u_int)0xc177b6b6, (u_int)0x75afdada, (u_int)0x63422121,
    (u_int)0x30201010, (u_int)0x1ae5ffff, (u_int)0x0efdf3f3, (u_int)0x6dbfd2d2,
    (u_int)0x4c81cdcd, (u_int)0x14180c0c, (u_int)0x35261313, (u_int)0x2fc3ecec,
    (u_int)0xe1be5f5f, (u_int)0xa2359797, (u_int)0xcc884444, (u_int)0x392e1717,
    (u_int)0x5793c4c4, (u_int)0xf255a7a7, (u_int)0x82fc7e7e, (u_int)0x477a3d3d,
    (u_int)0xacc86464, (u_int)0xe7ba5d5d, (u_int)0x2b321919, (u_int)0x95e67373,
    (u_int)0xa0c06060, (u_int)0x98198181, (u_int)0xd19e4f4f, (u_int)0x7fa3dcdc,
    (u_int)0x66442222, (u_int)0x7e542a2a, (u_int)0xab3b9090, (u_int)0x830b8888,
    (u_int)0xca8c4646, (u_int)0x29c7eeee, (u_int)0xd36bb8b8, (u_int)0x3c281414,
    (u_int)0x79a7dede, (u_int)0xe2bc5e5e, (u_int)0x1d160b0b, (u_int)0x76addbdb,
    (u_int)0x3bdbe0e0, (u_int)0x56643232, (u_int)0x4e743a3a, (u_int)0x1e140a0a,
    (u_int)0xdb924949, (u_int)0x0a0c0606, (u_int)0x6c482424, (u_int)0xe4b85c5c,
    (u_int)0x5d9fc2c2, (u_int)0x6ebdd3d3, (u_int)0xef43acac, (u_int)0xa6c46262,
    (u_int)0xa8399191, (u_int)0xa4319595, (u_int)0x37d3e4e4, (u_int)0x8bf27979,
    (u_int)0x32d5e7e7, (u_int)0x438bc8c8, (u_int)0x596e3737, (u_int)0xb7da6d6d,
    (u_int)0x8c018d8d, (u_int)0x64b1d5d5, (u_int)0xd29c4e4e, (u_int)0xe049a9a9,
    (u_int)0xb4d86c6c, (u_int)0xfaac5656, (u_int)0x07f3f4f4, (u_int)0x25cfeaea,
    (u_int)0xafca6565, (u_int)0x8ef47a7a, (u_int)0xe947aeae, (u_int)0x18100808,
    (u_int)0xd56fbaba, (u_int)0x88f07878, (u_int)0x6f4a2525, (u_int)0x725c2e2e,
    (u_int)0x24381c1c, (u_int)0xf157a6a6, (u_int)0xc773b4b4, (u_int)0x5197c6c6,
    (u_int)0x23cbe8e8, (u_int)0x7ca1dddd, (u_int)0x9ce87474, (u_int)0x213e1f1f,
    (u_int)0xdd964b4b, (u_int)0xdc61bdbd, (u_int)0x860d8b8b, (u_int)0x850f8a8a,
    (u_int)0x90e07070, (u_int)0x427c3e3e, (u_int)0xc471b5b5, (u_int)0xaacc6666,
    (u_int)0xd8904848, (u_int)0x05060303, (u_int)0x01f7f6f6, (u_int)0x121c0e0e,
    (u_int)0xa3c26161, (u_int)0x5f6a3535, (u_int)0xf9ae5757, (u_int)0xd069b9b9,
    (u_int)0x91178686, (u_int)0x5899c1c1, (u_int)0x273a1d1d, (u_int)0xb9279e9e,
    (u_int)0x38d9e1e1, (u_int)0x13ebf8f8, (u_int)0xb32b9898, (u_int)0x33221111,
    (u_int)0xbbd26969, (u_int)0x70a9d9d9, (u_int)0x89078e8e, (u_int)0xa7339494,
    (u_int)0xb62d9b9b, (u_int)0x223c1e1e, (u_int)0x92158787, (u_int)0x20c9e9e9,
    (u_int)0x4987cece, (u_int)0xffaa5555, (u_int)0x78502828, (u_int)0x7aa5dfdf,
    (u_int)0x8f038c8c, (u_int)0xf859a1a1, (u_int)0x80098989, (u_int)0x171a0d0d,
    (u_int)0xda65bfbf, (u_int)0x31d7e6e6, (u_int)0xc6844242, (u_int)0xb8d06868,
    (u_int)0xc3824141, (u_int)0xb0299999, (u_int)0x775a2d2d, (u_int)0x111e0f0f,
    (u_int)0xcb7bb0b0, (u_int)0xfca85454, (u_int)0xd66dbbbb, (u_int)0x3a2c1616,
};
static const u32 Te2[256] = {
    (u_int)0x63a5c663, (u_int)0x7c84f87c, (u_int)0x7799ee77, (u_int)0x7b8df67b,
    (u_int)0xf20dfff2, (u_int)0x6bbdd66b, (u_int)0x6fb1de6f, (u_int)0xc55491c5,
    (u_int)0x30506030, (u_int)0x01030201, (u_int)0x67a9ce67, (u_int)0x2b7d562b,
    (u_int)0xfe19e7fe, (u_int)0xd762b5d7, (u_int)0xabe64dab, (u_int)0x769aec76,
    (u_int)0xca458fca, (u_int)0x829d1f82, (u_int)0xc94089c9, (u_int)0x7d87fa7d,
    (u_int)0xfa15effa, (u_int)0x59ebb259, (u_int)0x47c98e47, (u_int)0xf00bfbf0,
    (u_int)0xadec41ad, (u_int)0xd467b3d4, (u_int)0xa2fd5fa2, (u_int)0xafea45af,
    (u_int)0x9cbf239c, (u_int)0xa4f753a4, (u_int)0x7296e472, (u_int)0xc05b9bc0,
    (u_int)0xb7c275b7, (u_int)0xfd1ce1fd, (u_int)0x93ae3d93, (u_int)0x266a4c26,
    (u_int)0x365a6c36, (u_int)0x3f417e3f, (u_int)0xf702f5f7, (u_int)0xcc4f83cc,
    (u_int)0x345c6834, (u_int)0xa5f451a5, (u_int)0xe534d1e5, (u_int)0xf108f9f1,
    (u_int)0x7193e271, (u_int)0xd873abd8, (u_int)0x31536231, (u_int)0x153f2a15,
    (u_int)0x040c0804, (u_int)0xc75295c7, (u_int)0x23654623, (u_int)0xc35e9dc3,
    (u_int)0x18283018, (u_int)0x96a13796, (u_int)0x050f0a05, (u_int)0x9ab52f9a,
    (u_int)0x07090e07, (u_int)0x12362412, (u_int)0x809b1b80, (u_int)0xe23ddfe2,
    (u_int)0xeb26cdeb, (u_int)0x27694e27, (u_int)0xb2cd7fb2, (u_int)0x759fea75,
    (u_int)0x091b1209, (u_int)0x839e1d83, (u_int)0x2c74582c, (u_int)0x1a2e341a,
    (u_int)0x1b2d361b, (u_int)0x6eb2dc6e, (u_int)0x5aeeb45a, (u_int)0xa0fb5ba0,
    (u_int)0x52f6a452, (u_int)0x3b4d763b, (u_int)0xd661b7d6, (u_int)0xb3ce7db3,
    (u_int)0x297b5229, (u_int)0xe33edde3, (u_int)0x2f715e2f, (u_int)0x84971384,
    (u_int)0x53f5a653, (u_int)0xd168b9d1, (u_int)0x00000000, (u_int)0xed2cc1ed,
    (u_int)0x20604020, (u_int)0xfc1fe3fc, (u_int)0xb1c879b1, (u_int)0x5bedb65b,
    (u_int)0x6abed46a, (u_int)0xcb468dcb, (u_int)0xbed967be, (u_int)0x394b7239,
    (u_int)0x4ade944a, (u_int)0x4cd4984c, (u_int)0x58e8b058, (u_int)0xcf4a85cf,
    (u_int)0xd06bbbd0, (u_int)0xef2ac5ef, (u_int)0xaae54faa, (u_int)0xfb16edfb,
    (u_int)0x43c58643, (u_int)0x4dd79a4d, (u_int)0x33556633, (u_int)0x85941185,
    (u_int)0x45cf8a45, (u_int)0xf910e9f9, (u_int)0x02060402, (u_int)0x7f81fe7f,
    (u_int)0x50f0a050, (u_int)0x3c44783c, (u_int)0x9fba259f, (u_int)0xa8e34ba8,
    (u_int)0x51f3a251, (u_int)0xa3fe5da3, (u_int)0x40c08040, (u_int)0x8f8a058f,
    (u_int)0x92ad3f92, (u_int)0x9dbc219d, (u_int)0x38487038, (u_int)0xf504f1f5,
    (u_int)0xbcdf63bc, (u_int)0xb6c177b6, (u_int)0xda75afda, (u_int)0x21634221,
    (u_int)0x10302010, (u_int)0xff1ae5ff, (u_int)0xf30efdf3, (u_int)0xd26dbfd2,
    (u_int)0xcd4c81cd, (u_int)0x0c14180c, (u_int)0x13352613, (u_int)0xec2fc3ec,
    (u_int)0x5fe1be5f, (u_int)0x97a23597, (u_int)0x44cc8844, (u_int)0x17392e17,
    (u_int)0xc45793c4, (u_int)0xa7f255a7, (u_int)0x7e82fc7e, (u_int)0x3d477a3d,
    (u_int)0x64acc864, (u_int)0x5de7ba5d, (u_int)0x192b3219, (u_int)0x7395e673,
    (u_int)0x60a0c060, (u_int)0x81981981, (u_int)0x4fd19e4f, (u_int)0xdc7fa3dc,
    (u_int)0x22664422, (u_int)0x2a7e542a, (u_int)0x90ab3b90, (u_int)0x88830b88,
    (u_int)0x46ca8c46, (u_int)0xee29c7ee, (u_int)0xb8d36bb8, (u_int)0x143c2814,
    (u_int)0xde79a7de, (u_int)0x5ee2bc5e, (u_int)0x0b1d160b, (u_int)0xdb76addb,
    (u_int)0xe03bdbe0, (u_int)0x32566432, (u_int)0x3a4e743a, (u_int)0x0a1e140a,
    (u_int)0x49db9249, (u_int)0x060a0c06, (u_int)0x246c4824, (u_int)0x5ce4b85c,
    (u_int)0xc25d9fc2, (u_int)0xd36ebdd3, (u_int)0xacef43ac, (u_int)0x62a6c462,
    (u_int)0x91a83991, (u_int)0x95a43195, (u_int)0xe437d3e4, (u_int)0x798bf279,
    (u_int)0xe732d5e7, (u_int)0xc8438bc8, (u_int)0x37596e37, (u_int)0x6db7da6d,
    (u_int)0x8d8c018d, (u_int)0xd564b1d5, (u_int)0x4ed29c4e, (u_int)0xa9e049a9,
    (u_int)0x6cb4d86c, (u_int)0x56faac56, (u_int)0xf407f3f4, (u_int)0xea25cfea,
    (u_int)0x65afca65, (u_int)0x7a8ef47a, (u_int)0xaee947ae, (u_int)0x08181008,
    (u_int)0xbad56fba, (u_int)0x7888f078, (u_int)0x256f4a25, (u_int)0x2e725c2e,
    (u_int)0x1c24381c, (u_int)0xa6f157a6, (u_int)0xb4c773b4, (u_int)0xc65197c6,
    (u_int)0xe823cbe8, (u_int)0xdd7ca1dd, (u_int)0x749ce874, (u_int)0x1f213e1f,
    (u_int)0x4bdd964b, (u_int)0xbddc61bd, (u_int)0x8b860d8b, (u_int)0x8a850f8a,
    (u_int)0x7090e070, (u_int)0x3e427c3e, (u_int)0xb5c471b5, (u_int)0x66aacc66,
    (u_int)0x48d89048, (u_int)0x03050603, (u_int)0xf601f7f6, (u_int)0x0e121c0e,
    (u_int)0x61a3c261, (u_int)0x355f6a35, (u_int)0x57f9ae57, (u_int)0xb9d069b9,
    (u_int)0x86911786, (u_int)0xc15899c1, (u_int)0x1d273a1d, (u_int)0x9eb9279e,
    (u_int)0xe138d9e1, (u_int)0xf813ebf8, (u_int)0x98b32b98, (u_int)0x11332211,
    (u_int)0x69bbd269, (u_int)0xd970a9d9, (u_int)0x8e89078e, (u_int)0x94a73394,
    (u_int)0x9bb62d9b, (u_int)0x1e223c1e, (u_int)0x87921587, (u_int)0xe920c9e9,
    (u_int)0xce4987ce, (u_int)0x55ffaa55, (u_int)0x28785028, (u_int)0xdf7aa5df,
    (u_int)0x8c8f038c, (u_int)0xa1f859a1, (u_int)0x89800989, (u_int)0x0d171a0d,
    (u_int)0xbfda65bf, (u_int)0xe631d7e6, (u_int)0x42c68442, (u_int)0x68b8d068,
    (u_int)0x41c38241, (u_int)0x99b02999, (u_int)0x2d775a2d, (u_int)0x0f111e0f,
    (u_int)0xb0cb7bb0, (u_int)0x54fca854, (u_int)0xbbd66dbb, (u_int)0x163a2c16,
};
static const u32 Te3[256] = {

    (u_int)0x6363a5c6, (u_int)0x7c7c84f8, (u_int)0x777799ee, (u_int)0x7b7b8df6,
    (u_int)0xf2f20dff, (u_int)0x6b6bbdd6, (u_int)0x6f6fb1de, (u_int)0xc5c55491,
    (u_int)0x30305060, (u_int)0x01010302, (u_int)0x6767a9ce, (u_int)0x2b2b7d56,
    (u_int)0xfefe19e7, (u_int)0xd7d762b5, (u_int)0xababe64d, (u_int)0x76769aec,
    (u_int)0xcaca458f, (u_int)0x82829d1f, (u_int)0xc9c94089, (u_int)0x7d7d87fa,
    (u_int)0xfafa15ef, (u_int)0x5959ebb2, (u_int)0x4747c98e, (u_int)0xf0f00bfb,
    (u_int)0xadadec41, (u_int)0xd4d467b3, (u_int)0xa2a2fd5f, (u_int)0xafafea45,
    (u_int)0x9c9cbf23, (u_int)0xa4a4f753, (u_int)0x727296e4, (u_int)0xc0c05b9b,
    (u_int)0xb7b7c275, (u_int)0xfdfd1ce1, (u_int)0x9393ae3d, (u_int)0x26266a4c,
    (u_int)0x36365a6c, (u_int)0x3f3f417e, (u_int)0xf7f702f5, (u_int)0xcccc4f83,
    (u_int)0x34345c68, (u_int)0xa5a5f451, (u_int)0xe5e534d1, (u_int)0xf1f108f9,
    (u_int)0x717193e2, (u_int)0xd8d873ab, (u_int)0x31315362, (u_int)0x15153f2a,
    (u_int)0x04040c08, (u_int)0xc7c75295, (u_int)0x23236546, (u_int)0xc3c35e9d,
    (u_int)0x18182830, (u_int)0x9696a137, (u_int)0x05050f0a, (u_int)0x9a9ab52f,
    (u_int)0x0707090e, (u_int)0x12123624, (u_int)0x80809b1b, (u_int)0xe2e23ddf,
    (u_int)0xebeb26cd, (u_int)0x2727694e, (u_int)0xb2b2cd7f, (u_int)0x75759fea,
    (u_int)0x09091b12, (u_int)0x83839e1d, (u_int)0x2c2c7458, (u_int)0x1a1a2e34,
    (u_int)0x1b1b2d36, (u_int)0x6e6eb2dc, (u_int)0x5a5aeeb4, (u_int)0xa0a0fb5b,
    (u_int)0x5252f6a4, (u_int)0x3b3b4d76, (u_int)0xd6d661b7, (u_int)0xb3b3ce7d,
    (u_int)0x29297b52, (u_int)0xe3e33edd, (u_int)0x2f2f715e, (u_int)0x84849713,
    (u_int)0x5353f5a6, (u_int)0xd1d168b9, (u_int)0x00000000, (u_int)0xeded2cc1,
    (u_int)0x20206040, (u_int)0xfcfc1fe3, (u_int)0xb1b1c879, (u_int)0x5b5bedb6,
    (u_int)0x6a6abed4, (u_int)0xcbcb468d, (u_int)0xbebed967, (u_int)0x39394b72,
    (u_int)0x4a4ade94, (u_int)0x4c4cd498, (u_int)0x5858e8b0, (u_int)0xcfcf4a85,
    (u_int)0xd0d06bbb, (u_int)0xefef2ac5, (u_int)0xaaaae54f, (u_int)0xfbfb16ed,
    (u_int)0x4343c586, (u_int)0x4d4dd79a, (u_int)0x33335566, (u_int)0x85859411,
    (u_int)0x4545cf8a, (u_int)0xf9f910e9, (u_int)0x02020604, (u_int)0x7f7f81fe,
    (u_int)0x5050f0a0, (u_int)0x3c3c4478, (u_int)0x9f9fba25, (u_int)0xa8a8e34b,
    (u_int)0x5151f3a2, (u_int)0xa3a3fe5d, (u_int)0x4040c080, (u_int)0x8f8f8a05,
    (u_int)0x9292ad3f, (u_int)0x9d9dbc21, (u_int)0x38384870, (u_int)0xf5f504f1,
    (u_int)0xbcbcdf63, (u_int)0xb6b6c177, (u_int)0xdada75af, (u_int)0x21216342,
    (u_int)0x10103020, (u_int)0xffff1ae5, (u_int)0xf3f30efd, (u_int)0xd2d26dbf,
    (u_int)0xcdcd4c81, (u_int)0x0c0c1418, (u_int)0x13133526, (u_int)0xecec2fc3,
    (u_int)0x5f5fe1be, (u_int)0x9797a235, (u_int)0x4444cc88, (u_int)0x1717392e,
    (u_int)0xc4c45793, (u_int)0xa7a7f255, (u_int)0x7e7e82fc, (u_int)0x3d3d477a,
    (u_int)0x6464acc8, (u_int)0x5d5de7ba, (u_int)0x19192b32, (u_int)0x737395e6,
    (u_int)0x6060a0c0, (u_int)0x81819819, (u_int)0x4f4fd19e, (u_int)0xdcdc7fa3,
    (u_int)0x22226644, (u_int)0x2a2a7e54, (u_int)0x9090ab3b, (u_int)0x8888830b,
    (u_int)0x4646ca8c, (u_int)0xeeee29c7, (u_int)0xb8b8d36b, (u_int)0x14143c28,
    (u_int)0xdede79a7, (u_int)0x5e5ee2bc, (u_int)0x0b0b1d16, (u_int)0xdbdb76ad,
    (u_int)0xe0e03bdb, (u_int)0x32325664, (u_int)0x3a3a4e74, (u_int)0x0a0a1e14,
    (u_int)0x4949db92, (u_int)0x06060a0c, (u_int)0x24246c48, (u_int)0x5c5ce4b8,
    (u_int)0xc2c25d9f, (u_int)0xd3d36ebd, (u_int)0xacacef43, (u_int)0x6262a6c4,
    (u_int)0x9191a839, (u_int)0x9595a431, (u_int)0xe4e437d3, (u_int)0x79798bf2,
    (u_int)0xe7e732d5, (u_int)0xc8c8438b, (u_int)0x3737596e, (u_int)0x6d6db7da,
    (u_int)0x8d8d8c01, (u_int)0xd5d564b1, (u_int)0x4e4ed29c, (u_int)0xa9a9e049,
    (u_int)0x6c6cb4d8, (u_int)0x5656faac, (u_int)0xf4f407f3, (u_int)0xeaea25cf,
    (u_int)0x6565afca, (u_int)0x7a7a8ef4, (u_int)0xaeaee947, (u_int)0x08081810,
    (u_int)0xbabad56f, (u_int)0x787888f0, (u_int)0x25256f4a, (u_int)0x2e2e725c,
    (u_int)0x1c1c2438, (u_int)0xa6a6f157, (u_int)0xb4b4c773, (u_int)0xc6c65197,
    (u_int)0xe8e823cb, (u_int)0xdddd7ca1, (u_int)0x74749ce8, (u_int)0x1f1f213e,
    (u_int)0x4b4bdd96, (u_int)0xbdbddc61, (u_int)0x8b8b860d, (u_int)0x8a8a850f,
    (u_int)0x707090e0, (u_int)0x3e3e427c, (u_int)0xb5b5c471, (u_int)0x6666aacc,
    (u_int)0x4848d890, (u_int)0x03030506, (u_int)0xf6f601f7, (u_int)0x0e0e121c,
    (u_int)0x6161a3c2, (u_int)0x35355f6a, (u_int)0x5757f9ae, (u_int)0xb9b9d069,
    (u_int)0x86869117, (u_int)0xc1c15899, (u_int)0x1d1d273a, (u_int)0x9e9eb927,
    (u_int)0xe1e138d9, (u_int)0xf8f813eb, (u_int)0x9898b32b, (u_int)0x11113322,
    (u_int)0x6969bbd2, (u_int)0xd9d970a9, (u_int)0x8e8e8907, (u_int)0x9494a733,
    (u_int)0x9b9bb62d, (u_int)0x1e1e223c, (u_int)0x87879215, (u_int)0xe9e920c9,
    (u_int)0xcece4987, (u_int)0x5555ffaa, (u_int)0x28287850, (u_int)0xdfdf7aa5,
    (u_int)0x8c8c8f03, (u_int)0xa1a1f859, (u_int)0x89898009, (u_int)0x0d0d171a,
    (u_int)0xbfbfda65, (u_int)0xe6e631d7, (u_int)0x4242c684, (u_int)0x6868b8d0,
    (u_int)0x4141c382, (u_int)0x9999b029, (u_int)0x2d2d775a, (u_int)0x0f0f111e,
    (u_int)0xb0b0cb7b, (u_int)0x5454fca8, (u_int)0xbbbbd66d, (u_int)0x16163a2c,
};
static const u32 Te4[256] = {
    (u_int)0x63636363, (u_int)0x7c7c7c7c, (u_int)0x77777777, (u_int)0x7b7b7b7b,
    (u_int)0xf2f2f2f2, (u_int)0x6b6b6b6b, (u_int)0x6f6f6f6f, (u_int)0xc5c5c5c5,
    (u_int)0x30303030, (u_int)0x01010101, (u_int)0x67676767, (u_int)0x2b2b2b2b,
    (u_int)0xfefefefe, (u_int)0xd7d7d7d7, (u_int)0xabababab, (u_int)0x76767676,
    (u_int)0xcacacaca, (u_int)0x82828282, (u_int)0xc9c9c9c9, (u_int)0x7d7d7d7d,
    (u_int)0xfafafafa, (u_int)0x59595959, (u_int)0x47474747, (u_int)0xf0f0f0f0,
    (u_int)0xadadadad, (u_int)0xd4d4d4d4, (u_int)0xa2a2a2a2, (u_int)0xafafafaf,
    (u_int)0x9c9c9c9c, (u_int)0xa4a4a4a4, (u_int)0x72727272, (u_int)0xc0c0c0c0,
    (u_int)0xb7b7b7b7, (u_int)0xfdfdfdfd, (u_int)0x93939393, (u_int)0x26262626,
    (u_int)0x36363636, (u_int)0x3f3f3f3f, (u_int)0xf7f7f7f7, (u_int)0xcccccccc,
    (u_int)0x34343434, (u_int)0xa5a5a5a5, (u_int)0xe5e5e5e5, (u_int)0xf1f1f1f1,
    (u_int)0x71717171, (u_int)0xd8d8d8d8, (u_int)0x31313131, (u_int)0x15151515,
    (u_int)0x04040404, (u_int)0xc7c7c7c7, (u_int)0x23232323, (u_int)0xc3c3c3c3,
    (u_int)0x18181818, (u_int)0x96969696, (u_int)0x05050505, (u_int)0x9a9a9a9a,
    (u_int)0x07070707, (u_int)0x12121212, (u_int)0x80808080, (u_int)0xe2e2e2e2,
    (u_int)0xebebebeb, (u_int)0x27272727, (u_int)0xb2b2b2b2, (u_int)0x75757575,
    (u_int)0x09090909, (u_int)0x83838383, (u_int)0x2c2c2c2c, (u_int)0x1a1a1a1a,
    (u_int)0x1b1b1b1b, (u_int)0x6e6e6e6e, (u_int)0x5a5a5a5a, (u_int)0xa0a0a0a0,
    (u_int)0x52525252, (u_int)0x3b3b3b3b, (u_int)0xd6d6d6d6, (u_int)0xb3b3b3b3,
    (u_int)0x29292929, (u_int)0xe3e3e3e3, (u_int)0x2f2f2f2f, (u_int)0x84848484,
    (u_int)0x53535353, (u_int)0xd1d1d1d1, (u_int)0x00000000, (u_int)0xedededed,
    (u_int)0x20202020, (u_int)0xfcfcfcfc, (u_int)0xb1b1b1b1, (u_int)0x5b5b5b5b,
    (u_int)0x6a6a6a6a, (u_int)0xcbcbcbcb, (u_int)0xbebebebe, (u_int)0x39393939,
    (u_int)0x4a4a4a4a, (u_int)0x4c4c4c4c, (u_int)0x58585858, (u_int)0xcfcfcfcf,
    (u_int)0xd0d0d0d0, (u_int)0xefefefef, (u_int)0xaaaaaaaa, (u_int)0xfbfbfbfb,
    (u_int)0x43434343, (u_int)0x4d4d4d4d, (u_int)0x33333333, (u_int)0x85858585,
    (u_int)0x45454545, (u_int)0xf9f9f9f9, (u_int)0x02020202, (u_int)0x7f7f7f7f,
    (u_int)0x50505050, (u_int)0x3c3c3c3c, (u_int)0x9f9f9f9f, (u_int)0xa8a8a8a8,
    (u_int)0x51515151, (u_int)0xa3a3a3a3, (u_int)0x40404040, (u_int)0x8f8f8f8f,
    (u_int)0x92929292, (u_int)0x9d9d9d9d, (u_int)0x38383838, (u_int)0xf5f5f5f5,
    (u_int)0xbcbcbcbc, (u_int)0xb6b6b6b6, (u_int)0xdadadada, (u_int)0x21212121,
    (u_int)0x10101010, (u_int)0xffffffff, (u_int)0xf3f3f3f3, (u_int)0xd2d2d2d2,
    (u_int)0xcdcdcdcd, (u_int)0x0c0c0c0c, (u_int)0x13131313, (u_int)0xecececec,
    (u_int)0x5f5f5f5f, (u_int)0x97979797, (u_int)0x44444444, (u_int)0x17171717,
    (u_int)0xc4c4c4c4, (u_int)0xa7a7a7a7, (u_int)0x7e7e7e7e, (u_int)0x3d3d3d3d,
    (u_int)0x64646464, (u_int)0x5d5d5d5d, (u_int)0x19191919, (u_int)0x73737373,
    (u_int)0x60606060, (u_int)0x81818181, (u_int)0x4f4f4f4f, (u_int)0xdcdcdcdc,
    (u_int)0x22222222, (u_int)0x2a2a2a2a, (u_int)0x90909090, (u_int)0x88888888,
    (u_int)0x46464646, (u_int)0xeeeeeeee, (u_int)0xb8b8b8b8, (u_int)0x14141414,
    (u_int)0xdededede, (u_int)0x5e5e5e5e, (u_int)0x0b0b0b0b, (u_int)0xdbdbdbdb,
    (u_int)0xe0e0e0e0, (u_int)0x32323232, (u_int)0x3a3a3a3a, (u_int)0x0a0a0a0a,
    (u_int)0x49494949, (u_int)0x06060606, (u_int)0x24242424, (u_int)0x5c5c5c5c,
    (u_int)0xc2c2c2c2, (u_int)0xd3d3d3d3, (u_int)0xacacacac, (u_int)0x62626262,
    (u_int)0x91919191, (u_int)0x95959595, (u_int)0xe4e4e4e4, (u_int)0x79797979,
    (u_int)0xe7e7e7e7, (u_int)0xc8c8c8c8, (u_int)0x37373737, (u_int)0x6d6d6d6d,
    (u_int)0x8d8d8d8d, (u_int)0xd5d5d5d5, (u_int)0x4e4e4e4e, (u_int)0xa9a9a9a9,
    (u_int)0x6c6c6c6c, (u_int)0x56565656, (u_int)0xf4f4f4f4, (u_int)0xeaeaeaea,
    (u_int)0x65656565, (u_int)0x7a7a7a7a, (u_int)0xaeaeaeae, (u_int)0x08080808,
    (u_int)0xbabababa, (u_int)0x78787878, (u_int)0x25252525, (u_int)0x2e2e2e2e,
    (u_int)0x1c1c1c1c, (u_int)0xa6a6a6a6, (u_int)0xb4b4b4b4, (u_int)0xc6c6c6c6,
    (u_int)0xe8e8e8e8, (u_int)0xdddddddd, (u_int)0x74747474, (u_int)0x1f1f1f1f,
    (u_int)0x4b4b4b4b, (u_int)0xbdbdbdbd, (u_int)0x8b8b8b8b, (u_int)0x8a8a8a8a,
    (u_int)0x70707070, (u_int)0x3e3e3e3e, (u_int)0xb5b5b5b5, (u_int)0x66666666,
    (u_int)0x48484848, (u_int)0x03030303, (u_int)0xf6f6f6f6, (u_int)0x0e0e0e0e,
    (u_int)0x61616161, (u_int)0x35353535, (u_int)0x57575757, (u_int)0xb9b9b9b9,
    (u_int)0x86868686, (u_int)0xc1c1c1c1, (u_int)0x1d1d1d1d, (u_int)0x9e9e9e9e,
    (u_int)0xe1e1e1e1, (u_int)0xf8f8f8f8, (u_int)0x98989898, (u_int)0x11111111,
    (u_int)0x69696969, (u_int)0xd9d9d9d9, (u_int)0x8e8e8e8e, (u_int)0x94949494,
    (u_int)0x9b9b9b9b, (u_int)0x1e1e1e1e, (u_int)0x87878787, (u_int)0xe9e9e9e9,
    (u_int)0xcececece, (u_int)0x55555555, (u_int)0x28282828, (u_int)0xdfdfdfdf,
    (u_int)0x8c8c8c8c, (u_int)0xa1a1a1a1, (u_int)0x89898989, (u_int)0x0d0d0d0d,
    (u_int)0xbfbfbfbf, (u_int)0xe6e6e6e6, (u_int)0x42424242, (u_int)0x68686868,
    (u_int)0x41414141, (u_int)0x99999999, (u_int)0x2d2d2d2d, (u_int)0x0f0f0f0f,
    (u_int)0xb0b0b0b0, (u_int)0x54545454, (u_int)0xbbbbbbbb, (u_int)0x16161616,
};
static const u32 Td0[256] = {
    (u_int)0x51f4a750, (u_int)0x7e416553, (u_int)0x1a17a4c3, (u_int)0x3a275e96,
    (u_int)0x3bab6bcb, (u_int)0x1f9d45f1, (u_int)0xacfa58ab, (u_int)0x4be30393,
    (u_int)0x2030fa55, (u_int)0xad766df6, (u_int)0x88cc7691, (u_int)0xf5024c25,
    (u_int)0x4fe5d7fc, (u_int)0xc52acbd7, (u_int)0x26354480, (u_int)0xb562a38f,
    (u_int)0xdeb15a49, (u_int)0x25ba1b67, (u_int)0x45ea0e98, (u_int)0x5dfec0e1,
    (u_int)0xc32f7502, (u_int)0x814cf012, (u_int)0x8d4697a3, (u_int)0x6bd3f9c6,
    (u_int)0x038f5fe7, (u_int)0x15929c95, (u_int)0xbf6d7aeb, (u_int)0x955259da,
    (u_int)0xd4be832d, (u_int)0x587421d3, (u_int)0x49e06929, (u_int)0x8ec9c844,
    (u_int)0x75c2896a, (u_int)0xf48e7978, (u_int)0x99583e6b, (u_int)0x27b971dd,
    (u_int)0xbee14fb6, (u_int)0xf088ad17, (u_int)0xc920ac66, (u_int)0x7dce3ab4,
    (u_int)0x63df4a18, (u_int)0xe51a3182, (u_int)0x97513360, (u_int)0x62537f45,
    (u_int)0xb16477e0, (u_int)0xbb6bae84, (u_int)0xfe81a01c, (u_int)0xf9082b94,
    (u_int)0x70486858, (u_int)0x8f45fd19, (u_int)0x94de6c87, (u_int)0x527bf8b7,
    (u_int)0xab73d323, (u_int)0x724b02e2, (u_int)0xe31f8f57, (u_int)0x6655ab2a,
    (u_int)0xb2eb2807, (u_int)0x2fb5c203, (u_int)0x86c57b9a, (u_int)0xd33708a5,
    (u_int)0x302887f2, (u_int)0x23bfa5b2, (u_int)0x02036aba, (u_int)0xed16825c,
    (u_int)0x8acf1c2b, (u_int)0xa779b492, (u_int)0xf307f2f0, (u_int)0x4e69e2a1,
    (u_int)0x65daf4cd, (u_int)0x0605bed5, (u_int)0xd134621f, (u_int)0xc4a6fe8a,
    (u_int)0x342e539d, (u_int)0xa2f355a0, (u_int)0x058ae132, (u_int)0xa4f6eb75,
    (u_int)0x0b83ec39, (u_int)0x4060efaa, (u_int)0x5e719f06, (u_int)0xbd6e1051,
    (u_int)0x3e218af9, (u_int)0x96dd063d, (u_int)0xdd3e05ae, (u_int)0x4de6bd46,
    (u_int)0x91548db5, (u_int)0x71c45d05, (u_int)0x0406d46f, (u_int)0x605015ff,
    (u_int)0x1998fb24, (u_int)0xd6bde997, (u_int)0x894043cc, (u_int)0x67d99e77,
    (u_int)0xb0e842bd, (u_int)0x07898b88, (u_int)0xe7195b38, (u_int)0x79c8eedb,
    (u_int)0xa17c0a47, (u_int)0x7c420fe9, (u_int)0xf8841ec9, (u_int)0x00000000,
    (u_int)0x09808683, (u_int)0x322bed48, (u_int)0x1e1170ac, (u_int)0x6c5a724e,
    (u_int)0xfd0efffb, (u_int)0x0f853856, (u_int)0x3daed51e, (u_int)0x362d3927,
    (u_int)0x0a0fd964, (u_int)0x685ca621, (u_int)0x9b5b54d1, (u_int)0x24362e3a,
    (u_int)0x0c0a67b1, (u_int)0x9357e70f, (u_int)0xb4ee96d2, (u_int)0x1b9b919e,
    (u_int)0x80c0c54f, (u_int)0x61dc20a2, (u_int)0x5a774b69, (u_int)0x1c121a16,
    (u_int)0xe293ba0a, (u_int)0xc0a02ae5, (u_int)0x3c22e043, (u_int)0x121b171d,
    (u_int)0x0e090d0b, (u_int)0xf28bc7ad, (u_int)0x2db6a8b9, (u_int)0x141ea9c8,
    (u_int)0x57f11985, (u_int)0xaf75074c, (u_int)0xee99ddbb, (u_int)0xa37f60fd,
    (u_int)0xf701269f, (u_int)0x5c72f5bc, (u_int)0x44663bc5, (u_int)0x5bfb7e34,
    (u_int)0x8b432976, (u_int)0xcb23c6dc, (u_int)0xb6edfc68, (u_int)0xb8e4f163,
    (u_int)0xd731dcca, (u_int)0x42638510, (u_int)0x13972240, (u_int)0x84c61120,
    (u_int)0x854a247d, (u_int)0xd2bb3df8, (u_int)0xaef93211, (u_int)0xc729a16d,
    (u_int)0x1d9e2f4b, (u_int)0xdcb230f3, (u_int)0x0d8652ec, (u_int)0x77c1e3d0,
    (u_int)0x2bb3166c, (u_int)0xa970b999, (u_int)0x119448fa, (u_int)0x47e96422,
    (u_int)0xa8fc8cc4, (u_int)0xa0f03f1a, (u_int)0x567d2cd8, (u_int)0x223390ef,
    (u_int)0x87494ec7, (u_int)0xd938d1c1, (u_int)0x8ccaa2fe, (u_int)0x98d40b36,
    (u_int)0xa6f581cf, (u_int)0xa57ade28, (u_int)0xdab78e26, (u_int)0x3fadbfa4,
    (u_int)0x2c3a9de4, (u_int)0x5078920d, (u_int)0x6a5fcc9b, (u_int)0x547e4662,
    (u_int)0xf68d13c2, (u_int)0x90d8b8e8, (u_int)0x2e39f75e, (u_int)0x82c3aff5,
    (u_int)0x9f5d80be, (u_int)0x69d0937c, (u_int)0x6fd52da9, (u_int)0xcf2512b3,
    (u_int)0xc8ac993b, (u_int)0x10187da7, (u_int)0xe89c636e, (u_int)0xdb3bbb7b,
    (u_int)0xcd267809, (u_int)0x6e5918f4, (u_int)0xec9ab701, (u_int)0x834f9aa8,
    (u_int)0xe6956e65, (u_int)0xaaffe67e, (u_int)0x21bccf08, (u_int)0xef15e8e6,
    (u_int)0xbae79bd9, (u_int)0x4a6f36ce, (u_int)0xea9f09d4, (u_int)0x29b07cd6,
    (u_int)0x31a4b2af, (u_int)0x2a3f2331, (u_int)0xc6a59430, (u_int)0x35a266c0,
    (u_int)0x744ebc37, (u_int)0xfc82caa6, (u_int)0xe090d0b0, (u_int)0x33a7d815,
    (u_int)0xf104984a, (u_int)0x41ecdaf7, (u_int)0x7fcd500e, (u_int)0x1791f62f,
    (u_int)0x764dd68d, (u_int)0x43efb04d, (u_int)0xccaa4d54, (u_int)0xe49604df,
    (u_int)0x9ed1b5e3, (u_int)0x4c6a881b, (u_int)0xc12c1fb8, (u_int)0x4665517f,
    (u_int)0x9d5eea04, (u_int)0x018c355d, (u_int)0xfa877473, (u_int)0xfb0b412e,
    (u_int)0xb3671d5a, (u_int)0x92dbd252, (u_int)0xe9105633, (u_int)0x6dd64713,
    (u_int)0x9ad7618c, (u_int)0x37a10c7a, (u_int)0x59f8148e, (u_int)0xeb133c89,
    (u_int)0xcea927ee, (u_int)0xb761c935, (u_int)0xe11ce5ed, (u_int)0x7a47b13c,
    (u_int)0x9cd2df59, (u_int)0x55f2733f, (u_int)0x1814ce79, (u_int)0x73c737bf,
    (u_int)0x53f7cdea, (u_int)0x5ffdaa5b, (u_int)0xdf3d6f14, (u_int)0x7844db86,
    (u_int)0xcaaff381, (u_int)0xb968c43e, (u_int)0x3824342c, (u_int)0xc2a3405f,
    (u_int)0x161dc372, (u_int)0xbce2250c, (u_int)0x283c498b, (u_int)0xff0d9541,
    (u_int)0x39a80171, (u_int)0x080cb3de, (u_int)0xd8b4e49c, (u_int)0x6456c190,
    (u_int)0x7bcb8461, (u_int)0xd532b670, (u_int)0x486c5c74, (u_int)0xd0b85742,
};
static const u32 Td1[256] = {
    (u_int)0x5051f4a7, (u_int)0x537e4165, (u_int)0xc31a17a4, (u_int)0x963a275e,
    (u_int)0xcb3bab6b, (u_int)0xf11f9d45, (u_int)0xabacfa58, (u_int)0x934be303,
    (u_int)0x552030fa, (u_int)0xf6ad766d, (u_int)0x9188cc76, (u_int)0x25f5024c,
    (u_int)0xfc4fe5d7, (u_int)0xd7c52acb, (u_int)0x80263544, (u_int)0x8fb562a3,
    (u_int)0x49deb15a, (u_int)0x6725ba1b, (u_int)0x9845ea0e, (u_int)0xe15dfec0,
    (u_int)0x02c32f75, (u_int)0x12814cf0, (u_int)0xa38d4697, (u_int)0xc66bd3f9,
    (u_int)0xe7038f5f, (u_int)0x9515929c, (u_int)0xebbf6d7a, (u_int)0xda955259,
    (u_int)0x2dd4be83, (u_int)0xd3587421, (u_int)0x2949e069, (u_int)0x448ec9c8,
    (u_int)0x6a75c289, (u_int)0x78f48e79, (u_int)0x6b99583e, (u_int)0xdd27b971,
    (u_int)0xb6bee14f, (u_int)0x17f088ad, (u_int)0x66c920ac, (u_int)0xb47dce3a,
    (u_int)0x1863df4a, (u_int)0x82e51a31, (u_int)0x60975133, (u_int)0x4562537f,
    (u_int)0xe0b16477, (u_int)0x84bb6bae, (u_int)0x1cfe81a0, (u_int)0x94f9082b,
    (u_int)0x58704868, (u_int)0x198f45fd, (u_int)0x8794de6c, (u_int)0xb7527bf8,
    (u_int)0x23ab73d3, (u_int)0xe2724b02, (u_int)0x57e31f8f, (u_int)0x2a6655ab,
    (u_int)0x07b2eb28, (u_int)0x032fb5c2, (u_int)0x9a86c57b, (u_int)0xa5d33708,
    (u_int)0xf2302887, (u_int)0xb223bfa5, (u_int)0xba02036a, (u_int)0x5ced1682,
    (u_int)0x2b8acf1c, (u_int)0x92a779b4, (u_int)0xf0f307f2, (u_int)0xa14e69e2,
    (u_int)0xcd65daf4, (u_int)0xd50605be, (u_int)0x1fd13462, (u_int)0x8ac4a6fe,
    (u_int)0x9d342e53, (u_int)0xa0a2f355, (u_int)0x32058ae1, (u_int)0x75a4f6eb,
    (u_int)0x390b83ec, (u_int)0xaa4060ef, (u_int)0x065e719f, (u_int)0x51bd6e10,
    (u_int)0xf93e218a, (u_int)0x3d96dd06, (u_int)0xaedd3e05, (u_int)0x464de6bd,
    (u_int)0xb591548d, (u_int)0x0571c45d, (u_int)0x6f0406d4, (u_int)0xff605015,
    (u_int)0x241998fb, (u_int)0x97d6bde9, (u_int)0xcc894043, (u_int)0x7767d99e,
    (u_int)0xbdb0e842, (u_int)0x8807898b, (u_int)0x38e7195b, (u_int)0xdb79c8ee,
    (u_int)0x47a17c0a, (u_int)0xe97c420f, (u_int)0xc9f8841e, (u_int)0x00000000,
    (u_int)0x83098086, (u_int)0x48322bed, (u_int)0xac1e1170, (u_int)0x4e6c5a72,
    (u_int)0xfbfd0eff, (u_int)0x560f8538, (u_int)0x1e3daed5, (u_int)0x27362d39,
    (u_int)0x640a0fd9, (u_int)0x21685ca6, (u_int)0xd19b5b54, (u_int)0x3a24362e,
    (u_int)0xb10c0a67, (u_int)0x0f9357e7, (u_int)0xd2b4ee96, (u_int)0x9e1b9b91,
    (u_int)0x4f80c0c5, (u_int)0xa261dc20, (u_int)0x695a774b, (u_int)0x161c121a,
    (u_int)0x0ae293ba, (u_int)0xe5c0a02a, (u_int)0x433c22e0, (u_int)0x1d121b17,
    (u_int)0x0b0e090d, (u_int)0xadf28bc7, (u_int)0xb92db6a8, (u_int)0xc8141ea9,
    (u_int)0x8557f119, (u_int)0x4caf7507, (u_int)0xbbee99dd, (u_int)0xfda37f60,
    (u_int)0x9ff70126, (u_int)0xbc5c72f5, (u_int)0xc544663b, (u_int)0x345bfb7e,
    (u_int)0x768b4329, (u_int)0xdccb23c6, (u_int)0x68b6edfc, (u_int)0x63b8e4f1,
    (u_int)0xcad731dc, (u_int)0x10426385, (u_int)0x40139722, (u_int)0x2084c611,
    (u_int)0x7d854a24, (u_int)0xf8d2bb3d, (u_int)0x11aef932, (u_int)0x6dc729a1,
    (u_int)0x4b1d9e2f, (u_int)0xf3dcb230, (u_int)0xec0d8652, (u_int)0xd077c1e3,
    (u_int)0x6c2bb316, (u_int)0x99a970b9, (u_int)0xfa119448, (u_int)0x2247e964,
    (u_int)0xc4a8fc8c, (u_int)0x1aa0f03f, (u_int)0xd8567d2c, (u_int)0xef223390,
    (u_int)0xc787494e, (u_int)0xc1d938d1, (u_int)0xfe8ccaa2, (u_int)0x3698d40b,
    (u_int)0xcfa6f581, (u_int)0x28a57ade, (u_int)0x26dab78e, (u_int)0xa43fadbf,
    (u_int)0xe42c3a9d, (u_int)0x0d507892, (u_int)0x9b6a5fcc, (u_int)0x62547e46,
    (u_int)0xc2f68d13, (u_int)0xe890d8b8, (u_int)0x5e2e39f7, (u_int)0xf582c3af,
    (u_int)0xbe9f5d80, (u_int)0x7c69d093, (u_int)0xa96fd52d, (u_int)0xb3cf2512,
    (u_int)0x3bc8ac99, (u_int)0xa710187d, (u_int)0x6ee89c63, (u_int)0x7bdb3bbb,
    (u_int)0x09cd2678, (u_int)0xf46e5918, (u_int)0x01ec9ab7, (u_int)0xa8834f9a,
    (u_int)0x65e6956e, (u_int)0x7eaaffe6, (u_int)0x0821bccf, (u_int)0xe6ef15e8,
    (u_int)0xd9bae79b, (u_int)0xce4a6f36, (u_int)0xd4ea9f09, (u_int)0xd629b07c,
    (u_int)0xaf31a4b2, (u_int)0x312a3f23, (u_int)0x30c6a594, (u_int)0xc035a266,
    (u_int)0x37744ebc, (u_int)0xa6fc82ca, (u_int)0xb0e090d0, (u_int)0x1533a7d8,
    (u_int)0x4af10498, (u_int)0xf741ecda, (u_int)0x0e7fcd50, (u_int)0x2f1791f6,
    (u_int)0x8d764dd6, (u_int)0x4d43efb0, (u_int)0x54ccaa4d, (u_int)0xdfe49604,
    (u_int)0xe39ed1b5, (u_int)0x1b4c6a88, (u_int)0xb8c12c1f, (u_int)0x7f466551,
    (u_int)0x049d5eea, (u_int)0x5d018c35, (u_int)0x73fa8774, (u_int)0x2efb0b41,
    (u_int)0x5ab3671d, (u_int)0x5292dbd2, (u_int)0x33e91056, (u_int)0x136dd647,
    (u_int)0x8c9ad761, (u_int)0x7a37a10c, (u_int)0x8e59f814, (u_int)0x89eb133c,
    (u_int)0xeecea927, (u_int)0x35b761c9, (u_int)0xede11ce5, (u_int)0x3c7a47b1,
    (u_int)0x599cd2df, (u_int)0x3f55f273, (u_int)0x791814ce, (u_int)0xbf73c737,
    (u_int)0xea53f7cd, (u_int)0x5b5ffdaa, (u_int)0x14df3d6f, (u_int)0x867844db,
    (u_int)0x81caaff3, (u_int)0x3eb968c4, (u_int)0x2c382434, (u_int)0x5fc2a340,
    (u_int)0x72161dc3, (u_int)0x0cbce225, (u_int)0x8b283c49, (u_int)0x41ff0d95,
    (u_int)0x7139a801, (u_int)0xde080cb3, (u_int)0x9cd8b4e4, (u_int)0x906456c1,
    (u_int)0x617bcb84, (u_int)0x70d532b6, (u_int)0x74486c5c, (u_int)0x42d0b857,
};
static const u32 Td2[256] = {
    (u_int)0xa75051f4, (u_int)0x65537e41, (u_int)0xa4c31a17, (u_int)0x5e963a27,
    (u_int)0x6bcb3bab, (u_int)0x45f11f9d, (u_int)0x58abacfa, (u_int)0x03934be3,
    (u_int)0xfa552030, (u_int)0x6df6ad76, (u_int)0x769188cc, (u_int)0x4c25f502,
    (u_int)0xd7fc4fe5, (u_int)0xcbd7c52a, (u_int)0x44802635, (u_int)0xa38fb562,
    (u_int)0x5a49deb1, (u_int)0x1b6725ba, (u_int)0x0e9845ea, (u_int)0xc0e15dfe,
    (u_int)0x7502c32f, (u_int)0xf012814c, (u_int)0x97a38d46, (u_int)0xf9c66bd3,
    (u_int)0x5fe7038f, (u_int)0x9c951592, (u_int)0x7aebbf6d, (u_int)0x59da9552,
    (u_int)0x832dd4be, (u_int)0x21d35874, (u_int)0x692949e0, (u_int)0xc8448ec9,
    (u_int)0x896a75c2, (u_int)0x7978f48e, (u_int)0x3e6b9958, (u_int)0x71dd27b9,
    (u_int)0x4fb6bee1, (u_int)0xad17f088, (u_int)0xac66c920, (u_int)0x3ab47dce,
    (u_int)0x4a1863df, (u_int)0x3182e51a, (u_int)0x33609751, (u_int)0x7f456253,
    (u_int)0x77e0b164, (u_int)0xae84bb6b, (u_int)0xa01cfe81, (u_int)0x2b94f908,
    (u_int)0x68587048, (u_int)0xfd198f45, (u_int)0x6c8794de, (u_int)0xf8b7527b,
    (u_int)0xd323ab73, (u_int)0x02e2724b, (u_int)0x8f57e31f, (u_int)0xab2a6655,
    (u_int)0x2807b2eb, (u_int)0xc2032fb5, (u_int)0x7b9a86c5, (u_int)0x08a5d337,
    (u_int)0x87f23028, (u_int)0xa5b223bf, (u_int)0x6aba0203, (u_int)0x825ced16,
    (u_int)0x1c2b8acf, (u_int)0xb492a779, (u_int)0xf2f0f307, (u_int)0xe2a14e69,
    (u_int)0xf4cd65da, (u_int)0xbed50605, (u_int)0x621fd134, (u_int)0xfe8ac4a6,
    (u_int)0x539d342e, (u_int)0x55a0a2f3, (u_int)0xe132058a, (u_int)0xeb75a4f6,
    (u_int)0xec390b83, (u_int)0xefaa4060, (u_int)0x9f065e71, (u_int)0x1051bd6e,

    (u_int)0x8af93e21, (u_int)0x063d96dd, (u_int)0x05aedd3e, (u_int)0xbd464de6,
    (u_int)0x8db59154, (u_int)0x5d0571c4, (u_int)0xd46f0406, (u_int)0x15ff6050,
    (u_int)0xfb241998, (u_int)0xe997d6bd, (u_int)0x43cc8940, (u_int)0x9e7767d9,
    (u_int)0x42bdb0e8, (u_int)0x8b880789, (u_int)0x5b38e719, (u_int)0xeedb79c8,
    (u_int)0x0a47a17c, (u_int)0x0fe97c42, (u_int)0x1ec9f884, (u_int)0x00000000,
    (u_int)0x86830980, (u_int)0xed48322b, (u_int)0x70ac1e11, (u_int)0x724e6c5a,
    (u_int)0xfffbfd0e, (u_int)0x38560f85, (u_int)0xd51e3dae, (u_int)0x3927362d,
    (u_int)0xd9640a0f, (u_int)0xa621685c, (u_int)0x54d19b5b, (u_int)0x2e3a2436,
    (u_int)0x67b10c0a, (u_int)0xe70f9357, (u_int)0x96d2b4ee, (u_int)0x919e1b9b,
    (u_int)0xc54f80c0, (u_int)0x20a261dc, (u_int)0x4b695a77, (u_int)0x1a161c12,
    (u_int)0xba0ae293, (u_int)0x2ae5c0a0, (u_int)0xe0433c22, (u_int)0x171d121b,
    (u_int)0x0d0b0e09, (u_int)0xc7adf28b, (u_int)0xa8b92db6, (u_int)0xa9c8141e,
    (u_int)0x198557f1, (u_int)0x074caf75, (u_int)0xddbbee99, (u_int)0x60fda37f,
    (u_int)0x269ff701, (u_int)0xf5bc5c72, (u_int)0x3bc54466, (u_int)0x7e345bfb,
    (u_int)0x29768b43, (u_int)0xc6dccb23, (u_int)0xfc68b6ed, (u_int)0xf163b8e4,
    (u_int)0xdccad731, (u_int)0x85104263, (u_int)0x22401397, (u_int)0x112084c6,
    (u_int)0x247d854a, (u_int)0x3df8d2bb, (u_int)0x3211aef9, (u_int)0xa16dc729,
    (u_int)0x2f4b1d9e, (u_int)0x30f3dcb2, (u_int)0x52ec0d86, (u_int)0xe3d077c1,
    (u_int)0x166c2bb3, (u_int)0xb999a970, (u_int)0x48fa1194, (u_int)0x642247e9,
    (u_int)0x8cc4a8fc, (u_int)0x3f1aa0f0, (u_int)0x2cd8567d, (u_int)0x90ef2233,
    (u_int)0x4ec78749, (u_int)0xd1c1d938, (u_int)0xa2fe8cca, (u_int)0x0b3698d4,
    (u_int)0x81cfa6f5, (u_int)0xde28a57a, (u_int)0x8e26dab7, (u_int)0xbfa43fad,
    (u_int)0x9de42c3a, (u_int)0x920d5078, (u_int)0xcc9b6a5f, (u_int)0x4662547e,
    (u_int)0x13c2f68d, (u_int)0xb8e890d8, (u_int)0xf75e2e39, (u_int)0xaff582c3,
    (u_int)0x80be9f5d, (u_int)0x937c69d0, (u_int)0x2da96fd5, (u_int)0x12b3cf25,
    (u_int)0x993bc8ac, (u_int)0x7da71018, (u_int)0x636ee89c, (u_int)0xbb7bdb3b,
    (u_int)0x7809cd26, (u_int)0x18f46e59, (u_int)0xb701ec9a, (u_int)0x9aa8834f,
    (u_int)0x6e65e695, (u_int)0xe67eaaff, (u_int)0xcf0821bc, (u_int)0xe8e6ef15,
    (u_int)0x9bd9bae7, (u_int)0x36ce4a6f, (u_int)0x09d4ea9f, (u_int)0x7cd629b0,
    (u_int)0xb2af31a4, (u_int)0x23312a3f, (u_int)0x9430c6a5, (u_int)0x66c035a2,
    (u_int)0xbc37744e, (u_int)0xcaa6fc82, (u_int)0xd0b0e090, (u_int)0xd81533a7,
    (u_int)0x984af104, (u_int)0xdaf741ec, (u_int)0x500e7fcd, (u_int)0xf62f1791,
    (u_int)0xd68d764d, (u_int)0xb04d43ef, (u_int)0x4d54ccaa, (u_int)0x04dfe496,
    (u_int)0xb5e39ed1, (u_int)0x881b4c6a, (u_int)0x1fb8c12c, (u_int)0x517f4665,
    (u_int)0xea049d5e, (u_int)0x355d018c, (u_int)0x7473fa87, (u_int)0x412efb0b,
    (u_int)0x1d5ab367, (u_int)0xd25292db, (u_int)0x5633e910, (u_int)0x47136dd6,
    (u_int)0x618c9ad7, (u_int)0x0c7a37a1, (u_int)0x148e59f8, (u_int)0x3c89eb13,
    (u_int)0x27eecea9, (u_int)0xc935b761, (u_int)0xe5ede11c, (u_int)0xb13c7a47,
    (u_int)0xdf599cd2, (u_int)0x733f55f2, (u_int)0xce791814, (u_int)0x37bf73c7,
    (u_int)0xcdea53f7, (u_int)0xaa5b5ffd, (u_int)0x6f14df3d, (u_int)0xdb867844,
    (u_int)0xf381caaf, (u_int)0xc43eb968, (u_int)0x342c3824, (u_int)0x405fc2a3,
    (u_int)0xc372161d, (u_int)0x250cbce2, (u_int)0x498b283c, (u_int)0x9541ff0d,
    (u_int)0x017139a8, (u_int)0xb3de080c, (u_int)0xe49cd8b4, (u_int)0xc1906456,
    (u_int)0x84617bcb, (u_int)0xb670d532, (u_int)0x5c74486c, (u_int)0x5742d0b8,
};
static const u32 Td3[256] = {
    (u_int)0xf4a75051, (u_int)0x4165537e, (u_int)0x17a4c31a, (u_int)0x275e963a,
    (u_int)0xab6bcb3b, (u_int)0x9d45f11f, (u_int)0xfa58abac, (u_int)0xe303934b,
    (u_int)0x30fa5520, (u_int)0x766df6ad, (u_int)0xcc769188, (u_int)0x024c25f5,
    (u_int)0xe5d7fc4f, (u_int)0x2acbd7c5, (u_int)0x35448026, (u_int)0x62a38fb5,
    (u_int)0xb15a49de, (u_int)0xba1b6725, (u_int)0xea0e9845, (u_int)0xfec0e15d,
    (u_int)0x2f7502c3, (u_int)0x4cf01281, (u_int)0x4697a38d, (u_int)0xd3f9c66b,
    (u_int)0x8f5fe703, (u_int)0x929c9515, (u_int)0x6d7aebbf, (u_int)0x5259da95,
    (u_int)0xbe832dd4, (u_int)0x7421d358, (u_int)0xe0692949, (u_int)0xc9c8448e,
    (u_int)0xc2896a75, (u_int)0x8e7978f4, (u_int)0x583e6b99, (u_int)0xb971dd27,
    (u_int)0xe14fb6be, (u_int)0x88ad17f0, (u_int)0x20ac66c9, (u_int)0xce3ab47d,
    (u_int)0xdf4a1863, (u_int)0x1a3182e5, (u_int)0x51336097, (u_int)0x537f4562,
    (u_int)0x6477e0b1, (u_int)0x6bae84bb, (u_int)0x81a01cfe, (u_int)0x082b94f9,
    (u_int)0x48685870, (u_int)0x45fd198f, (u_int)0xde6c8794, (u_int)0x7bf8b752,
    (u_int)0x73d323ab, (u_int)0x4b02e272, (u_int)0x1f8f57e3, (u_int)0x55ab2a66,
    (u_int)0xeb2807b2, (u_int)0xb5c2032f, (u_int)0xc57b9a86, (u_int)0x3708a5d3,
    (u_int)0x2887f230, (u_int)0xbfa5b223, (u_int)0x036aba02, (u_int)0x16825ced,
    (u_int)0xcf1c2b8a, (u_int)0x79b492a7, (u_int)0x07f2f0f3, (u_int)0x69e2a14e,
    (u_int)0xdaf4cd65, (u_int)0x05bed506, (u_int)0x34621fd1, (u_int)0xa6fe8ac4,
    (u_int)0x2e539d34, (u_int)0xf355a0a2, (u_int)0x8ae13205, (u_int)0xf6eb75a4,
    (u_int)0x83ec390b, (u_int)0x60efaa40, (u_int)0x719f065e, (u_int)0x6e1051bd,
    (u_int)0x218af93e, (u_int)0xdd063d96, (u_int)0x3e05aedd, (u_int)0xe6bd464d,
    (u_int)0x548db591, (u_int)0xc45d0571, (u_int)0x06d46f04, (u_int)0x5015ff60,
    (u_int)0x98fb2419, (u_int)0xbde997d6, (u_int)0x4043cc89, (u_int)0xd99e7767,
    (u_int)0xe842bdb0, (u_int)0x898b8807, (u_int)0x195b38e7, (u_int)0xc8eedb79,
    (u_int)0x7c0a47a1, (u_int)0x420fe97c, (u_int)0x841ec9f8, (u_int)0x00000000,
    (u_int)0x80868309, (u_int)0x2bed4832, (u_int)0x1170ac1e, (u_int)0x5a724e6c,
    (u_int)0x0efffbfd, (u_int)0x8538560f, (u_int)0xaed51e3d, (u_int)0x2d392736,
    (u_int)0x0fd9640a, (u_int)0x5ca62168, (u_int)0x5b54d19b, (u_int)0x362e3a24,
    (u_int)0x0a67b10c, (u_int)0x57e70f93, (u_int)0xee96d2b4, (u_int)0x9b919e1b,
    (u_int)0xc0c54f80, (u_int)0xdc20a261, (u_int)0x774b695a, (u_int)0x121a161c,
    (u_int)0x93ba0ae2, (u_int)0xa02ae5c0, (u_int)0x22e0433c, (u_int)0x1b171d12,
    (u_int)0x090d0b0e, (u_int)0x8bc7adf2, (u_int)0xb6a8b92d, (u_int)0x1ea9c814,
    (u_int)0xf1198557, (u_int)0x75074caf, (u_int)0x99ddbbee, (u_int)0x7f60fda3,
    (u_int)0x01269ff7, (u_int)0x72f5bc5c, (u_int)0x663bc544, (u_int)0xfb7e345b,
    (u_int)0x4329768b, (u_int)0x23c6dccb, (u_int)0xedfc68b6, (u_int)0xe4f163b8,
    (u_int)0x31dccad7, (u_int)0x63851042, (u_int)0x97224013, (u_int)0xc6112084,
    (u_int)0x4a247d85, (u_int)0xbb3df8d2, (u_int)0xf93211ae, (u_int)0x29a16dc7,
    (u_int)0x9e2f4b1d, (u_int)0xb230f3dc, (u_int)0x8652ec0d, (u_int)0xc1e3d077,
    (u_int)0xb3166c2b, (u_int)0x70b999a9, (u_int)0x9448fa11, (u_int)0xe9642247,
    (u_int)0xfc8cc4a8, (u_int)0xf03f1aa0, (u_int)0x7d2cd856, (u_int)0x3390ef22,
    (u_int)0x494ec787, (u_int)0x38d1c1d9, (u_int)0xcaa2fe8c, (u_int)0xd40b3698,
    (u_int)0xf581cfa6, (u_int)0x7ade28a5, (u_int)0xb78e26da, (u_int)0xadbfa43f,
    (u_int)0x3a9de42c, (u_int)0x78920d50, (u_int)0x5fcc9b6a, (u_int)0x7e466254,
    (u_int)0x8d13c2f6, (u_int)0xd8b8e890, (u_int)0x39f75e2e, (u_int)0xc3aff582,
    (u_int)0x5d80be9f, (u_int)0xd0937c69, (u_int)0xd52da96f, (u_int)0x2512b3cf,
    (u_int)0xac993bc8, (u_int)0x187da710, (u_int)0x9c636ee8, (u_int)0x3bbb7bdb,
    (u_int)0x267809cd, (u_int)0x5918f46e, (u_int)0x9ab701ec, (u_int)0x4f9aa883,
    (u_int)0x956e65e6, (u_int)0xffe67eaa, (u_int)0xbccf0821, (u_int)0x15e8e6ef,
    (u_int)0xe79bd9ba, (u_int)0x6f36ce4a, (u_int)0x9f09d4ea, (u_int)0xb07cd629,
    (u_int)0xa4b2af31, (u_int)0x3f23312a, (u_int)0xa59430c6, (u_int)0xa266c035,
    (u_int)0x4ebc3774, (u_int)0x82caa6fc, (u_int)0x90d0b0e0, (u_int)0xa7d81533,
    (u_int)0x04984af1, (u_int)0xecdaf741, (u_int)0xcd500e7f, (u_int)0x91f62f17,
    (u_int)0x4dd68d76, (u_int)0xefb04d43, (u_int)0xaa4d54cc, (u_int)0x9604dfe4,
    (u_int)0xd1b5e39e, (u_int)0x6a881b4c, (u_int)0x2c1fb8c1, (u_int)0x65517f46,
    (u_int)0x5eea049d, (u_int)0x8c355d01, (u_int)0x877473fa, (u_int)0x0b412efb,
    (u_int)0x671d5ab3, (u_int)0xdbd25292, (u_int)0x105633e9, (u_int)0xd647136d,
    (u_int)0xd7618c9a, (u_int)0xa10c7a37, (u_int)0xf8148e59, (u_int)0x133c89eb,
    (u_int)0xa927eece, (u_int)0x61c935b7, (u_int)0x1ce5ede1, (u_int)0x47b13c7a,
    (u_int)0xd2df599c, (u_int)0xf2733f55, (u_int)0x14ce7918, (u_int)0xc737bf73,
    (u_int)0xf7cdea53, (u_int)0xfdaa5b5f, (u_int)0x3d6f14df, (u_int)0x44db8678,
    (u_int)0xaff381ca, (u_int)0x68c43eb9, (u_int)0x24342c38, (u_int)0xa3405fc2,
    (u_int)0x1dc37216, (u_int)0xe2250cbc, (u_int)0x3c498b28, (u_int)0x0d9541ff,
    (u_int)0xa8017139, (u_int)0x0cb3de08, (u_int)0xb4e49cd8, (u_int)0x56c19064,
    (u_int)0xcb84617b, (u_int)0x32b670d5, (u_int)0x6c5c7448, (u_int)0xb85742d0,
};
static const u32 Td4[256] = {
    (u_int)0x52525252, (u_int)0x09090909, (u_int)0x6a6a6a6a, (u_int)0xd5d5d5d5,
    (u_int)0x30303030, (u_int)0x36363636, (u_int)0xa5a5a5a5, (u_int)0x38383838,
    (u_int)0xbfbfbfbf, (u_int)0x40404040, (u_int)0xa3a3a3a3, (u_int)0x9e9e9e9e,
    (u_int)0x81818181, (u_int)0xf3f3f3f3, (u_int)0xd7d7d7d7, (u_int)0xfbfbfbfb,
    (u_int)0x7c7c7c7c, (u_int)0xe3e3e3e3, (u_int)0x39393939, (u_int)0x82828282,
    (u_int)0x9b9b9b9b, (u_int)0x2f2f2f2f, (u_int)0xffffffff, (u_int)0x87878787,
    (u_int)0x34343434, (u_int)0x8e8e8e8e, (u_int)0x43434343, (u_int)0x44444444,
    (u_int)0xc4c4c4c4, (u_int)0xdededede, (u_int)0xe9e9e9e9, (u_int)0xcbcbcbcb,
    (u_int)0x54545454, (u_int)0x7b7b7b7b, (u_int)0x94949494, (u_int)0x32323232,
    (u_int)0xa6a6a6a6, (u_int)0xc2c2c2c2, (u_int)0x23232323, (u_int)0x3d3d3d3d,
    (u_int)0xeeeeeeee, (u_int)0x4c4c4c4c, (u_int)0x95959595, (u_int)0x0b0b0b0b,
    (u_int)0x42424242, (u_int)0xfafafafa, (u_int)0xc3c3c3c3, (u_int)0x4e4e4e4e,
    (u_int)0x08080808, (u_int)0x2e2e2e2e, (u_int)0xa1a1a1a1, (u_int)0x66666666,
    (u_int)0x28282828, (u_int)0xd9d9d9d9, (u_int)0x24242424, (u_int)0xb2b2b2b2,
    (u_int)0x76767676, (u_int)0x5b5b5b5b, (u_int)0xa2a2a2a2, (u_int)0x49494949,
    (u_int)0x6d6d6d6d, (u_int)0x8b8b8b8b, (u_int)0xd1d1d1d1, (u_int)0x25252525,
    (u_int)0x72727272, (u_int)0xf8f8f8f8, (u_int)0xf6f6f6f6, (u_int)0x64646464,
    (u_int)0x86868686, (u_int)0x68686868, (u_int)0x98989898, (u_int)0x16161616,
    (u_int)0xd4d4d4d4, (u_int)0xa4a4a4a4, (u_int)0x5c5c5c5c, (u_int)0xcccccccc,
    (u_int)0x5d5d5d5d, (u_int)0x65656565, (u_int)0xb6b6b6b6, (u_int)0x92929292,
    (u_int)0x6c6c6c6c, (u_int)0x70707070, (u_int)0x48484848, (u_int)0x50505050,
    (u_int)0xfdfdfdfd, (u_int)0xedededed, (u_int)0xb9b9b9b9, (u_int)0xdadadada,
    (u_int)0x5e5e5e5e, (u_int)0x15151515, (u_int)0x46464646, (u_int)0x57575757,
    (u_int)0xa7a7a7a7, (u_int)0x8d8d8d8d, (u_int)0x9d9d9d9d, (u_int)0x84848484,
    (u_int)0x90909090, (u_int)0xd8d8d8d8, (u_int)0xabababab, (u_int)0x00000000,
    (u_int)0x8c8c8c8c, (u_int)0xbcbcbcbc, (u_int)0xd3d3d3d3, (u_int)0x0a0a0a0a,
    (u_int)0xf7f7f7f7, (u_int)0xe4e4e4e4, (u_int)0x58585858, (u_int)0x05050505,
    (u_int)0xb8b8b8b8, (u_int)0xb3b3b3b3, (u_int)0x45454545, (u_int)0x06060606,
    (u_int)0xd0d0d0d0, (u_int)0x2c2c2c2c, (u_int)0x1e1e1e1e, (u_int)0x8f8f8f8f,
    (u_int)0xcacacaca, (u_int)0x3f3f3f3f, (u_int)0x0f0f0f0f, (u_int)0x02020202,
    (u_int)0xc1c1c1c1, (u_int)0xafafafaf, (u_int)0xbdbdbdbd, (u_int)0x03030303,
    (u_int)0x01010101, (u_int)0x13131313, (u_int)0x8a8a8a8a, (u_int)0x6b6b6b6b,
    (u_int)0x3a3a3a3a, (u_int)0x91919191, (u_int)0x11111111, (u_int)0x41414141,
    (u_int)0x4f4f4f4f, (u_int)0x67676767, (u_int)0xdcdcdcdc, (u_int)0xeaeaeaea,
    (u_int)0x97979797, (u_int)0xf2f2f2f2, (u_int)0xcfcfcfcf, (u_int)0xcececece,
    (u_int)0xf0f0f0f0, (u_int)0xb4b4b4b4, (u_int)0xe6e6e6e6, (u_int)0x73737373,
    (u_int)0x96969696, (u_int)0xacacacac, (u_int)0x74747474, (u_int)0x22222222,
    (u_int)0xe7e7e7e7, (u_int)0xadadadad, (u_int)0x35353535, (u_int)0x85858585,
    (u_int)0xe2e2e2e2, (u_int)0xf9f9f9f9, (u_int)0x37373737, (u_int)0xe8e8e8e8,
    (u_int)0x1c1c1c1c, (u_int)0x75757575, (u_int)0xdfdfdfdf, (u_int)0x6e6e6e6e,
    (u_int)0x47474747, (u_int)0xf1f1f1f1, (u_int)0x1a1a1a1a, (u_int)0x71717171,
    (u_int)0x1d1d1d1d, (u_int)0x29292929, (u_int)0xc5c5c5c5, (u_int)0x89898989,
    (u_int)0x6f6f6f6f, (u_int)0xb7b7b7b7, (u_int)0x62626262, (u_int)0x0e0e0e0e,
    (u_int)0xaaaaaaaa, (u_int)0x18181818, (u_int)0xbebebebe, (u_int)0x1b1b1b1b,
    (u_int)0xfcfcfcfc, (u_int)0x56565656, (u_int)0x3e3e3e3e, (u_int)0x4b4b4b4b,
    (u_int)0xc6c6c6c6, (u_int)0xd2d2d2d2, (u_int)0x79797979, (u_int)0x20202020,
    (u_int)0x9a9a9a9a, (u_int)0xdbdbdbdb, (u_int)0xc0c0c0c0, (u_int)0xfefefefe,
    (u_int)0x78787878, (u_int)0xcdcdcdcd, (u_int)0x5a5a5a5a, (u_int)0xf4f4f4f4,
    (u_int)0x1f1f1f1f, (u_int)0xdddddddd, (u_int)0xa8a8a8a8, (u_int)0x33333333,
    (u_int)0x88888888, (u_int)0x07070707, (u_int)0xc7c7c7c7, (u_int)0x31313131,
    (u_int)0xb1b1b1b1, (u_int)0x12121212, (u_int)0x10101010, (u_int)0x59595959,
    (u_int)0x27272727, (u_int)0x80808080, (u_int)0xecececec, (u_int)0x5f5f5f5f,
    (u_int)0x60606060, (u_int)0x51515151, (u_int)0x7f7f7f7f, (u_int)0xa9a9a9a9,
    (u_int)0x19191919, (u_int)0xb5b5b5b5, (u_int)0x4a4a4a4a, (u_int)0x0d0d0d0d,
    (u_int)0x2d2d2d2d, (u_int)0xe5e5e5e5, (u_int)0x7a7a7a7a, (u_int)0x9f9f9f9f,
    (u_int)0x93939393, (u_int)0xc9c9c9c9, (u_int)0x9c9c9c9c, (u_int)0xefefefef,
    (u_int)0xa0a0a0a0, (u_int)0xe0e0e0e0, (u_int)0x3b3b3b3b, (u_int)0x4d4d4d4d,
    (u_int)0xaeaeaeae, (u_int)0x2a2a2a2a, (u_int)0xf5f5f5f5, (u_int)0xb0b0b0b0,
    (u_int)0xc8c8c8c8, (u_int)0xebebebeb, (u_int)0xbbbbbbbb, (u_int)0x3c3c3c3c,
    (u_int)0x83838383, (u_int)0x53535353, (u_int)0x99999999, (u_int)0x61616161,
    (u_int)0x17171717, (u_int)0x2b2b2b2b, (u_int)0x04040404, (u_int)0x7e7e7e7e,
    (u_int)0xbabababa, (u_int)0x77777777, (u_int)0xd6d6d6d6, (u_int)0x26262626,
    (u_int)0xe1e1e1e1, (u_int)0x69696969, (u_int)0x14141414, (u_int)0x63636363,
    (u_int)0x55555555, (u_int)0x21212121, (u_int)0x0c0c0c0c, (u_int)0x7d7d7d7d,
};
static const u32 rcon[] = {
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x1B000000, 0x36000000, /* for 128-bit blocks, Rijndael never uses more than 10 rcon values */
};

#define	SWAP(x) (_lrotl(x, 8) & 0x00ff00ff | _lrotr(x, 8) & 0xff00ff00)

#ifdef _MSC_VER
#define	GETU32(p) SWAP(*((u32 *)(p)))
#define	PUTU32(ct, st) { *((u32 *)(ct)) = SWAP((st)); }
#else
#define	GETU32(pt) (((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] <<  8) ^ ((u32)(pt)[3]))
#define	PUTU32(ct, st) { (ct)[0] = (u8)((st) >> 24); (ct)[1] = (u8)((st) >> 16); (ct)[2] = (u8)((st) >>  8); (ct)[3] = (u8)(st); }
#endif

/**
 * Expand the cipher key into the encryption key schedule.
 *
 * @return	the number of rounds for the given cipher key size.
 */
/*
 * __db_rijndaelKeySetupEnc --
 *
 * PUBLIC: int __db_rijndaelKeySetupEnc __P((u32 *, const u8 *, int));
 */
int
__db_rijndaelKeySetupEnc(rk, cipherKey, keyBits)
	u32 *rk;	/* rk[4*(Nr + 1)] */
	const u8 *cipherKey;
	int keyBits;
{
	int i = 0;
	u32 temp;

	rk[0] = GETU32(cipherKey     );
	rk[1] = GETU32(cipherKey +  4);
	rk[2] = GETU32(cipherKey +  8);
	rk[3] = GETU32(cipherKey + 12);
	if (keyBits == 128) {
		for (;;) {
			temp  = rk[3];
			rk[4] = rk[0] ^
				(Te4[(temp >> 16) & 0xff] & 0xff000000) ^
				(Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
				(Te4[(temp      ) & 0xff] & 0x0000ff00) ^
				(Te4[(temp >> 24)       ] & 0x000000ff) ^
				rcon[i];
			rk[5] = rk[1] ^ rk[4];
			rk[6] = rk[2] ^ rk[5];
			rk[7] = rk[3] ^ rk[6];
			if (++i == 10) {
				return 10;
			}
			rk += 4;
		}
	}
	rk[4] = GETU32(cipherKey + 16);
	rk[5] = GETU32(cipherKey + 20);
	if (keyBits == 192) {
		for (;;) {
			temp = rk[ 5];
			rk[ 6] = rk[ 0] ^
				(Te4[(temp >> 16) & 0xff] & 0xff000000) ^
				(Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
				(Te4[(temp      ) & 0xff] & 0x0000ff00) ^
				(Te4[(temp >> 24)       ] & 0x000000ff) ^
				rcon[i];
			rk[ 7] = rk[ 1] ^ rk[ 6];
			rk[ 8] = rk[ 2] ^ rk[ 7];
			rk[ 9] = rk[ 3] ^ rk[ 8];
			if (++i == 8) {
				return 12;
			}
			rk[10] = rk[ 4] ^ rk[ 9];
			rk[11] = rk[ 5] ^ rk[10];
			rk += 6;
		}
	}
	rk[6] = GETU32(cipherKey + 24);
	rk[7] = GETU32(cipherKey + 28);
	if (keyBits == 256) {
	for (;;) {
		temp = rk[ 7];
		rk[ 8] = rk[ 0] ^
			(Te4[(temp >> 16) & 0xff] & 0xff000000) ^
			(Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
			(Te4[(temp      ) & 0xff] & 0x0000ff00) ^
			(Te4[(temp >> 24)       ] & 0x000000ff) ^
			rcon[i];
		rk[ 9] = rk[ 1] ^ rk[ 8];
		rk[10] = rk[ 2] ^ rk[ 9];
		rk[11] = rk[ 3] ^ rk[10];
			if (++i == 7) {
				return 14;
			}
		temp = rk[11];
		rk[12] = rk[ 4] ^
			(Te4[(temp >> 24)       ] & 0xff000000) ^
			(Te4[(temp >> 16) & 0xff] & 0x00ff0000) ^
			(Te4[(temp >>  8) & 0xff] & 0x0000ff00) ^
			(Te4[(temp      ) & 0xff] & 0x000000ff);
		rk[13] = rk[ 5] ^ rk[12];
		rk[14] = rk[ 6] ^ rk[13];
		rk[15] = rk[ 7] ^ rk[14];

			rk += 8;
	}
	}
	return 0;
}

/**
 * Expand the cipher key into the decryption key schedule.
 *
 * @return	the number of rounds for the given cipher key size.
 */
/*
 * __db_rijndaelKeySetupDec --
 *
 * PUBLIC: int __db_rijndaelKeySetupDec __P((u32 *, const u8 *, int));
 */
int
__db_rijndaelKeySetupDec(rk, cipherKey, keyBits)
	u32 *rk;	/* rk[4*(Nr + 1)] */
	const u8 *cipherKey;
	int keyBits;
{
	int Nr, i, j;
	u32 temp;

	/* expand the cipher key: */
	Nr = __db_rijndaelKeySetupEnc(rk, cipherKey, keyBits);
	/* invert the order of the round keys: */
	for (i = 0, j = 4*Nr; i < j; i += 4, j -= 4) {
		temp = rk[i    ]; rk[i    ] = rk[j    ]; rk[j    ] = temp;
		temp = rk[i + 1]; rk[i + 1] = rk[j + 1]; rk[j + 1] = temp;
		temp = rk[i + 2]; rk[i + 2] = rk[j + 2]; rk[j + 2] = temp;
		temp = rk[i + 3]; rk[i + 3] = rk[j + 3]; rk[j + 3] = temp;
	}
	/* apply the inverse MixColumn transform to all round keys but the first and the last: */
	for (i = 1; i < Nr; i++) {
		rk += 4;
		rk[0] =
			Td0[Te4[(rk[0] >> 24)       ] & 0xff] ^
			Td1[Te4[(rk[0] >> 16) & 0xff] & 0xff] ^
			Td2[Te4[(rk[0] >>  8) & 0xff] & 0xff] ^
			Td3[Te4[(rk[0]      ) & 0xff] & 0xff];
		rk[1] =
			Td0[Te4[(rk[1] >> 24)       ] & 0xff] ^
			Td1[Te4[(rk[1] >> 16) & 0xff] & 0xff] ^
			Td2[Te4[(rk[1] >>  8) & 0xff] & 0xff] ^
			Td3[Te4[(rk[1]      ) & 0xff] & 0xff];
		rk[2] =
			Td0[Te4[(rk[2] >> 24)       ] & 0xff] ^
			Td1[Te4[(rk[2] >> 16) & 0xff] & 0xff] ^
			Td2[Te4[(rk[2] >>  8) & 0xff] & 0xff] ^
			Td3[Te4[(rk[2]      ) & 0xff] & 0xff];
		rk[3] =
			Td0[Te4[(rk[3] >> 24)       ] & 0xff] ^
			Td1[Te4[(rk[3] >> 16) & 0xff] & 0xff] ^
			Td2[Te4[(rk[3] >>  8) & 0xff] & 0xff] ^
			Td3[Te4[(rk[3]      ) & 0xff] & 0xff];
	}
	return Nr;
}

/*
 * __db_rijndaelEncrypt --
 *
 * PUBLIC: void __db_rijndaelEncrypt __P((u32 *, int, const u8 *, u8 *));
 */
void
__db_rijndaelEncrypt(rk, Nr, pt, ct)
	u32 *rk;	/* rk[4*(Nr + 1)] */
	int Nr;
	const u8 *pt;
	u8 *ct;
{
	u32 s0, s1, s2, s3, t0, t1, t2, t3;
#ifndef FULL_UNROLL
    int r;
#endif /* ?FULL_UNROLL */

    /*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(pt     ) ^ rk[0];
	s1 = GETU32(pt +  4) ^ rk[1];
	s2 = GETU32(pt +  8) ^ rk[2];
	s3 = GETU32(pt + 12) ^ rk[3];
#ifdef FULL_UNROLL
    /* round 1: */
	t0 = Te0[s0 >> 24] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >>  8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[ 4];
	t1 = Te0[s1 >> 24] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >>  8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[ 5];
	t2 = Te0[s2 >> 24] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >>  8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[ 6];
	t3 = Te0[s3 >> 24] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >>  8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[ 7];
	/* round 2: */
	s0 = Te0[t0 >> 24] ^ Te1[(t1 >> 16) & 0xff] ^ Te2[(t2 >>  8) & 0xff] ^ Te3[t3 & 0xff] ^ rk[ 8];
	s1 = Te0[t1 >> 24] ^ Te1[(t2 >> 16) & 0xff] ^ Te2[(t3 >>  8) & 0xff] ^ Te3[t0 & 0xff] ^ rk[ 9];
	s2 = Te0[t2 >> 24] ^ Te1[(t3 >> 16) & 0xff] ^ Te2[(t0 >>  8) & 0xff] ^ Te3[t1 & 0xff] ^ rk[10];
	s3 = Te0[t3 >> 24] ^ Te1[(t0 >> 16) & 0xff] ^ Te2[(t1 >>  8) & 0xff] ^ Te3[t2 & 0xff] ^ rk[11];
    /* round 3: */
	t0 = Te0[s0 >> 24] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >>  8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[12];
	t1 = Te0[s1 >> 24] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >>  8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[13];
	t2 = Te0[s2 >> 24] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >>  8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[14];
	t3 = Te0[s3 >> 24] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >>  8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[15];
	/* round 4: */
	s0 = Te0[t0 >> 24] ^ Te1[(t1 >> 16) & 0xff] ^ Te2[(t2 >>  8) & 0xff] ^ Te3[t3 & 0xff] ^ rk[16];
	s1 = Te0[t1 >> 24] ^ Te1[(t2 >> 16) & 0xff] ^ Te2[(t3 >>  8) & 0xff] ^ Te3[t0 & 0xff] ^ rk[17];
	s2 = Te0[t2 >> 24] ^ Te1[(t3 >> 16) & 0xff] ^ Te2[(t0 >>  8) & 0xff] ^ Te3[t1 & 0xff] ^ rk[18];
	s3 = Te0[t3 >> 24] ^ Te1[(t0 >> 16) & 0xff] ^ Te2[(t1 >>  8) & 0xff] ^ Te3[t2 & 0xff] ^ rk[19];
    /* round 5: */
	t0 = Te0[s0 >> 24] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >>  8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[20];
	t1 = Te0[s1 >> 24] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >>  8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[21];
	t2 = Te0[s2 >> 24] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >>  8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[22];
	t3 = Te0[s3 >> 24] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >>  8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[23];
	/* round 6: */
	s0 = Te0[t0 >> 24] ^ Te1[(t1 >> 16) & 0xff] ^ Te2[(t2 >>  8) & 0xff] ^ Te3[t3 & 0xff] ^ rk[24];
	s1 = Te0[t1 >> 24] ^ Te1[(t2 >> 16) & 0xff] ^ Te2[(t3 >>  8) & 0xff] ^ Te3[t0 & 0xff] ^ rk[25];
	s2 = Te0[t2 >> 24] ^ Te1[(t3 >> 16) & 0xff] ^ Te2[(t0 >>  8) & 0xff] ^ Te3[t1 & 0xff] ^ rk[26];
	s3 = Te0[t3 >> 24] ^ Te1[(t0 >> 16) & 0xff] ^ Te2[(t1 >>  8) & 0xff] ^ Te3[t2 & 0xff] ^ rk[27];
    /* round 7: */
	t0 = Te0[s0 >> 24] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >>  8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[28];
	t1 = Te0[s1 >> 24] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >>  8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[29];
	t2 = Te0[s2 >> 24] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >>  8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[30];
	t3 = Te0[s3 >> 24] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >>  8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[31];
	/* round 8: */
	s0 = Te0[t0 >> 24] ^ Te1[(t1 >> 16) & 0xff] ^ Te2[(t2 >>  8) & 0xff] ^ Te3[t3 & 0xff] ^ rk[32];
	s1 = Te0[t1 >> 24] ^ Te1[(t2 >> 16) & 0xff] ^ Te2[(t3 >>  8) & 0xff] ^ Te3[t0 & 0xff] ^ rk[33];
	s2 = Te0[t2 >> 24] ^ Te1[(t3 >> 16) & 0xff] ^ Te2[(t0 >>  8) & 0xff] ^ Te3[t1 & 0xff] ^ rk[34];
	s3 = Te0[t3 >> 24] ^ Te1[(t0 >> 16) & 0xff] ^ Te2[(t1 >>  8) & 0xff] ^ Te3[t2 & 0xff] ^ rk[35];
    /* round 9: */
	t0 = Te0[s0 >> 24] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >>  8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[36];
	t1 = Te0[s1 >> 24] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >>  8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[37];
	t2 = Te0[s2 >> 24] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >>  8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[38];
	t3 = Te0[s3 >> 24] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >>  8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[39];
    if (Nr > 10) {
	/* round 10: */
	s0 = Te0[t0 >> 24] ^ Te1[(t1 >> 16) & 0xff] ^ Te2[(t2 >>  8) & 0xff] ^ Te3[t3 & 0xff] ^ rk[40];
	s1 = Te0[t1 >> 24] ^ Te1[(t2 >> 16) & 0xff] ^ Te2[(t3 >>  8) & 0xff] ^ Te3[t0 & 0xff] ^ rk[41];
	s2 = Te0[t2 >> 24] ^ Te1[(t3 >> 16) & 0xff] ^ Te2[(t0 >>  8) & 0xff] ^ Te3[t1 & 0xff] ^ rk[42];
	s3 = Te0[t3 >> 24] ^ Te1[(t0 >> 16) & 0xff] ^ Te2[(t1 >>  8) & 0xff] ^ Te3[t2 & 0xff] ^ rk[43];
	/* round 11: */
	t0 = Te0[s0 >> 24] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >>  8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[44];
	t1 = Te0[s1 >> 24] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >>  8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[45];
	t2 = Te0[s2 >> 24] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >>  8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[46];
	t3 = Te0[s3 >> 24] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >>  8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[47];
	if (Nr > 12) {
	    /* round 12: */
	    s0 = Te0[t0 >> 24] ^ Te1[(t1 >> 16) & 0xff] ^ Te2[(t2 >>  8) & 0xff] ^ Te3[t3 & 0xff] ^ rk[48];
	    s1 = Te0[t1 >> 24] ^ Te1[(t2 >> 16) & 0xff] ^ Te2[(t3 >>  8) & 0xff] ^ Te3[t0 & 0xff] ^ rk[49];
	    s2 = Te0[t2 >> 24] ^ Te1[(t3 >> 16) & 0xff] ^ Te2[(t0 >>  8) & 0xff] ^ Te3[t1 & 0xff] ^ rk[50];
	    s3 = Te0[t3 >> 24] ^ Te1[(t0 >> 16) & 0xff] ^ Te2[(t1 >>  8) & 0xff] ^ Te3[t2 & 0xff] ^ rk[51];
	    /* round 13: */
	    t0 = Te0[s0 >> 24] ^ Te1[(s1 >> 16) & 0xff] ^ Te2[(s2 >>  8) & 0xff] ^ Te3[s3 & 0xff] ^ rk[52];
	    t1 = Te0[s1 >> 24] ^ Te1[(s2 >> 16) & 0xff] ^ Te2[(s3 >>  8) & 0xff] ^ Te3[s0 & 0xff] ^ rk[53];
	    t2 = Te0[s2 >> 24] ^ Te1[(s3 >> 16) & 0xff] ^ Te2[(s0 >>  8) & 0xff] ^ Te3[s1 & 0xff] ^ rk[54];
	    t3 = Te0[s3 >> 24] ^ Te1[(s0 >> 16) & 0xff] ^ Te2[(s1 >>  8) & 0xff] ^ Te3[s2 & 0xff] ^ rk[55];
	}
    }
    rk += Nr << 2;
#else  /* !FULL_UNROLL */
    /*
	 * Nr - 1 full rounds:
	 */
    r = Nr >> 1;
    for (;;) {
	t0 =
	    Te0[(s0 >> 24)       ] ^
	    Te1[(s1 >> 16) & 0xff] ^
	    Te2[(s2 >>  8) & 0xff] ^
	    Te3[(s3      ) & 0xff] ^
	    rk[4];
	t1 =
	    Te0[(s1 >> 24)       ] ^
	    Te1[(s2 >> 16) & 0xff] ^
	    Te2[(s3 >>  8) & 0xff] ^
	    Te3[(s0      ) & 0xff] ^
	    rk[5];
	t2 =
	    Te0[(s2 >> 24)       ] ^
	    Te1[(s3 >> 16) & 0xff] ^
	    Te2[(s0 >>  8) & 0xff] ^
	    Te3[(s1      ) & 0xff] ^
	    rk[6];
	t3 =
	    Te0[(s3 >> 24)       ] ^
	    Te1[(s0 >> 16) & 0xff] ^
	    Te2[(s1 >>  8) & 0xff] ^
	    Te3[(s2      ) & 0xff] ^
	    rk[7];

	rk += 8;
	if (--r == 0) {
	    break;
	}

	s0 =
	    Te0[(t0 >> 24)       ] ^
	    Te1[(t1 >> 16) & 0xff] ^
	    Te2[(t2 >>  8) & 0xff] ^
	    Te3[(t3      ) & 0xff] ^
	    rk[0];
	s1 =
	    Te0[(t1 >> 24)       ] ^
	    Te1[(t2 >> 16) & 0xff] ^
	    Te2[(t3 >>  8) & 0xff] ^
	    Te3[(t0      ) & 0xff] ^
	    rk[1];
	s2 =
	    Te0[(t2 >> 24)       ] ^
	    Te1[(t3 >> 16) & 0xff] ^
	    Te2[(t0 >>  8) & 0xff] ^
	    Te3[(t1      ) & 0xff] ^
	    rk[2];
	s3 =
	    Te0[(t3 >> 24)       ] ^
	    Te1[(t0 >> 16) & 0xff] ^
	    Te2[(t1 >>  8) & 0xff] ^
	    Te3[(t2      ) & 0xff] ^
	    rk[3];
    }
#endif /* ?FULL_UNROLL */
    /*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	s0 =
		(Te4[(t0 >> 24)       ] & 0xff000000) ^
		(Te4[(t1 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(t2 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(t3      ) & 0xff] & 0x000000ff) ^
		rk[0];
	PUTU32(ct     , s0);
	s1 =
		(Te4[(t1 >> 24)       ] & 0xff000000) ^
		(Te4[(t2 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(t3 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(t0      ) & 0xff] & 0x000000ff) ^
		rk[1];
	PUTU32(ct +  4, s1);
	s2 =
		(Te4[(t2 >> 24)       ] & 0xff000000) ^
		(Te4[(t3 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(t0 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(t1      ) & 0xff] & 0x000000ff) ^
		rk[2];
	PUTU32(ct +  8, s2);
	s3 =
		(Te4[(t3 >> 24)       ] & 0xff000000) ^
		(Te4[(t0 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(t1 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(t2      ) & 0xff] & 0x000000ff) ^
		rk[3];
	PUTU32(ct + 12, s3);
}

/*
 * __db_rijndaelDecrypt --
 *
 * PUBLIC: void __db_rijndaelDecrypt __P((u32 *, int, const u8 *, u8 *));
 */
void
__db_rijndaelDecrypt(rk, Nr, ct, pt)
	u32 *rk;	/* rk[4*(Nr + 1)] */
	int Nr;
	const u8 *ct;
	u8 *pt;
{
	u32 s0, s1, s2, s3, t0, t1, t2, t3;
#ifndef FULL_UNROLL
    int r;
#endif /* ?FULL_UNROLL */

    /*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
    s0 = GETU32(ct     ) ^ rk[0];
    s1 = GETU32(ct +  4) ^ rk[1];
    s2 = GETU32(ct +  8) ^ rk[2];
    s3 = GETU32(ct + 12) ^ rk[3];
#ifdef FULL_UNROLL
    /* round 1: */
    t0 = Td0[s0 >> 24] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >>  8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[ 4];
    t1 = Td0[s1 >> 24] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >>  8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[ 5];
    t2 = Td0[s2 >> 24] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >>  8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[ 6];
    t3 = Td0[s3 >> 24] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >>  8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[ 7];
    /* round 2: */
    s0 = Td0[t0 >> 24] ^ Td1[(t3 >> 16) & 0xff] ^ Td2[(t2 >>  8) & 0xff] ^ Td3[t1 & 0xff] ^ rk[ 8];
    s1 = Td0[t1 >> 24] ^ Td1[(t0 >> 16) & 0xff] ^ Td2[(t3 >>  8) & 0xff] ^ Td3[t2 & 0xff] ^ rk[ 9];
    s2 = Td0[t2 >> 24] ^ Td1[(t1 >> 16) & 0xff] ^ Td2[(t0 >>  8) & 0xff] ^ Td3[t3 & 0xff] ^ rk[10];
    s3 = Td0[t3 >> 24] ^ Td1[(t2 >> 16) & 0xff] ^ Td2[(t1 >>  8) & 0xff] ^ Td3[t0 & 0xff] ^ rk[11];
    /* round 3: */
    t0 = Td0[s0 >> 24] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >>  8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[12];
    t1 = Td0[s1 >> 24] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >>  8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[13];
    t2 = Td0[s2 >> 24] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >>  8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[14];
    t3 = Td0[s3 >> 24] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >>  8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[15];
    /* round 4: */
    s0 = Td0[t0 >> 24] ^ Td1[(t3 >> 16) & 0xff] ^ Td2[(t2 >>  8) & 0xff] ^ Td3[t1 & 0xff] ^ rk[16];
    s1 = Td0[t1 >> 24] ^ Td1[(t0 >> 16) & 0xff] ^ Td2[(t3 >>  8) & 0xff] ^ Td3[t2 & 0xff] ^ rk[17];
    s2 = Td0[t2 >> 24] ^ Td1[(t1 >> 16) & 0xff] ^ Td2[(t0 >>  8) & 0xff] ^ Td3[t3 & 0xff] ^ rk[18];
    s3 = Td0[t3 >> 24] ^ Td1[(t2 >> 16) & 0xff] ^ Td2[(t1 >>  8) & 0xff] ^ Td3[t0 & 0xff] ^ rk[19];
    /* round 5: */
    t0 = Td0[s0 >> 24] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >>  8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[20];
    t1 = Td0[s1 >> 24] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >>  8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[21];
    t2 = Td0[s2 >> 24] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >>  8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[22];
    t3 = Td0[s3 >> 24] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >>  8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[23];
    /* round 6: */
    s0 = Td0[t0 >> 24] ^ Td1[(t3 >> 16) & 0xff] ^ Td2[(t2 >>  8) & 0xff] ^ Td3[t1 & 0xff] ^ rk[24];
    s1 = Td0[t1 >> 24] ^ Td1[(t0 >> 16) & 0xff] ^ Td2[(t3 >>  8) & 0xff] ^ Td3[t2 & 0xff] ^ rk[25];
    s2 = Td0[t2 >> 24] ^ Td1[(t1 >> 16) & 0xff] ^ Td2[(t0 >>  8) & 0xff] ^ Td3[t3 & 0xff] ^ rk[26];
    s3 = Td0[t3 >> 24] ^ Td1[(t2 >> 16) & 0xff] ^ Td2[(t1 >>  8) & 0xff] ^ Td3[t0 & 0xff] ^ rk[27];
    /* round 7: */
    t0 = Td0[s0 >> 24] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >>  8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[28];
    t1 = Td0[s1 >> 24] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >>  8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[29];
    t2 = Td0[s2 >> 24] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >>  8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[30];
    t3 = Td0[s3 >> 24] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >>  8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[31];
    /* round 8: */
    s0 = Td0[t0 >> 24] ^ Td1[(t3 >> 16) & 0xff] ^ Td2[(t2 >>  8) & 0xff] ^ Td3[t1 & 0xff] ^ rk[32];
    s1 = Td0[t1 >> 24] ^ Td1[(t0 >> 16) & 0xff] ^ Td2[(t3 >>  8) & 0xff] ^ Td3[t2 & 0xff] ^ rk[33];
    s2 = Td0[t2 >> 24] ^ Td1[(t1 >> 16) & 0xff] ^ Td2[(t0 >>  8) & 0xff] ^ Td3[t3 & 0xff] ^ rk[34];
    s3 = Td0[t3 >> 24] ^ Td1[(t2 >> 16) & 0xff] ^ Td2[(t1 >>  8) & 0xff] ^ Td3[t0 & 0xff] ^ rk[35];
    /* round 9: */
    t0 = Td0[s0 >> 24] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >>  8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[36];
    t1 = Td0[s1 >> 24] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >>  8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[37];
    t2 = Td0[s2 >> 24] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >>  8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[38];
    t3 = Td0[s3 >> 24] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >>  8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[39];
    if (Nr > 10) {
	/* round 10: */
	s0 = Td0[t0 >> 24] ^ Td1[(t3 >> 16) & 0xff] ^ Td2[(t2 >>  8) & 0xff] ^ Td3[t1 & 0xff] ^ rk[40];
	s1 = Td0[t1 >> 24] ^ Td1[(t0 >> 16) & 0xff] ^ Td2[(t3 >>  8) & 0xff] ^ Td3[t2 & 0xff] ^ rk[41];
	s2 = Td0[t2 >> 24] ^ Td1[(t1 >> 16) & 0xff] ^ Td2[(t0 >>  8) & 0xff] ^ Td3[t3 & 0xff] ^ rk[42];
	s3 = Td0[t3 >> 24] ^ Td1[(t2 >> 16) & 0xff] ^ Td2[(t1 >>  8) & 0xff] ^ Td3[t0 & 0xff] ^ rk[43];
	/* round 11: */
	t0 = Td0[s0 >> 24] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >>  8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[44];
	t1 = Td0[s1 >> 24] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >>  8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[45];
	t2 = Td0[s2 >> 24] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >>  8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[46];
	t3 = Td0[s3 >> 24] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >>  8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[47];
	if (Nr > 12) {
	    /* round 12: */
	    s0 = Td0[t0 >> 24] ^ Td1[(t3 >> 16) & 0xff] ^ Td2[(t2 >>  8) & 0xff] ^ Td3[t1 & 0xff] ^ rk[48];
	    s1 = Td0[t1 >> 24] ^ Td1[(t0 >> 16) & 0xff] ^ Td2[(t3 >>  8) & 0xff] ^ Td3[t2 & 0xff] ^ rk[49];
	    s2 = Td0[t2 >> 24] ^ Td1[(t1 >> 16) & 0xff] ^ Td2[(t0 >>  8) & 0xff] ^ Td3[t3 & 0xff] ^ rk[50];
	    s3 = Td0[t3 >> 24] ^ Td1[(t2 >> 16) & 0xff] ^ Td2[(t1 >>  8) & 0xff] ^ Td3[t0 & 0xff] ^ rk[51];
	    /* round 13: */
	    t0 = Td0[s0 >> 24] ^ Td1[(s3 >> 16) & 0xff] ^ Td2[(s2 >>  8) & 0xff] ^ Td3[s1 & 0xff] ^ rk[52];
	    t1 = Td0[s1 >> 24] ^ Td1[(s0 >> 16) & 0xff] ^ Td2[(s3 >>  8) & 0xff] ^ Td3[s2 & 0xff] ^ rk[53];
	    t2 = Td0[s2 >> 24] ^ Td1[(s1 >> 16) & 0xff] ^ Td2[(s0 >>  8) & 0xff] ^ Td3[s3 & 0xff] ^ rk[54];
	    t3 = Td0[s3 >> 24] ^ Td1[(s2 >> 16) & 0xff] ^ Td2[(s1 >>  8) & 0xff] ^ Td3[s0 & 0xff] ^ rk[55];
	}
    }
	rk += Nr << 2;
#else  /* !FULL_UNROLL */
    /*
     * Nr - 1 full rounds:
     */
    r = Nr >> 1;
    for (;;) {
	t0 =
	    Td0[(s0 >> 24)       ] ^
	    Td1[(s3 >> 16) & 0xff] ^
	    Td2[(s2 >>  8) & 0xff] ^
	    Td3[(s1      ) & 0xff] ^
	    rk[4];
	t1 =
	    Td0[(s1 >> 24)       ] ^
	    Td1[(s0 >> 16) & 0xff] ^
	    Td2[(s3 >>  8) & 0xff] ^
	    Td3[(s2      ) & 0xff] ^
	    rk[5];
	t2 =
	    Td0[(s2 >> 24)       ] ^
	    Td1[(s1 >> 16) & 0xff] ^
	    Td2[(s0 >>  8) & 0xff] ^
	    Td3[(s3      ) & 0xff] ^
	    rk[6];
	t3 =
	    Td0[(s3 >> 24)       ] ^
	    Td1[(s2 >> 16) & 0xff] ^
	    Td2[(s1 >>  8) & 0xff] ^
	    Td3[(s0      ) & 0xff] ^
	    rk[7];

	rk += 8;
	if (--r == 0) {
	    break;
	}

	s0 =
	    Td0[(t0 >> 24)       ] ^
	    Td1[(t3 >> 16) & 0xff] ^
	    Td2[(t2 >>  8) & 0xff] ^
	    Td3[(t1      ) & 0xff] ^
	    rk[0];
	s1 =
	    Td0[(t1 >> 24)       ] ^
	    Td1[(t0 >> 16) & 0xff] ^
	    Td2[(t3 >>  8) & 0xff] ^
	    Td3[(t2      ) & 0xff] ^
	    rk[1];
	s2 =
	    Td0[(t2 >> 24)       ] ^
	    Td1[(t1 >> 16) & 0xff] ^
	    Td2[(t0 >>  8) & 0xff] ^
	    Td3[(t3      ) & 0xff] ^
	    rk[2];
	s3 =
	    Td0[(t3 >> 24)       ] ^
	    Td1[(t2 >> 16) & 0xff] ^
	    Td2[(t1 >>  8) & 0xff] ^
	    Td3[(t0      ) & 0xff] ^
	    rk[3];
    }
#endif /* ?FULL_UNROLL */
    /*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	s0 =
		(Td4[(t0 >> 24)       ] & 0xff000000) ^
		(Td4[(t3 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(t2 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(t1      ) & 0xff] & 0x000000ff) ^
		rk[0];
	PUTU32(pt     , s0);
	s1 =
		(Td4[(t1 >> 24)       ] & 0xff000000) ^
		(Td4[(t0 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(t3 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(t2      ) & 0xff] & 0x000000ff) ^
		rk[1];
	PUTU32(pt +  4, s1);
	s2 =
		(Td4[(t2 >> 24)       ] & 0xff000000) ^
		(Td4[(t1 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(t0 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(t3      ) & 0xff] & 0x000000ff) ^
		rk[2];
	PUTU32(pt +  8, s2);
	s3 =
		(Td4[(t3 >> 24)       ] & 0xff000000) ^
		(Td4[(t2 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(t1 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(t0      ) & 0xff] & 0x000000ff) ^
		rk[3];
	PUTU32(pt + 12, s3);
}

#ifdef INTERMEDIATE_VALUE_KAT

/*
 * __db_rijndaelEncryptRound --
 *
 * PUBLIC: void __db_rijndaelEncryptRound __P((const u32 *, int, u8 *, int));
 */
void
__db_rijndaelEncryptRound(rk, Nr, pt, ct)
	const u32 *rk;	/* rk[4*(Nr + 1)] */
	int Nr;
	u8 *block;
	int rounds;
{
	int r;
	u32 s0, s1, s2, s3, t0, t1, t2, t3;

    /*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(block     ) ^ rk[0];
	s1 = GETU32(block +  4) ^ rk[1];
	s2 = GETU32(block +  8) ^ rk[2];
	s3 = GETU32(block + 12) ^ rk[3];
    rk += 4;

    /*
	 * Nr - 1 full rounds:
	 */
	for (r = (rounds < Nr ? rounds : Nr - 1); r > 0; r--) {
		t0 =
			Te0[(s0 >> 24)       ] ^
			Te1[(s1 >> 16) & 0xff] ^
			Te2[(s2 >>  8) & 0xff] ^
			Te3[(s3      ) & 0xff] ^
			rk[0];
		t1 =
			Te0[(s1 >> 24)       ] ^
			Te1[(s2 >> 16) & 0xff] ^
			Te2[(s3 >>  8) & 0xff] ^
			Te3[(s0      ) & 0xff] ^
			rk[1];
		t2 =
			Te0[(s2 >> 24)       ] ^
			Te1[(s3 >> 16) & 0xff] ^
			Te2[(s0 >>  8) & 0xff] ^
			Te3[(s1      ) & 0xff] ^
			rk[2];
		t3 =
			Te0[(s3 >> 24)       ] ^
			Te1[(s0 >> 16) & 0xff] ^
			Te2[(s1 >>  8) & 0xff] ^
			Te3[(s2      ) & 0xff] ^
			rk[3];

		s0 = t0;
		s1 = t1;
		s2 = t2;
		s3 = t3;
		rk += 4;

    }

    /*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	if (rounds == Nr) {
	t0 =
		(Te4[(s0 >> 24)       ] & 0xff000000) ^
		(Te4[(s1 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(s2 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(s3      ) & 0xff] & 0x000000ff) ^
		rk[0];
	t1 =
		(Te4[(s1 >> 24)       ] & 0xff000000) ^
		(Te4[(s2 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(s3 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(s0      ) & 0xff] & 0x000000ff) ^
		rk[1];
	t2 =
		(Te4[(s2 >> 24)       ] & 0xff000000) ^
		(Te4[(s3 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(s0 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(s1      ) & 0xff] & 0x000000ff) ^
		rk[2];
	t3 =
		(Te4[(s3 >> 24)       ] & 0xff000000) ^
		(Te4[(s0 >> 16) & 0xff] & 0x00ff0000) ^
		(Te4[(s1 >>  8) & 0xff] & 0x0000ff00) ^
		(Te4[(s2      ) & 0xff] & 0x000000ff) ^
		rk[3];

		s0 = t0;
		s1 = t1;
		s2 = t2;
		s3 = t3;
	}

	PUTU32(block     , s0);
	PUTU32(block +  4, s1);
	PUTU32(block +  8, s2);
	PUTU32(block + 12, s3);
}

/*
 * __db_rijndaelDecryptRound --
 *
 * PUBLIC: void __db_rijndaelDecryptRound __P((const u32 *, int, u8 *, int));
 */
void
__db_rijndaelDecryptRound(rk, Nr, pt, ct)
	const u32 *rk;	/* rk[4*(Nr + 1)] */
	int Nr;
	u8 *block;
	int rounds;
{
	int r;
	u32 s0, s1, s2, s3, t0, t1, t2, t3;

    /*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(block     ) ^ rk[0];
	s1 = GETU32(block +  4) ^ rk[1];
	s2 = GETU32(block +  8) ^ rk[2];
	s3 = GETU32(block + 12) ^ rk[3];
    rk += 4;

    /*
	 * Nr - 1 full rounds:
	 */
	for (r = (rounds < Nr ? rounds : Nr) - 1; r > 0; r--) {
		t0 =
			Td0[(s0 >> 24)       ] ^
			Td1[(s3 >> 16) & 0xff] ^
			Td2[(s2 >>  8) & 0xff] ^
			Td3[(s1      ) & 0xff] ^
			rk[0];
		t1 =
			Td0[(s1 >> 24)       ] ^
			Td1[(s0 >> 16) & 0xff] ^
			Td2[(s3 >>  8) & 0xff] ^
			Td3[(s2      ) & 0xff] ^
			rk[1];
		t2 =
			Td0[(s2 >> 24)       ] ^
			Td1[(s1 >> 16) & 0xff] ^
			Td2[(s0 >>  8) & 0xff] ^
			Td3[(s3      ) & 0xff] ^
			rk[2];
		t3 =
			Td0[(s3 >> 24)       ] ^
			Td1[(s2 >> 16) & 0xff] ^
			Td2[(s1 >>  8) & 0xff] ^
			Td3[(s0      ) & 0xff] ^
			rk[3];

		s0 = t0;
		s1 = t1;
		s2 = t2;
		s3 = t3;
		rk += 4;

    }

    /*
	 * complete the last round and
	 * map cipher state to byte array block:
	 */
	t0 =
		(Td4[(s0 >> 24)       ] & 0xff000000) ^
		(Td4[(s3 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(s2 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(s1      ) & 0xff] & 0x000000ff);
	t1 =
		(Td4[(s1 >> 24)       ] & 0xff000000) ^
		(Td4[(s0 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(s3 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(s2      ) & 0xff] & 0x000000ff);
	t2 =
		(Td4[(s2 >> 24)       ] & 0xff000000) ^
		(Td4[(s1 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(s0 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(s3      ) & 0xff] & 0x000000ff);
	t3 =
		(Td4[(s3 >> 24)       ] & 0xff000000) ^
		(Td4[(s2 >> 16) & 0xff] & 0x00ff0000) ^
		(Td4[(s1 >>  8) & 0xff] & 0x0000ff00) ^
		(Td4[(s0      ) & 0xff] & 0x000000ff);

	if (rounds == Nr) {
	    t0 ^= rk[0];
	    t1 ^= rk[1];
	    t2 ^= rk[2];
	    t3 ^= rk[3];
	}

	PUTU32(block     , t0);
	PUTU32(block +  4, t1);
	PUTU32(block +  8, t2);
	PUTU32(block + 12, t3);
}

#endif /* INTERMEDIATE_VALUE_KAT */
