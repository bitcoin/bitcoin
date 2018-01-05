#include <stddef.h>
#include <string.h>

#include "sph_fugue.h"

#ifdef __cplusplus
extern "C"{
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

static const sph_u32 IV224[] = {
	SPH_C32(0xf4c9120d), SPH_C32(0x6286f757), SPH_C32(0xee39e01c),
	SPH_C32(0xe074e3cb), SPH_C32(0xa1127c62), SPH_C32(0x9a43d215),
	SPH_C32(0xbd8d679a)
};

static const sph_u32 IV256[] = {
	SPH_C32(0xe952bdde), SPH_C32(0x6671135f), SPH_C32(0xe0d4f668),
	SPH_C32(0xd2b0b594), SPH_C32(0xf96c621d), SPH_C32(0xfbf929de),
	SPH_C32(0x9149e899), SPH_C32(0x34f8c248)
};

static const sph_u32 IV384[] = {
	SPH_C32(0xaa61ec0d), SPH_C32(0x31252e1f), SPH_C32(0xa01db4c7),
	SPH_C32(0x00600985), SPH_C32(0x215ef44a), SPH_C32(0x741b5e9c),
	SPH_C32(0xfa693e9a), SPH_C32(0x473eb040), SPH_C32(0xe502ae8a),
	SPH_C32(0xa99c25e0), SPH_C32(0xbc95517c), SPH_C32(0x5c1095a1)
};

static const sph_u32 IV512[] = {
	SPH_C32(0x8807a57e), SPH_C32(0xe616af75), SPH_C32(0xc5d3e4db),
	SPH_C32(0xac9ab027), SPH_C32(0xd915f117), SPH_C32(0xb6eecc54),
	SPH_C32(0x06e8020b), SPH_C32(0x4a92efd1), SPH_C32(0xaac6e2c9),
	SPH_C32(0xddb21398), SPH_C32(0xcae65838), SPH_C32(0x437f203f),
	SPH_C32(0x25ea78e7), SPH_C32(0x951fddd6), SPH_C32(0xda6ed11d),
	SPH_C32(0xe13e3567)
};

static const sph_u32 mixtab0[] = {
	SPH_C32(0x63633297), SPH_C32(0x7c7c6feb), SPH_C32(0x77775ec7),
	SPH_C32(0x7b7b7af7), SPH_C32(0xf2f2e8e5), SPH_C32(0x6b6b0ab7),
	SPH_C32(0x6f6f16a7), SPH_C32(0xc5c56d39), SPH_C32(0x303090c0),
	SPH_C32(0x01010704), SPH_C32(0x67672e87), SPH_C32(0x2b2bd1ac),
	SPH_C32(0xfefeccd5), SPH_C32(0xd7d71371), SPH_C32(0xabab7c9a),
	SPH_C32(0x767659c3), SPH_C32(0xcaca4005), SPH_C32(0x8282a33e),
	SPH_C32(0xc9c94909), SPH_C32(0x7d7d68ef), SPH_C32(0xfafad0c5),
	SPH_C32(0x5959947f), SPH_C32(0x4747ce07), SPH_C32(0xf0f0e6ed),
	SPH_C32(0xadad6e82), SPH_C32(0xd4d41a7d), SPH_C32(0xa2a243be),
	SPH_C32(0xafaf608a), SPH_C32(0x9c9cf946), SPH_C32(0xa4a451a6),
	SPH_C32(0x727245d3), SPH_C32(0xc0c0762d), SPH_C32(0xb7b728ea),
	SPH_C32(0xfdfdc5d9), SPH_C32(0x9393d47a), SPH_C32(0x2626f298),
	SPH_C32(0x363682d8), SPH_C32(0x3f3fbdfc), SPH_C32(0xf7f7f3f1),
	SPH_C32(0xcccc521d), SPH_C32(0x34348cd0), SPH_C32(0xa5a556a2),
	SPH_C32(0xe5e58db9), SPH_C32(0xf1f1e1e9), SPH_C32(0x71714cdf),
	SPH_C32(0xd8d83e4d), SPH_C32(0x313197c4), SPH_C32(0x15156b54),
	SPH_C32(0x04041c10), SPH_C32(0xc7c76331), SPH_C32(0x2323e98c),
	SPH_C32(0xc3c37f21), SPH_C32(0x18184860), SPH_C32(0x9696cf6e),
	SPH_C32(0x05051b14), SPH_C32(0x9a9aeb5e), SPH_C32(0x0707151c),
	SPH_C32(0x12127e48), SPH_C32(0x8080ad36), SPH_C32(0xe2e298a5),
	SPH_C32(0xebeba781), SPH_C32(0x2727f59c), SPH_C32(0xb2b233fe),
	SPH_C32(0x757550cf), SPH_C32(0x09093f24), SPH_C32(0x8383a43a),
	SPH_C32(0x2c2cc4b0), SPH_C32(0x1a1a4668), SPH_C32(0x1b1b416c),
	SPH_C32(0x6e6e11a3), SPH_C32(0x5a5a9d73), SPH_C32(0xa0a04db6),
	SPH_C32(0x5252a553), SPH_C32(0x3b3ba1ec), SPH_C32(0xd6d61475),
	SPH_C32(0xb3b334fa), SPH_C32(0x2929dfa4), SPH_C32(0xe3e39fa1),
	SPH_C32(0x2f2fcdbc), SPH_C32(0x8484b126), SPH_C32(0x5353a257),
	SPH_C32(0xd1d10169), SPH_C32(0x00000000), SPH_C32(0xededb599),
	SPH_C32(0x2020e080), SPH_C32(0xfcfcc2dd), SPH_C32(0xb1b13af2),
	SPH_C32(0x5b5b9a77), SPH_C32(0x6a6a0db3), SPH_C32(0xcbcb4701),
	SPH_C32(0xbebe17ce), SPH_C32(0x3939afe4), SPH_C32(0x4a4aed33),
	SPH_C32(0x4c4cff2b), SPH_C32(0x5858937b), SPH_C32(0xcfcf5b11),
	SPH_C32(0xd0d0066d), SPH_C32(0xefefbb91), SPH_C32(0xaaaa7b9e),
	SPH_C32(0xfbfbd7c1), SPH_C32(0x4343d217), SPH_C32(0x4d4df82f),
	SPH_C32(0x333399cc), SPH_C32(0x8585b622), SPH_C32(0x4545c00f),
	SPH_C32(0xf9f9d9c9), SPH_C32(0x02020e08), SPH_C32(0x7f7f66e7),
	SPH_C32(0x5050ab5b), SPH_C32(0x3c3cb4f0), SPH_C32(0x9f9ff04a),
	SPH_C32(0xa8a87596), SPH_C32(0x5151ac5f), SPH_C32(0xa3a344ba),
	SPH_C32(0x4040db1b), SPH_C32(0x8f8f800a), SPH_C32(0x9292d37e),
	SPH_C32(0x9d9dfe42), SPH_C32(0x3838a8e0), SPH_C32(0xf5f5fdf9),
	SPH_C32(0xbcbc19c6), SPH_C32(0xb6b62fee), SPH_C32(0xdada3045),
	SPH_C32(0x2121e784), SPH_C32(0x10107040), SPH_C32(0xffffcbd1),
	SPH_C32(0xf3f3efe1), SPH_C32(0xd2d20865), SPH_C32(0xcdcd5519),
	SPH_C32(0x0c0c2430), SPH_C32(0x1313794c), SPH_C32(0xececb29d),
	SPH_C32(0x5f5f8667), SPH_C32(0x9797c86a), SPH_C32(0x4444c70b),
	SPH_C32(0x1717655c), SPH_C32(0xc4c46a3d), SPH_C32(0xa7a758aa),
	SPH_C32(0x7e7e61e3), SPH_C32(0x3d3db3f4), SPH_C32(0x6464278b),
	SPH_C32(0x5d5d886f), SPH_C32(0x19194f64), SPH_C32(0x737342d7),
	SPH_C32(0x60603b9b), SPH_C32(0x8181aa32), SPH_C32(0x4f4ff627),
	SPH_C32(0xdcdc225d), SPH_C32(0x2222ee88), SPH_C32(0x2a2ad6a8),
	SPH_C32(0x9090dd76), SPH_C32(0x88889516), SPH_C32(0x4646c903),
	SPH_C32(0xeeeebc95), SPH_C32(0xb8b805d6), SPH_C32(0x14146c50),
	SPH_C32(0xdede2c55), SPH_C32(0x5e5e8163), SPH_C32(0x0b0b312c),
	SPH_C32(0xdbdb3741), SPH_C32(0xe0e096ad), SPH_C32(0x32329ec8),
	SPH_C32(0x3a3aa6e8), SPH_C32(0x0a0a3628), SPH_C32(0x4949e43f),
	SPH_C32(0x06061218), SPH_C32(0x2424fc90), SPH_C32(0x5c5c8f6b),
	SPH_C32(0xc2c27825), SPH_C32(0xd3d30f61), SPH_C32(0xacac6986),
	SPH_C32(0x62623593), SPH_C32(0x9191da72), SPH_C32(0x9595c662),
	SPH_C32(0xe4e48abd), SPH_C32(0x797974ff), SPH_C32(0xe7e783b1),
	SPH_C32(0xc8c84e0d), SPH_C32(0x373785dc), SPH_C32(0x6d6d18af),
	SPH_C32(0x8d8d8e02), SPH_C32(0xd5d51d79), SPH_C32(0x4e4ef123),
	SPH_C32(0xa9a97292), SPH_C32(0x6c6c1fab), SPH_C32(0x5656b943),
	SPH_C32(0xf4f4fafd), SPH_C32(0xeaeaa085), SPH_C32(0x6565208f),
	SPH_C32(0x7a7a7df3), SPH_C32(0xaeae678e), SPH_C32(0x08083820),
	SPH_C32(0xbaba0bde), SPH_C32(0x787873fb), SPH_C32(0x2525fb94),
	SPH_C32(0x2e2ecab8), SPH_C32(0x1c1c5470), SPH_C32(0xa6a65fae),
	SPH_C32(0xb4b421e6), SPH_C32(0xc6c66435), SPH_C32(0xe8e8ae8d),
	SPH_C32(0xdddd2559), SPH_C32(0x747457cb), SPH_C32(0x1f1f5d7c),
	SPH_C32(0x4b4bea37), SPH_C32(0xbdbd1ec2), SPH_C32(0x8b8b9c1a),
	SPH_C32(0x8a8a9b1e), SPH_C32(0x70704bdb), SPH_C32(0x3e3ebaf8),
	SPH_C32(0xb5b526e2), SPH_C32(0x66662983), SPH_C32(0x4848e33b),
	SPH_C32(0x0303090c), SPH_C32(0xf6f6f4f5), SPH_C32(0x0e0e2a38),
	SPH_C32(0x61613c9f), SPH_C32(0x35358bd4), SPH_C32(0x5757be47),
	SPH_C32(0xb9b902d2), SPH_C32(0x8686bf2e), SPH_C32(0xc1c17129),
	SPH_C32(0x1d1d5374), SPH_C32(0x9e9ef74e), SPH_C32(0xe1e191a9),
	SPH_C32(0xf8f8decd), SPH_C32(0x9898e556), SPH_C32(0x11117744),
	SPH_C32(0x696904bf), SPH_C32(0xd9d93949), SPH_C32(0x8e8e870e),
	SPH_C32(0x9494c166), SPH_C32(0x9b9bec5a), SPH_C32(0x1e1e5a78),
	SPH_C32(0x8787b82a), SPH_C32(0xe9e9a989), SPH_C32(0xcece5c15),
	SPH_C32(0x5555b04f), SPH_C32(0x2828d8a0), SPH_C32(0xdfdf2b51),
	SPH_C32(0x8c8c8906), SPH_C32(0xa1a14ab2), SPH_C32(0x89899212),
	SPH_C32(0x0d0d2334), SPH_C32(0xbfbf10ca), SPH_C32(0xe6e684b5),
	SPH_C32(0x4242d513), SPH_C32(0x686803bb), SPH_C32(0x4141dc1f),
	SPH_C32(0x9999e252), SPH_C32(0x2d2dc3b4), SPH_C32(0x0f0f2d3c),
	SPH_C32(0xb0b03df6), SPH_C32(0x5454b74b), SPH_C32(0xbbbb0cda),
	SPH_C32(0x16166258)
};

static const sph_u32 mixtab1[] = {
	SPH_C32(0x97636332), SPH_C32(0xeb7c7c6f), SPH_C32(0xc777775e),
	SPH_C32(0xf77b7b7a), SPH_C32(0xe5f2f2e8), SPH_C32(0xb76b6b0a),
	SPH_C32(0xa76f6f16), SPH_C32(0x39c5c56d), SPH_C32(0xc0303090),
	SPH_C32(0x04010107), SPH_C32(0x8767672e), SPH_C32(0xac2b2bd1),
	SPH_C32(0xd5fefecc), SPH_C32(0x71d7d713), SPH_C32(0x9aabab7c),
	SPH_C32(0xc3767659), SPH_C32(0x05caca40), SPH_C32(0x3e8282a3),
	SPH_C32(0x09c9c949), SPH_C32(0xef7d7d68), SPH_C32(0xc5fafad0),
	SPH_C32(0x7f595994), SPH_C32(0x074747ce), SPH_C32(0xedf0f0e6),
	SPH_C32(0x82adad6e), SPH_C32(0x7dd4d41a), SPH_C32(0xbea2a243),
	SPH_C32(0x8aafaf60), SPH_C32(0x469c9cf9), SPH_C32(0xa6a4a451),
	SPH_C32(0xd3727245), SPH_C32(0x2dc0c076), SPH_C32(0xeab7b728),
	SPH_C32(0xd9fdfdc5), SPH_C32(0x7a9393d4), SPH_C32(0x982626f2),
	SPH_C32(0xd8363682), SPH_C32(0xfc3f3fbd), SPH_C32(0xf1f7f7f3),
	SPH_C32(0x1dcccc52), SPH_C32(0xd034348c), SPH_C32(0xa2a5a556),
	SPH_C32(0xb9e5e58d), SPH_C32(0xe9f1f1e1), SPH_C32(0xdf71714c),
	SPH_C32(0x4dd8d83e), SPH_C32(0xc4313197), SPH_C32(0x5415156b),
	SPH_C32(0x1004041c), SPH_C32(0x31c7c763), SPH_C32(0x8c2323e9),
	SPH_C32(0x21c3c37f), SPH_C32(0x60181848), SPH_C32(0x6e9696cf),
	SPH_C32(0x1405051b), SPH_C32(0x5e9a9aeb), SPH_C32(0x1c070715),
	SPH_C32(0x4812127e), SPH_C32(0x368080ad), SPH_C32(0xa5e2e298),
	SPH_C32(0x81ebeba7), SPH_C32(0x9c2727f5), SPH_C32(0xfeb2b233),
	SPH_C32(0xcf757550), SPH_C32(0x2409093f), SPH_C32(0x3a8383a4),
	SPH_C32(0xb02c2cc4), SPH_C32(0x681a1a46), SPH_C32(0x6c1b1b41),
	SPH_C32(0xa36e6e11), SPH_C32(0x735a5a9d), SPH_C32(0xb6a0a04d),
	SPH_C32(0x535252a5), SPH_C32(0xec3b3ba1), SPH_C32(0x75d6d614),
	SPH_C32(0xfab3b334), SPH_C32(0xa42929df), SPH_C32(0xa1e3e39f),
	SPH_C32(0xbc2f2fcd), SPH_C32(0x268484b1), SPH_C32(0x575353a2),
	SPH_C32(0x69d1d101), SPH_C32(0x00000000), SPH_C32(0x99ededb5),
	SPH_C32(0x802020e0), SPH_C32(0xddfcfcc2), SPH_C32(0xf2b1b13a),
	SPH_C32(0x775b5b9a), SPH_C32(0xb36a6a0d), SPH_C32(0x01cbcb47),
	SPH_C32(0xcebebe17), SPH_C32(0xe43939af), SPH_C32(0x334a4aed),
	SPH_C32(0x2b4c4cff), SPH_C32(0x7b585893), SPH_C32(0x11cfcf5b),
	SPH_C32(0x6dd0d006), SPH_C32(0x91efefbb), SPH_C32(0x9eaaaa7b),
	SPH_C32(0xc1fbfbd7), SPH_C32(0x174343d2), SPH_C32(0x2f4d4df8),
	SPH_C32(0xcc333399), SPH_C32(0x228585b6), SPH_C32(0x0f4545c0),
	SPH_C32(0xc9f9f9d9), SPH_C32(0x0802020e), SPH_C32(0xe77f7f66),
	SPH_C32(0x5b5050ab), SPH_C32(0xf03c3cb4), SPH_C32(0x4a9f9ff0),
	SPH_C32(0x96a8a875), SPH_C32(0x5f5151ac), SPH_C32(0xbaa3a344),
	SPH_C32(0x1b4040db), SPH_C32(0x0a8f8f80), SPH_C32(0x7e9292d3),
	SPH_C32(0x429d9dfe), SPH_C32(0xe03838a8), SPH_C32(0xf9f5f5fd),
	SPH_C32(0xc6bcbc19), SPH_C32(0xeeb6b62f), SPH_C32(0x45dada30),
	SPH_C32(0x842121e7), SPH_C32(0x40101070), SPH_C32(0xd1ffffcb),
	SPH_C32(0xe1f3f3ef), SPH_C32(0x65d2d208), SPH_C32(0x19cdcd55),
	SPH_C32(0x300c0c24), SPH_C32(0x4c131379), SPH_C32(0x9dececb2),
	SPH_C32(0x675f5f86), SPH_C32(0x6a9797c8), SPH_C32(0x0b4444c7),
	SPH_C32(0x5c171765), SPH_C32(0x3dc4c46a), SPH_C32(0xaaa7a758),
	SPH_C32(0xe37e7e61), SPH_C32(0xf43d3db3), SPH_C32(0x8b646427),
	SPH_C32(0x6f5d5d88), SPH_C32(0x6419194f), SPH_C32(0xd7737342),
	SPH_C32(0x9b60603b), SPH_C32(0x328181aa), SPH_C32(0x274f4ff6),
	SPH_C32(0x5ddcdc22), SPH_C32(0x882222ee), SPH_C32(0xa82a2ad6),
	SPH_C32(0x769090dd), SPH_C32(0x16888895), SPH_C32(0x034646c9),
	SPH_C32(0x95eeeebc), SPH_C32(0xd6b8b805), SPH_C32(0x5014146c),
	SPH_C32(0x55dede2c), SPH_C32(0x635e5e81), SPH_C32(0x2c0b0b31),
	SPH_C32(0x41dbdb37), SPH_C32(0xade0e096), SPH_C32(0xc832329e),
	SPH_C32(0xe83a3aa6), SPH_C32(0x280a0a36), SPH_C32(0x3f4949e4),
	SPH_C32(0x18060612), SPH_C32(0x902424fc), SPH_C32(0x6b5c5c8f),
	SPH_C32(0x25c2c278), SPH_C32(0x61d3d30f), SPH_C32(0x86acac69),
	SPH_C32(0x93626235), SPH_C32(0x729191da), SPH_C32(0x629595c6),
	SPH_C32(0xbde4e48a), SPH_C32(0xff797974), SPH_C32(0xb1e7e783),
	SPH_C32(0x0dc8c84e), SPH_C32(0xdc373785), SPH_C32(0xaf6d6d18),
	SPH_C32(0x028d8d8e), SPH_C32(0x79d5d51d), SPH_C32(0x234e4ef1),
	SPH_C32(0x92a9a972), SPH_C32(0xab6c6c1f), SPH_C32(0x435656b9),
	SPH_C32(0xfdf4f4fa), SPH_C32(0x85eaeaa0), SPH_C32(0x8f656520),
	SPH_C32(0xf37a7a7d), SPH_C32(0x8eaeae67), SPH_C32(0x20080838),
	SPH_C32(0xdebaba0b), SPH_C32(0xfb787873), SPH_C32(0x942525fb),
	SPH_C32(0xb82e2eca), SPH_C32(0x701c1c54), SPH_C32(0xaea6a65f),
	SPH_C32(0xe6b4b421), SPH_C32(0x35c6c664), SPH_C32(0x8de8e8ae),
	SPH_C32(0x59dddd25), SPH_C32(0xcb747457), SPH_C32(0x7c1f1f5d),
	SPH_C32(0x374b4bea), SPH_C32(0xc2bdbd1e), SPH_C32(0x1a8b8b9c),
	SPH_C32(0x1e8a8a9b), SPH_C32(0xdb70704b), SPH_C32(0xf83e3eba),
	SPH_C32(0xe2b5b526), SPH_C32(0x83666629), SPH_C32(0x3b4848e3),
	SPH_C32(0x0c030309), SPH_C32(0xf5f6f6f4), SPH_C32(0x380e0e2a),
	SPH_C32(0x9f61613c), SPH_C32(0xd435358b), SPH_C32(0x475757be),
	SPH_C32(0xd2b9b902), SPH_C32(0x2e8686bf), SPH_C32(0x29c1c171),
	SPH_C32(0x741d1d53), SPH_C32(0x4e9e9ef7), SPH_C32(0xa9e1e191),
	SPH_C32(0xcdf8f8de), SPH_C32(0x569898e5), SPH_C32(0x44111177),
	SPH_C32(0xbf696904), SPH_C32(0x49d9d939), SPH_C32(0x0e8e8e87),
	SPH_C32(0x669494c1), SPH_C32(0x5a9b9bec), SPH_C32(0x781e1e5a),
	SPH_C32(0x2a8787b8), SPH_C32(0x89e9e9a9), SPH_C32(0x15cece5c),
	SPH_C32(0x4f5555b0), SPH_C32(0xa02828d8), SPH_C32(0x51dfdf2b),
	SPH_C32(0x068c8c89), SPH_C32(0xb2a1a14a), SPH_C32(0x12898992),
	SPH_C32(0x340d0d23), SPH_C32(0xcabfbf10), SPH_C32(0xb5e6e684),
	SPH_C32(0x134242d5), SPH_C32(0xbb686803), SPH_C32(0x1f4141dc),
	SPH_C32(0x529999e2), SPH_C32(0xb42d2dc3), SPH_C32(0x3c0f0f2d),
	SPH_C32(0xf6b0b03d), SPH_C32(0x4b5454b7), SPH_C32(0xdabbbb0c),
	SPH_C32(0x58161662)
};

static const sph_u32 mixtab2[] = {
	SPH_C32(0x32976363), SPH_C32(0x6feb7c7c), SPH_C32(0x5ec77777),
	SPH_C32(0x7af77b7b), SPH_C32(0xe8e5f2f2), SPH_C32(0x0ab76b6b),
	SPH_C32(0x16a76f6f), SPH_C32(0x6d39c5c5), SPH_C32(0x90c03030),
	SPH_C32(0x07040101), SPH_C32(0x2e876767), SPH_C32(0xd1ac2b2b),
	SPH_C32(0xccd5fefe), SPH_C32(0x1371d7d7), SPH_C32(0x7c9aabab),
	SPH_C32(0x59c37676), SPH_C32(0x4005caca), SPH_C32(0xa33e8282),
	SPH_C32(0x4909c9c9), SPH_C32(0x68ef7d7d), SPH_C32(0xd0c5fafa),
	SPH_C32(0x947f5959), SPH_C32(0xce074747), SPH_C32(0xe6edf0f0),
	SPH_C32(0x6e82adad), SPH_C32(0x1a7dd4d4), SPH_C32(0x43bea2a2),
	SPH_C32(0x608aafaf), SPH_C32(0xf9469c9c), SPH_C32(0x51a6a4a4),
	SPH_C32(0x45d37272), SPH_C32(0x762dc0c0), SPH_C32(0x28eab7b7),
	SPH_C32(0xc5d9fdfd), SPH_C32(0xd47a9393), SPH_C32(0xf2982626),
	SPH_C32(0x82d83636), SPH_C32(0xbdfc3f3f), SPH_C32(0xf3f1f7f7),
	SPH_C32(0x521dcccc), SPH_C32(0x8cd03434), SPH_C32(0x56a2a5a5),
	SPH_C32(0x8db9e5e5), SPH_C32(0xe1e9f1f1), SPH_C32(0x4cdf7171),
	SPH_C32(0x3e4dd8d8), SPH_C32(0x97c43131), SPH_C32(0x6b541515),
	SPH_C32(0x1c100404), SPH_C32(0x6331c7c7), SPH_C32(0xe98c2323),
	SPH_C32(0x7f21c3c3), SPH_C32(0x48601818), SPH_C32(0xcf6e9696),
	SPH_C32(0x1b140505), SPH_C32(0xeb5e9a9a), SPH_C32(0x151c0707),
	SPH_C32(0x7e481212), SPH_C32(0xad368080), SPH_C32(0x98a5e2e2),
	SPH_C32(0xa781ebeb), SPH_C32(0xf59c2727), SPH_C32(0x33feb2b2),
	SPH_C32(0x50cf7575), SPH_C32(0x3f240909), SPH_C32(0xa43a8383),
	SPH_C32(0xc4b02c2c), SPH_C32(0x46681a1a), SPH_C32(0x416c1b1b),
	SPH_C32(0x11a36e6e), SPH_C32(0x9d735a5a), SPH_C32(0x4db6a0a0),
	SPH_C32(0xa5535252), SPH_C32(0xa1ec3b3b), SPH_C32(0x1475d6d6),
	SPH_C32(0x34fab3b3), SPH_C32(0xdfa42929), SPH_C32(0x9fa1e3e3),
	SPH_C32(0xcdbc2f2f), SPH_C32(0xb1268484), SPH_C32(0xa2575353),
	SPH_C32(0x0169d1d1), SPH_C32(0x00000000), SPH_C32(0xb599eded),
	SPH_C32(0xe0802020), SPH_C32(0xc2ddfcfc), SPH_C32(0x3af2b1b1),
	SPH_C32(0x9a775b5b), SPH_C32(0x0db36a6a), SPH_C32(0x4701cbcb),
	SPH_C32(0x17cebebe), SPH_C32(0xafe43939), SPH_C32(0xed334a4a),
	SPH_C32(0xff2b4c4c), SPH_C32(0x937b5858), SPH_C32(0x5b11cfcf),
	SPH_C32(0x066dd0d0), SPH_C32(0xbb91efef), SPH_C32(0x7b9eaaaa),
	SPH_C32(0xd7c1fbfb), SPH_C32(0xd2174343), SPH_C32(0xf82f4d4d),
	SPH_C32(0x99cc3333), SPH_C32(0xb6228585), SPH_C32(0xc00f4545),
	SPH_C32(0xd9c9f9f9), SPH_C32(0x0e080202), SPH_C32(0x66e77f7f),
	SPH_C32(0xab5b5050), SPH_C32(0xb4f03c3c), SPH_C32(0xf04a9f9f),
	SPH_C32(0x7596a8a8), SPH_C32(0xac5f5151), SPH_C32(0x44baa3a3),
	SPH_C32(0xdb1b4040), SPH_C32(0x800a8f8f), SPH_C32(0xd37e9292),
	SPH_C32(0xfe429d9d), SPH_C32(0xa8e03838), SPH_C32(0xfdf9f5f5),
	SPH_C32(0x19c6bcbc), SPH_C32(0x2feeb6b6), SPH_C32(0x3045dada),
	SPH_C32(0xe7842121), SPH_C32(0x70401010), SPH_C32(0xcbd1ffff),
	SPH_C32(0xefe1f3f3), SPH_C32(0x0865d2d2), SPH_C32(0x5519cdcd),
	SPH_C32(0x24300c0c), SPH_C32(0x794c1313), SPH_C32(0xb29decec),
	SPH_C32(0x86675f5f), SPH_C32(0xc86a9797), SPH_C32(0xc70b4444),
	SPH_C32(0x655c1717), SPH_C32(0x6a3dc4c4), SPH_C32(0x58aaa7a7),
	SPH_C32(0x61e37e7e), SPH_C32(0xb3f43d3d), SPH_C32(0x278b6464),
	SPH_C32(0x886f5d5d), SPH_C32(0x4f641919), SPH_C32(0x42d77373),
	SPH_C32(0x3b9b6060), SPH_C32(0xaa328181), SPH_C32(0xf6274f4f),
	SPH_C32(0x225ddcdc), SPH_C32(0xee882222), SPH_C32(0xd6a82a2a),
	SPH_C32(0xdd769090), SPH_C32(0x95168888), SPH_C32(0xc9034646),
	SPH_C32(0xbc95eeee), SPH_C32(0x05d6b8b8), SPH_C32(0x6c501414),
	SPH_C32(0x2c55dede), SPH_C32(0x81635e5e), SPH_C32(0x312c0b0b),
	SPH_C32(0x3741dbdb), SPH_C32(0x96ade0e0), SPH_C32(0x9ec83232),
	SPH_C32(0xa6e83a3a), SPH_C32(0x36280a0a), SPH_C32(0xe43f4949),
	SPH_C32(0x12180606), SPH_C32(0xfc902424), SPH_C32(0x8f6b5c5c),
	SPH_C32(0x7825c2c2), SPH_C32(0x0f61d3d3), SPH_C32(0x6986acac),
	SPH_C32(0x35936262), SPH_C32(0xda729191), SPH_C32(0xc6629595),
	SPH_C32(0x8abde4e4), SPH_C32(0x74ff7979), SPH_C32(0x83b1e7e7),
	SPH_C32(0x4e0dc8c8), SPH_C32(0x85dc3737), SPH_C32(0x18af6d6d),
	SPH_C32(0x8e028d8d), SPH_C32(0x1d79d5d5), SPH_C32(0xf1234e4e),
	SPH_C32(0x7292a9a9), SPH_C32(0x1fab6c6c), SPH_C32(0xb9435656),
	SPH_C32(0xfafdf4f4), SPH_C32(0xa085eaea), SPH_C32(0x208f6565),
	SPH_C32(0x7df37a7a), SPH_C32(0x678eaeae), SPH_C32(0x38200808),
	SPH_C32(0x0bdebaba), SPH_C32(0x73fb7878), SPH_C32(0xfb942525),
	SPH_C32(0xcab82e2e), SPH_C32(0x54701c1c), SPH_C32(0x5faea6a6),
	SPH_C32(0x21e6b4b4), SPH_C32(0x6435c6c6), SPH_C32(0xae8de8e8),
	SPH_C32(0x2559dddd), SPH_C32(0x57cb7474), SPH_C32(0x5d7c1f1f),
	SPH_C32(0xea374b4b), SPH_C32(0x1ec2bdbd), SPH_C32(0x9c1a8b8b),
	SPH_C32(0x9b1e8a8a), SPH_C32(0x4bdb7070), SPH_C32(0xbaf83e3e),
	SPH_C32(0x26e2b5b5), SPH_C32(0x29836666), SPH_C32(0xe33b4848),
	SPH_C32(0x090c0303), SPH_C32(0xf4f5f6f6), SPH_C32(0x2a380e0e),
	SPH_C32(0x3c9f6161), SPH_C32(0x8bd43535), SPH_C32(0xbe475757),
	SPH_C32(0x02d2b9b9), SPH_C32(0xbf2e8686), SPH_C32(0x7129c1c1),
	SPH_C32(0x53741d1d), SPH_C32(0xf74e9e9e), SPH_C32(0x91a9e1e1),
	SPH_C32(0xdecdf8f8), SPH_C32(0xe5569898), SPH_C32(0x77441111),
	SPH_C32(0x04bf6969), SPH_C32(0x3949d9d9), SPH_C32(0x870e8e8e),
	SPH_C32(0xc1669494), SPH_C32(0xec5a9b9b), SPH_C32(0x5a781e1e),
	SPH_C32(0xb82a8787), SPH_C32(0xa989e9e9), SPH_C32(0x5c15cece),
	SPH_C32(0xb04f5555), SPH_C32(0xd8a02828), SPH_C32(0x2b51dfdf),
	SPH_C32(0x89068c8c), SPH_C32(0x4ab2a1a1), SPH_C32(0x92128989),
	SPH_C32(0x23340d0d), SPH_C32(0x10cabfbf), SPH_C32(0x84b5e6e6),
	SPH_C32(0xd5134242), SPH_C32(0x03bb6868), SPH_C32(0xdc1f4141),
	SPH_C32(0xe2529999), SPH_C32(0xc3b42d2d), SPH_C32(0x2d3c0f0f),
	SPH_C32(0x3df6b0b0), SPH_C32(0xb74b5454), SPH_C32(0x0cdabbbb),
	SPH_C32(0x62581616)
};

static const sph_u32 mixtab3[] = {
	SPH_C32(0x63329763), SPH_C32(0x7c6feb7c), SPH_C32(0x775ec777),
	SPH_C32(0x7b7af77b), SPH_C32(0xf2e8e5f2), SPH_C32(0x6b0ab76b),
	SPH_C32(0x6f16a76f), SPH_C32(0xc56d39c5), SPH_C32(0x3090c030),
	SPH_C32(0x01070401), SPH_C32(0x672e8767), SPH_C32(0x2bd1ac2b),
	SPH_C32(0xfeccd5fe), SPH_C32(0xd71371d7), SPH_C32(0xab7c9aab),
	SPH_C32(0x7659c376), SPH_C32(0xca4005ca), SPH_C32(0x82a33e82),
	SPH_C32(0xc94909c9), SPH_C32(0x7d68ef7d), SPH_C32(0xfad0c5fa),
	SPH_C32(0x59947f59), SPH_C32(0x47ce0747), SPH_C32(0xf0e6edf0),
	SPH_C32(0xad6e82ad), SPH_C32(0xd41a7dd4), SPH_C32(0xa243bea2),
	SPH_C32(0xaf608aaf), SPH_C32(0x9cf9469c), SPH_C32(0xa451a6a4),
	SPH_C32(0x7245d372), SPH_C32(0xc0762dc0), SPH_C32(0xb728eab7),
	SPH_C32(0xfdc5d9fd), SPH_C32(0x93d47a93), SPH_C32(0x26f29826),
	SPH_C32(0x3682d836), SPH_C32(0x3fbdfc3f), SPH_C32(0xf7f3f1f7),
	SPH_C32(0xcc521dcc), SPH_C32(0x348cd034), SPH_C32(0xa556a2a5),
	SPH_C32(0xe58db9e5), SPH_C32(0xf1e1e9f1), SPH_C32(0x714cdf71),
	SPH_C32(0xd83e4dd8), SPH_C32(0x3197c431), SPH_C32(0x156b5415),
	SPH_C32(0x041c1004), SPH_C32(0xc76331c7), SPH_C32(0x23e98c23),
	SPH_C32(0xc37f21c3), SPH_C32(0x18486018), SPH_C32(0x96cf6e96),
	SPH_C32(0x051b1405), SPH_C32(0x9aeb5e9a), SPH_C32(0x07151c07),
	SPH_C32(0x127e4812), SPH_C32(0x80ad3680), SPH_C32(0xe298a5e2),
	SPH_C32(0xeba781eb), SPH_C32(0x27f59c27), SPH_C32(0xb233feb2),
	SPH_C32(0x7550cf75), SPH_C32(0x093f2409), SPH_C32(0x83a43a83),
	SPH_C32(0x2cc4b02c), SPH_C32(0x1a46681a), SPH_C32(0x1b416c1b),
	SPH_C32(0x6e11a36e), SPH_C32(0x5a9d735a), SPH_C32(0xa04db6a0),
	SPH_C32(0x52a55352), SPH_C32(0x3ba1ec3b), SPH_C32(0xd61475d6),
	SPH_C32(0xb334fab3), SPH_C32(0x29dfa429), SPH_C32(0xe39fa1e3),
	SPH_C32(0x2fcdbc2f), SPH_C32(0x84b12684), SPH_C32(0x53a25753),
	SPH_C32(0xd10169d1), SPH_C32(0x00000000), SPH_C32(0xedb599ed),
	SPH_C32(0x20e08020), SPH_C32(0xfcc2ddfc), SPH_C32(0xb13af2b1),
	SPH_C32(0x5b9a775b), SPH_C32(0x6a0db36a), SPH_C32(0xcb4701cb),
	SPH_C32(0xbe17cebe), SPH_C32(0x39afe439), SPH_C32(0x4aed334a),
	SPH_C32(0x4cff2b4c), SPH_C32(0x58937b58), SPH_C32(0xcf5b11cf),
	SPH_C32(0xd0066dd0), SPH_C32(0xefbb91ef), SPH_C32(0xaa7b9eaa),
	SPH_C32(0xfbd7c1fb), SPH_C32(0x43d21743), SPH_C32(0x4df82f4d),
	SPH_C32(0x3399cc33), SPH_C32(0x85b62285), SPH_C32(0x45c00f45),
	SPH_C32(0xf9d9c9f9), SPH_C32(0x020e0802), SPH_C32(0x7f66e77f),
	SPH_C32(0x50ab5b50), SPH_C32(0x3cb4f03c), SPH_C32(0x9ff04a9f),
	SPH_C32(0xa87596a8), SPH_C32(0x51ac5f51), SPH_C32(0xa344baa3),
	SPH_C32(0x40db1b40), SPH_C32(0x8f800a8f), SPH_C32(0x92d37e92),
	SPH_C32(0x9dfe429d), SPH_C32(0x38a8e038), SPH_C32(0xf5fdf9f5),
	SPH_C32(0xbc19c6bc), SPH_C32(0xb62feeb6), SPH_C32(0xda3045da),
	SPH_C32(0x21e78421), SPH_C32(0x10704010), SPH_C32(0xffcbd1ff),
	SPH_C32(0xf3efe1f3), SPH_C32(0xd20865d2), SPH_C32(0xcd5519cd),
	SPH_C32(0x0c24300c), SPH_C32(0x13794c13), SPH_C32(0xecb29dec),
	SPH_C32(0x5f86675f), SPH_C32(0x97c86a97), SPH_C32(0x44c70b44),
	SPH_C32(0x17655c17), SPH_C32(0xc46a3dc4), SPH_C32(0xa758aaa7),
	SPH_C32(0x7e61e37e), SPH_C32(0x3db3f43d), SPH_C32(0x64278b64),
	SPH_C32(0x5d886f5d), SPH_C32(0x194f6419), SPH_C32(0x7342d773),
	SPH_C32(0x603b9b60), SPH_C32(0x81aa3281), SPH_C32(0x4ff6274f),
	SPH_C32(0xdc225ddc), SPH_C32(0x22ee8822), SPH_C32(0x2ad6a82a),
	SPH_C32(0x90dd7690), SPH_C32(0x88951688), SPH_C32(0x46c90346),
	SPH_C32(0xeebc95ee), SPH_C32(0xb805d6b8), SPH_C32(0x146c5014),
	SPH_C32(0xde2c55de), SPH_C32(0x5e81635e), SPH_C32(0x0b312c0b),
	SPH_C32(0xdb3741db), SPH_C32(0xe096ade0), SPH_C32(0x329ec832),
	SPH_C32(0x3aa6e83a), SPH_C32(0x0a36280a), SPH_C32(0x49e43f49),
	SPH_C32(0x06121806), SPH_C32(0x24fc9024), SPH_C32(0x5c8f6b5c),
	SPH_C32(0xc27825c2), SPH_C32(0xd30f61d3), SPH_C32(0xac6986ac),
	SPH_C32(0x62359362), SPH_C32(0x91da7291), SPH_C32(0x95c66295),
	SPH_C32(0xe48abde4), SPH_C32(0x7974ff79), SPH_C32(0xe783b1e7),
	SPH_C32(0xc84e0dc8), SPH_C32(0x3785dc37), SPH_C32(0x6d18af6d),
	SPH_C32(0x8d8e028d), SPH_C32(0xd51d79d5), SPH_C32(0x4ef1234e),
	SPH_C32(0xa97292a9), SPH_C32(0x6c1fab6c), SPH_C32(0x56b94356),
	SPH_C32(0xf4fafdf4), SPH_C32(0xeaa085ea), SPH_C32(0x65208f65),
	SPH_C32(0x7a7df37a), SPH_C32(0xae678eae), SPH_C32(0x08382008),
	SPH_C32(0xba0bdeba), SPH_C32(0x7873fb78), SPH_C32(0x25fb9425),
	SPH_C32(0x2ecab82e), SPH_C32(0x1c54701c), SPH_C32(0xa65faea6),
	SPH_C32(0xb421e6b4), SPH_C32(0xc66435c6), SPH_C32(0xe8ae8de8),
	SPH_C32(0xdd2559dd), SPH_C32(0x7457cb74), SPH_C32(0x1f5d7c1f),
	SPH_C32(0x4bea374b), SPH_C32(0xbd1ec2bd), SPH_C32(0x8b9c1a8b),
	SPH_C32(0x8a9b1e8a), SPH_C32(0x704bdb70), SPH_C32(0x3ebaf83e),
	SPH_C32(0xb526e2b5), SPH_C32(0x66298366), SPH_C32(0x48e33b48),
	SPH_C32(0x03090c03), SPH_C32(0xf6f4f5f6), SPH_C32(0x0e2a380e),
	SPH_C32(0x613c9f61), SPH_C32(0x358bd435), SPH_C32(0x57be4757),
	SPH_C32(0xb902d2b9), SPH_C32(0x86bf2e86), SPH_C32(0xc17129c1),
	SPH_C32(0x1d53741d), SPH_C32(0x9ef74e9e), SPH_C32(0xe191a9e1),
	SPH_C32(0xf8decdf8), SPH_C32(0x98e55698), SPH_C32(0x11774411),
	SPH_C32(0x6904bf69), SPH_C32(0xd93949d9), SPH_C32(0x8e870e8e),
	SPH_C32(0x94c16694), SPH_C32(0x9bec5a9b), SPH_C32(0x1e5a781e),
	SPH_C32(0x87b82a87), SPH_C32(0xe9a989e9), SPH_C32(0xce5c15ce),
	SPH_C32(0x55b04f55), SPH_C32(0x28d8a028), SPH_C32(0xdf2b51df),
	SPH_C32(0x8c89068c), SPH_C32(0xa14ab2a1), SPH_C32(0x89921289),
	SPH_C32(0x0d23340d), SPH_C32(0xbf10cabf), SPH_C32(0xe684b5e6),
	SPH_C32(0x42d51342), SPH_C32(0x6803bb68), SPH_C32(0x41dc1f41),
	SPH_C32(0x99e25299), SPH_C32(0x2dc3b42d), SPH_C32(0x0f2d3c0f),
	SPH_C32(0xb03df6b0), SPH_C32(0x54b74b54), SPH_C32(0xbb0cdabb),
	SPH_C32(0x16625816)
};

#define TIX2(q, x00, x01, x08, x10, x24)   do { \
		x10 ^= x00; \
		x00 = (q); \
		x08 ^= x00; \
		x01 ^= x24; \
	} while (0)

#define TIX3(q, x00, x01, x04, x08, x16, x27, x30)   do { \
		x16 ^= x00; \
		x00 = (q); \
		x08 ^= x00; \
		x01 ^= x27; \
		x04 ^= x30; \
	} while (0)

#define TIX4(q, x00, x01, x04, x07, x08, x22, x24, x27, x30)   do { \
		x22 ^= x00; \
		x00 = (q); \
		x08 ^= x00; \
		x01 ^= x24; \
		x04 ^= x27; \
		x07 ^= x30; \
	} while (0)

#define CMIX30(x00, x01, x02, x04, x05, x06, x15, x16, x17)   do { \
		x00 ^= x04; \
		x01 ^= x05; \
		x02 ^= x06; \
		x15 ^= x04; \
		x16 ^= x05; \
		x17 ^= x06; \
	} while (0)

#define CMIX36(x00, x01, x02, x04, x05, x06, x18, x19, x20)   do { \
		x00 ^= x04; \
		x01 ^= x05; \
		x02 ^= x06; \
		x18 ^= x04; \
		x19 ^= x05; \
		x20 ^= x06; \
	} while (0)

#define SMIX(x0, x1, x2, x3)   do { \
		sph_u32 c0 = 0; \
		sph_u32 c1 = 0; \
		sph_u32 c2 = 0; \
		sph_u32 c3 = 0; \
		sph_u32 r0 = 0; \
		sph_u32 r1 = 0; \
		sph_u32 r2 = 0; \
		sph_u32 r3 = 0; \
		sph_u32 tmp; \
		tmp = mixtab0[x0 >> 24]; \
		c0 ^= tmp; \
		tmp = mixtab1[(x0 >> 16) & 0xFF]; \
		c0 ^= tmp; \
		r1 ^= tmp; \
		tmp = mixtab2[(x0 >>  8) & 0xFF]; \
		c0 ^= tmp; \
		r2 ^= tmp; \
		tmp = mixtab3[x0 & 0xFF]; \
		c0 ^= tmp; \
		r3 ^= tmp; \
		tmp = mixtab0[x1 >> 24]; \
		c1 ^= tmp; \
		r0 ^= tmp; \
		tmp = mixtab1[(x1 >> 16) & 0xFF]; \
		c1 ^= tmp; \
		tmp = mixtab2[(x1 >>  8) & 0xFF]; \
		c1 ^= tmp; \
		r2 ^= tmp; \
		tmp = mixtab3[x1 & 0xFF]; \
		c1 ^= tmp; \
		r3 ^= tmp; \
		tmp = mixtab0[x2 >> 24]; \
		c2 ^= tmp; \
		r0 ^= tmp; \
		tmp = mixtab1[(x2 >> 16) & 0xFF]; \
		c2 ^= tmp; \
		r1 ^= tmp; \
		tmp = mixtab2[(x2 >>  8) & 0xFF]; \
		c2 ^= tmp; \
		tmp = mixtab3[x2 & 0xFF]; \
		c2 ^= tmp; \
		r3 ^= tmp; \
		tmp = mixtab0[x3 >> 24]; \
		c3 ^= tmp; \
		r0 ^= tmp; \
		tmp = mixtab1[(x3 >> 16) & 0xFF]; \
		c3 ^= tmp; \
		r1 ^= tmp; \
		tmp = mixtab2[(x3 >>  8) & 0xFF]; \
		c3 ^= tmp; \
		r2 ^= tmp; \
		tmp = mixtab3[x3 & 0xFF]; \
		c3 ^= tmp; \
		x0 = ((c0 ^ r0) & SPH_C32(0xFF000000)) \
			| ((c1 ^ r1) & SPH_C32(0x00FF0000)) \
			| ((c2 ^ r2) & SPH_C32(0x0000FF00)) \
			| ((c3 ^ r3) & SPH_C32(0x000000FF)); \
		x1 = ((c1 ^ (r0 << 8)) & SPH_C32(0xFF000000)) \
			| ((c2 ^ (r1 << 8)) & SPH_C32(0x00FF0000)) \
			| ((c3 ^ (r2 << 8)) & SPH_C32(0x0000FF00)) \
			| ((c0 ^ (r3 >> 24)) & SPH_C32(0x000000FF)); \
		x2 = ((c2 ^ (r0 << 16)) & SPH_C32(0xFF000000)) \
			| ((c3 ^ (r1 << 16)) & SPH_C32(0x00FF0000)) \
			| ((c0 ^ (r2 >> 16)) & SPH_C32(0x0000FF00)) \
			| ((c1 ^ (r3 >> 16)) & SPH_C32(0x000000FF)); \
		x3 = ((c3 ^ (r0 << 24)) & SPH_C32(0xFF000000)) \
			| ((c0 ^ (r1 >> 8)) & SPH_C32(0x00FF0000)) \
			| ((c1 ^ (r2 >> 8)) & SPH_C32(0x0000FF00)) \
			| ((c2 ^ (r3 >> 8)) & SPH_C32(0x000000FF)); \
		/* */ \
	} while (0)

#if SPH_FUGUE_NOCOPY

#define DECL_STATE_SMALL
#define READ_STATE_SMALL(state)
#define WRITE_STATE_SMALL(state)
#define DECL_STATE_BIG
#define READ_STATE_BIG(state)
#define WRITE_STATE_BIG(state)

#define S00   ((sc)->S[ 0])
#define S01   ((sc)->S[ 1])
#define S02   ((sc)->S[ 2])
#define S03   ((sc)->S[ 3])
#define S04   ((sc)->S[ 4])
#define S05   ((sc)->S[ 5])
#define S06   ((sc)->S[ 6])
#define S07   ((sc)->S[ 7])
#define S08   ((sc)->S[ 8])
#define S09   ((sc)->S[ 9])
#define S10   ((sc)->S[10])
#define S11   ((sc)->S[11])
#define S12   ((sc)->S[12])
#define S13   ((sc)->S[13])
#define S14   ((sc)->S[14])
#define S15   ((sc)->S[15])
#define S16   ((sc)->S[16])
#define S17   ((sc)->S[17])
#define S18   ((sc)->S[18])
#define S19   ((sc)->S[19])
#define S20   ((sc)->S[20])
#define S21   ((sc)->S[21])
#define S22   ((sc)->S[22])
#define S23   ((sc)->S[23])
#define S24   ((sc)->S[24])
#define S25   ((sc)->S[25])
#define S26   ((sc)->S[26])
#define S27   ((sc)->S[27])
#define S28   ((sc)->S[28])
#define S29   ((sc)->S[29])
#define S30   ((sc)->S[30])
#define S31   ((sc)->S[31])
#define S32   ((sc)->S[32])
#define S33   ((sc)->S[33])
#define S34   ((sc)->S[34])
#define S35   ((sc)->S[35])

#else

#define DECL_STATE_SMALL \
	sph_u32 S00, S01, S02, S03, S04, S05, S06, S07, S08, S09; \
	sph_u32 S10, S11, S12, S13, S14, S15, S16, S17, S18, S19; \
	sph_u32 S20, S21, S22, S23, S24, S25, S26, S27, S28, S29;

#define DECL_STATE_BIG \
	DECL_STATE_SMALL \
	sph_u32 S30, S31, S32, S33, S34, S35;

#define READ_STATE_SMALL(state)   do { \
		S00 = (state)->S[ 0]; \
		S01 = (state)->S[ 1]; \
		S02 = (state)->S[ 2]; \
		S03 = (state)->S[ 3]; \
		S04 = (state)->S[ 4]; \
		S05 = (state)->S[ 5]; \
		S06 = (state)->S[ 6]; \
		S07 = (state)->S[ 7]; \
		S08 = (state)->S[ 8]; \
		S09 = (state)->S[ 9]; \
		S10 = (state)->S[10]; \
		S11 = (state)->S[11]; \
		S12 = (state)->S[12]; \
		S13 = (state)->S[13]; \
		S14 = (state)->S[14]; \
		S15 = (state)->S[15]; \
		S16 = (state)->S[16]; \
		S17 = (state)->S[17]; \
		S18 = (state)->S[18]; \
		S19 = (state)->S[19]; \
		S20 = (state)->S[20]; \
		S21 = (state)->S[21]; \
		S22 = (state)->S[22]; \
		S23 = (state)->S[23]; \
		S24 = (state)->S[24]; \
		S25 = (state)->S[25]; \
		S26 = (state)->S[26]; \
		S27 = (state)->S[27]; \
		S28 = (state)->S[28]; \
		S29 = (state)->S[29]; \
	} while (0)

#define READ_STATE_BIG(state)   do { \
		READ_STATE_SMALL(state); \
		S30 = (state)->S[30]; \
		S31 = (state)->S[31]; \
		S32 = (state)->S[32]; \
		S33 = (state)->S[33]; \
		S34 = (state)->S[34]; \
		S35 = (state)->S[35]; \
	} while (0)

#define WRITE_STATE_SMALL(state)   do { \
		(state)->S[ 0] = S00; \
		(state)->S[ 1] = S01; \
		(state)->S[ 2] = S02; \
		(state)->S[ 3] = S03; \
		(state)->S[ 4] = S04; \
		(state)->S[ 5] = S05; \
		(state)->S[ 6] = S06; \
		(state)->S[ 7] = S07; \
		(state)->S[ 8] = S08; \
		(state)->S[ 9] = S09; \
		(state)->S[10] = S10; \
		(state)->S[11] = S11; \
		(state)->S[12] = S12; \
		(state)->S[13] = S13; \
		(state)->S[14] = S14; \
		(state)->S[15] = S15; \
		(state)->S[16] = S16; \
		(state)->S[17] = S17; \
		(state)->S[18] = S18; \
		(state)->S[19] = S19; \
		(state)->S[20] = S20; \
		(state)->S[21] = S21; \
		(state)->S[22] = S22; \
		(state)->S[23] = S23; \
		(state)->S[24] = S24; \
		(state)->S[25] = S25; \
		(state)->S[26] = S26; \
		(state)->S[27] = S27; \
		(state)->S[28] = S28; \
		(state)->S[29] = S29; \
	} while (0)

#define WRITE_STATE_BIG(state)   do { \
		WRITE_STATE_SMALL(state); \
		(state)->S[30] = S30; \
		(state)->S[31] = S31; \
		(state)->S[32] = S32; \
		(state)->S[33] = S33; \
		(state)->S[34] = S34; \
		(state)->S[35] = S35; \
	} while (0)

#endif

static void
fugue_init(sph_fugue_context *sc, size_t z_len,
	const sph_u32 *iv, size_t iv_len)
{
	size_t u;

	for (u = 0; u < z_len; u ++)
		sc->S[u] = 0;
	memcpy(&sc->S[z_len], iv, iv_len * sizeof *iv);
	sc->partial = 0;
	sc->partial_len = 0;
	sc->round_shift = 0;
#if SPH_64
	sc->bit_count = 0;
#else
	sc->bit_count_high = 0;
	sc->bit_count_low = 0;
#endif
}

#if SPH_64

#define INCR_COUNTER   do { \
		sc->bit_count += (sph_u64)len << 3; \
	} while (0)

#else

#define INCR_COUNTER   do { \
		sph_u32 tmp = SPH_T32((sph_u32)len << 3); \
		sc->bit_count_low = SPH_T32(sc->bit_count_low + tmp); \
		if (sc->bit_count_low < tmp) \
			sc->bit_count_high ++; \
		sc->bit_count_high = SPH_T32(sc->bit_count_high \
			+ ((sph_u32)len >> 29)); \
	} while (0)

#endif

#define CORE_ENTRY \
	sph_u32 p; \
	unsigned plen, rshift; \
	INCR_COUNTER; \
	p = sc->partial; \
	plen = sc->partial_len; \
	if (plen < 4) { \
		unsigned count = 4 - plen; \
		if (len < count) \
			count = len; \
		plen += count; \
		while (count -- > 0) { \
			p = (p << 8) | *(const unsigned char *)data; \
			data = (const unsigned char *)data + 1; \
			len --; \
		} \
		if (len == 0) { \
			sc->partial = p; \
			sc->partial_len = plen; \
			return; \
		} \
	}

#define CORE_EXIT \
	p = 0; \
	sc->partial_len = (unsigned)len; \
	while (len -- > 0) { \
		p = (p << 8) | *(const unsigned char *)data; \
		data = (const unsigned char *)data + 1; \
	} \
	sc->partial = p; \
	sc->round_shift = rshift;

/*
 * Not in a do..while: the 'break' must exit the outer loop.
 */
#define NEXT(rc) \
	if (len <= 4) { \
		rshift = (rc); \
		break; \
	} \
	p = sph_dec32be(data); \
	data = (const unsigned char *)data + 4; \
	len -= 4

static void
fugue2_core(sph_fugue_context *sc, const void *data, size_t len)
{
	DECL_STATE_SMALL
	CORE_ENTRY
	READ_STATE_SMALL(sc);
	rshift = sc->round_shift;
	switch (rshift) {
		for (;;) {
			sph_u32 q;

		case 0:
			q = p;
			TIX2(q, S00, S01, S08, S10, S24);
			CMIX30(S27, S28, S29, S01, S02, S03, S12, S13, S14);
			SMIX(S27, S28, S29, S00);
			CMIX30(S24, S25, S26, S28, S29, S00, S09, S10, S11);
			SMIX(S24, S25, S26, S27);
			NEXT(1);
			/* fall through */
		case 1:
			q = p;
			TIX2(q, S24, S25, S02, S04, S18);
			CMIX30(S21, S22, S23, S25, S26, S27, S06, S07, S08);
			SMIX(S21, S22, S23, S24);
			CMIX30(S18, S19, S20, S22, S23, S24, S03, S04, S05);
			SMIX(S18, S19, S20, S21);
			NEXT(2);
			/* fall through */
		case 2:
			q = p;
			TIX2(q, S18, S19, S26, S28, S12);
			CMIX30(S15, S16, S17, S19, S20, S21, S00, S01, S02);
			SMIX(S15, S16, S17, S18);
			CMIX30(S12, S13, S14, S16, S17, S18, S27, S28, S29);
			SMIX(S12, S13, S14, S15);
			NEXT(3);
			/* fall through */
		case 3:
			q = p;
			TIX2(q, S12, S13, S20, S22, S06);
			CMIX30(S09, S10, S11, S13, S14, S15, S24, S25, S26);
			SMIX(S09, S10, S11, S12);
			CMIX30(S06, S07, S08, S10, S11, S12, S21, S22, S23);
			SMIX(S06, S07, S08, S09);
			NEXT(4);
			/* fall through */
		case 4:
			q = p;
			TIX2(q, S06, S07, S14, S16, S00);
			CMIX30(S03, S04, S05, S07, S08, S09, S18, S19, S20);
			SMIX(S03, S04, S05, S06);
			CMIX30(S00, S01, S02, S04, S05, S06, S15, S16, S17);
			SMIX(S00, S01, S02, S03);
			NEXT(0);
		}
	}
	CORE_EXIT
	WRITE_STATE_SMALL(sc);
}

static void
fugue3_core(sph_fugue_context *sc, const void *data, size_t len)
{
	DECL_STATE_BIG
	CORE_ENTRY
	READ_STATE_BIG(sc);
	rshift = sc->round_shift;
	switch (rshift) {
		for (;;) {
			sph_u32 q;

		case 0:
			q = p;
			TIX3(q, S00, S01, S04, S08, S16, S27, S30);
			CMIX36(S33, S34, S35, S01, S02, S03, S15, S16, S17);
			SMIX(S33, S34, S35, S00);
			CMIX36(S30, S31, S32, S34, S35, S00, S12, S13, S14);
			SMIX(S30, S31, S32, S33);
			CMIX36(S27, S28, S29, S31, S32, S33, S09, S10, S11);
			SMIX(S27, S28, S29, S30);
			NEXT(1);
			/* fall through */
		case 1:
			q = p;
			TIX3(q, S27, S28, S31, S35, S07, S18, S21);
			CMIX36(S24, S25, S26, S28, S29, S30, S06, S07, S08);
			SMIX(S24, S25, S26, S27);
			CMIX36(S21, S22, S23, S25, S26, S27, S03, S04, S05);
			SMIX(S21, S22, S23, S24);
			CMIX36(S18, S19, S20, S22, S23, S24, S00, S01, S02);
			SMIX(S18, S19, S20, S21);
			NEXT(2);
			/* fall through */
		case 2:
			q = p;
			TIX3(q, S18, S19, S22, S26, S34, S09, S12);
			CMIX36(S15, S16, S17, S19, S20, S21, S33, S34, S35);
			SMIX(S15, S16, S17, S18);
			CMIX36(S12, S13, S14, S16, S17, S18, S30, S31, S32);
			SMIX(S12, S13, S14, S15);
			CMIX36(S09, S10, S11, S13, S14, S15, S27, S28, S29);
			SMIX(S09, S10, S11, S12);
			NEXT(3);
			/* fall through */
		case 3:
			q = p;
			TIX3(q, S09, S10, S13, S17, S25, S00, S03);
			CMIX36(S06, S07, S08, S10, S11, S12, S24, S25, S26);
			SMIX(S06, S07, S08, S09);
			CMIX36(S03, S04, S05, S07, S08, S09, S21, S22, S23);
			SMIX(S03, S04, S05, S06);
			CMIX36(S00, S01, S02, S04, S05, S06, S18, S19, S20);
			SMIX(S00, S01, S02, S03);
			NEXT(0);
		}
	}
	CORE_EXIT
	WRITE_STATE_BIG(sc);
}

static void
fugue4_core(sph_fugue_context *sc, const void *data, size_t len)
{
	DECL_STATE_BIG
	CORE_ENTRY
	READ_STATE_BIG(sc);
	rshift = sc->round_shift;
	switch (rshift) {
		for (;;) {
			sph_u32 q;

		case 0:
			q = p;
			TIX4(q, S00, S01, S04, S07, S08, S22, S24, S27, S30);
			CMIX36(S33, S34, S35, S01, S02, S03, S15, S16, S17);
			SMIX(S33, S34, S35, S00);
			CMIX36(S30, S31, S32, S34, S35, S00, S12, S13, S14);
			SMIX(S30, S31, S32, S33);
			CMIX36(S27, S28, S29, S31, S32, S33, S09, S10, S11);
			SMIX(S27, S28, S29, S30);
			CMIX36(S24, S25, S26, S28, S29, S30, S06, S07, S08);
			SMIX(S24, S25, S26, S27);
			NEXT(1);
			/* fall through */
		case 1:
			q = p;
			TIX4(q, S24, S25, S28, S31, S32, S10, S12, S15, S18);
			CMIX36(S21, S22, S23, S25, S26, S27, S03, S04, S05);
			SMIX(S21, S22, S23, S24);
			CMIX36(S18, S19, S20, S22, S23, S24, S00, S01, S02);
			SMIX(S18, S19, S20, S21);
			CMIX36(S15, S16, S17, S19, S20, S21, S33, S34, S35);
			SMIX(S15, S16, S17, S18);
			CMIX36(S12, S13, S14, S16, S17, S18, S30, S31, S32);
			SMIX(S12, S13, S14, S15);
			NEXT(2);
			/* fall through */
		case 2:
			q = p;
			TIX4(q, S12, S13, S16, S19, S20, S34, S00, S03, S06);
			CMIX36(S09, S10, S11, S13, S14, S15, S27, S28, S29);
			SMIX(S09, S10, S11, S12);
			CMIX36(S06, S07, S08, S10, S11, S12, S24, S25, S26);
			SMIX(S06, S07, S08, S09);
			CMIX36(S03, S04, S05, S07, S08, S09, S21, S22, S23);
			SMIX(S03, S04, S05, S06);
			CMIX36(S00, S01, S02, S04, S05, S06, S18, S19, S20);
			SMIX(S00, S01, S02, S03);
			NEXT(0);
		}
	}
	CORE_EXIT
	WRITE_STATE_BIG(sc);
}

#if SPH_64

#define WRITE_COUNTER   do { \
		sph_enc64be(buf + 4, sc->bit_count + n); \
	} while (0)

#else

#define WRITE_COUNTER   do { \
		sph_enc32be(buf + 4, sc->bit_count_high); \
		sph_enc32be(buf + 8, sc->bit_count_low + n); \
	} while (0)

#endif

#define CLOSE_ENTRY(s, rcm, core) \
	unsigned char buf[16]; \
	unsigned plen, rms; \
	unsigned char *out; \
	sph_u32 S[s]; \
	plen = sc->partial_len; \
	WRITE_COUNTER; \
	if (plen == 0 && n == 0) { \
		plen = 4; \
	} else if (plen < 4 || n != 0) { \
		unsigned u; \
 \
		if (plen == 4) \
			plen = 0; \
		buf[plen] = ub & ~(0xFFU >> n); \
		for (u = plen + 1; u < 4; u ++) \
			buf[u] = 0; \
	} \
	core(sc, buf + plen, (sizeof buf) - plen); \
	rms = sc->round_shift * (rcm); \
	memcpy(S, sc->S + (s) - rms, rms * sizeof(sph_u32)); \
	memcpy(S + rms, sc->S, ((s) - rms) * sizeof(sph_u32));

#define ROR(n, s)   do { \
		sph_u32 tmp[n]; \
		memcpy(tmp, S + ((s) - (n)), (n) * sizeof(sph_u32)); \
		memmove(S + (n), S, ((s) - (n)) * sizeof(sph_u32)); \
		memcpy(S, tmp, (n) * sizeof(sph_u32)); \
	} while (0)

static void
fugue2_close(sph_fugue_context *sc, unsigned ub, unsigned n,
	void *dst, size_t out_size_w32)
{
	int i;

	CLOSE_ENTRY(30, 6, fugue2_core)
	for (i = 0; i < 10; i ++) {
		ROR(3, 30);
		CMIX30(S[0], S[1], S[2], S[4], S[5], S[6], S[15], S[16], S[17]);
		SMIX(S[0], S[1], S[2], S[3]);
	}
	for (i = 0; i < 13; i ++) {
		S[4] ^= S[0];
		S[15] ^= S[0];
		ROR(15, 30);
		SMIX(S[0], S[1], S[2], S[3]);
		S[4] ^= S[0];
		S[16] ^= S[0];
		ROR(14, 30);
		SMIX(S[0], S[1], S[2], S[3]);
	}
	S[4] ^= S[0];
	S[15] ^= S[0];
	out = dst;
	sph_enc32be(out +  0, S[ 1]);
	sph_enc32be(out +  4, S[ 2]);
	sph_enc32be(out +  8, S[ 3]);
	sph_enc32be(out + 12, S[ 4]);
	sph_enc32be(out + 16, S[15]);
	sph_enc32be(out + 20, S[16]);
	sph_enc32be(out + 24, S[17]);
	if (out_size_w32 == 8) {
		sph_enc32be(out + 28, S[18]);
		sph_fugue256_init(sc);
	} else {
		sph_fugue224_init(sc);
	}
}

static void
fugue3_close(sph_fugue_context *sc, unsigned ub, unsigned n, void *dst)
{
	int i;

	CLOSE_ENTRY(36, 9, fugue3_core)
	for (i = 0; i < 18; i ++) {
		ROR(3, 36);
		CMIX36(S[0], S[1], S[2], S[4], S[5], S[6], S[18], S[19], S[20]);
		SMIX(S[0], S[1], S[2], S[3]);
	}
	for (i = 0; i < 13; i ++) {
		S[4] ^= S[0];
		S[12] ^= S[0];
		S[24] ^= S[0];
		ROR(12, 36);
		SMIX(S[0], S[1], S[2], S[3]);
		S[4] ^= S[0];
		S[13] ^= S[0];
		S[24] ^= S[0];
		ROR(12, 36);
		SMIX(S[0], S[1], S[2], S[3]);
		S[4] ^= S[0];
		S[13] ^= S[0];
		S[25] ^= S[0];
		ROR(11, 36);
		SMIX(S[0], S[1], S[2], S[3]);
	}
	S[4] ^= S[0];
	S[12] ^= S[0];
	S[24] ^= S[0];
	out = dst;
	sph_enc32be(out +  0, S[ 1]);
	sph_enc32be(out +  4, S[ 2]);
	sph_enc32be(out +  8, S[ 3]);
	sph_enc32be(out + 12, S[ 4]);
	sph_enc32be(out + 16, S[12]);
	sph_enc32be(out + 20, S[13]);
	sph_enc32be(out + 24, S[14]);
	sph_enc32be(out + 28, S[15]);
	sph_enc32be(out + 32, S[24]);
	sph_enc32be(out + 36, S[25]);
	sph_enc32be(out + 40, S[26]);
	sph_enc32be(out + 44, S[27]);
	sph_fugue384_init(sc);
}

static void
fugue4_close(sph_fugue_context *sc, unsigned ub, unsigned n, void *dst)
{
	int i;

	CLOSE_ENTRY(36, 12, fugue4_core)
	for (i = 0; i < 32; i ++) {
		ROR(3, 36);
		CMIX36(S[0], S[1], S[2], S[4], S[5], S[6], S[18], S[19], S[20]);
		SMIX(S[0], S[1], S[2], S[3]);
	}
	for (i = 0; i < 13; i ++) {
		S[4] ^= S[0];
		S[9] ^= S[0];
		S[18] ^= S[0];
		S[27] ^= S[0];
		ROR(9, 36);
		SMIX(S[0], S[1], S[2], S[3]);
		S[4] ^= S[0];
		S[10] ^= S[0];
		S[18] ^= S[0];
		S[27] ^= S[0];
		ROR(9, 36);
		SMIX(S[0], S[1], S[2], S[3]);
		S[4] ^= S[0];
		S[10] ^= S[0];
		S[19] ^= S[0];
		S[27] ^= S[0];
		ROR(9, 36);
		SMIX(S[0], S[1], S[2], S[3]);
		S[4] ^= S[0];
		S[10] ^= S[0];
		S[19] ^= S[0];
		S[28] ^= S[0];
		ROR(8, 36);
		SMIX(S[0], S[1], S[2], S[3]);
	}
	S[4] ^= S[0];
	S[9] ^= S[0];
	S[18] ^= S[0];
	S[27] ^= S[0];
	out = dst;
	sph_enc32be(out +  0, S[ 1]);
	sph_enc32be(out +  4, S[ 2]);
	sph_enc32be(out +  8, S[ 3]);
	sph_enc32be(out + 12, S[ 4]);
	sph_enc32be(out + 16, S[ 9]);
	sph_enc32be(out + 20, S[10]);
	sph_enc32be(out + 24, S[11]);
	sph_enc32be(out + 28, S[12]);
	sph_enc32be(out + 32, S[18]);
	sph_enc32be(out + 36, S[19]);
	sph_enc32be(out + 40, S[20]);
	sph_enc32be(out + 44, S[21]);
	sph_enc32be(out + 48, S[27]);
	sph_enc32be(out + 52, S[28]);
	sph_enc32be(out + 56, S[29]);
	sph_enc32be(out + 60, S[30]);
//	sph_fugue512_init(sc);
}

void
sph_fugue224_init(void *cc)
{
	fugue_init(cc, 23, IV224, 7);
}

void
sph_fugue224(void *cc, const void *data, size_t len)
{
	fugue2_core(cc, data, len);
}

void
sph_fugue224_close(void *cc, void *dst)
{
	fugue2_close(cc, 0, 0, dst, 7);
}

void
sph_fugue224_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	fugue2_close(cc, ub, n, dst, 7);
}

void
sph_fugue256_init(void *cc)
{
	fugue_init(cc, 22, IV256, 8);
}

void
sph_fugue256(void *cc, const void *data, size_t len)
{
	fugue2_core(cc, data, len);
}

void
sph_fugue256_close(void *cc, void *dst)
{
	fugue2_close(cc, 0, 0, dst, 8);
}

void
sph_fugue256_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	fugue2_close(cc, ub, n, dst, 8);
}

void
sph_fugue384_init(void *cc)
{
	fugue_init(cc, 24, IV384, 12);
}

void
sph_fugue384(void *cc, const void *data, size_t len)
{
	fugue3_core(cc, data, len);
}

void
sph_fugue384_close(void *cc, void *dst)
{
	fugue3_close(cc, 0, 0, dst);
}

void
sph_fugue384_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	fugue3_close(cc, ub, n, dst);
}

void
sph_fugue512_init(void *cc)
{
	fugue_init(cc, 20, IV512, 16);
}

void
sph_fugue512(void *cc, const void *data, size_t len)
{
	fugue4_core(cc, data, len);
}

void
sph_fugue512_close(void *cc, void *dst)
{
	fugue4_close(cc, 0, 0, dst);
}

void
sph_fugue512_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst)
{
	fugue4_close(cc, ub, n, dst);
}
#ifdef __cplusplus
}
#endif
