# /* Copyright (C) 2001
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
# /* Revised by Edward Diener (2020) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_REPETITION_FOR_HPP
# define BOOST_PREPROCESSOR_REPETITION_FOR_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/debug/error.hpp>
# include <boost/preprocessor/facilities/empty.hpp>
# include <boost/preprocessor/logical/bool.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
#
# /* BOOST_PP_FOR */
#
# if 0
#    define BOOST_PP_FOR(state, pred, op, macro)
# endif
#
# define BOOST_PP_FOR BOOST_PP_CAT(BOOST_PP_FOR_, BOOST_PP_AUTO_REC(BOOST_PP_FOR_P, 256))
#
# define BOOST_PP_FOR_P(n) BOOST_PP_CAT(BOOST_PP_FOR_CHECK_, BOOST_PP_FOR_ ## n(1, BOOST_PP_FOR_SR_P, BOOST_PP_FOR_SR_O, BOOST_PP_FOR_SR_M))
#
# define BOOST_PP_FOR_SR_P(r, s) s
# define BOOST_PP_FOR_SR_O(r, s) 0
# define BOOST_PP_FOR_SR_M(r, s) BOOST_PP_NIL
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    include <boost/preprocessor/repetition/detail/edg/for.hpp>
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    include <boost/preprocessor/repetition/detail/msvc/for.hpp>
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
#    include <boost/preprocessor/repetition/detail/dmc/for.hpp>
# else
#    include <boost/preprocessor/repetition/detail/for.hpp>
# endif
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
# define BOOST_PP_FOR_257_PR(s, p) BOOST_PP_BOOL(p##(257, s))
# else
# define BOOST_PP_FOR_257_PR(s, p) BOOST_PP_BOOL(p(257, s))
# endif

# define BOOST_PP_FOR_257_ERROR() BOOST_PP_ERROR(0x0002)
# define BOOST_PP_FOR_257(s, p, o, m) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_FOR_257_PR(s,p), \
        BOOST_PP_FOR_257_ERROR, \
        BOOST_PP_EMPTY \
        ) \
    () \
/**/
// # define BOOST_PP_FOR_257(s, p, o, m) BOOST_PP_ERROR(0x0002)
#
# define BOOST_PP_FOR_CHECK_BOOST_PP_NIL 1
#
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_1(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_2(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_3(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_4(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_5(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_6(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_7(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_8(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_9(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_10(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_11(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_12(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_13(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_14(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_15(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_16(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_17(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_18(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_19(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_20(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_21(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_22(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_23(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_24(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_25(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_26(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_27(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_28(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_29(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_30(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_31(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_32(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_33(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_34(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_35(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_36(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_37(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_38(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_39(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_40(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_41(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_42(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_43(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_44(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_45(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_46(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_47(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_48(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_49(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_50(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_51(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_52(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_53(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_54(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_55(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_56(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_57(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_58(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_59(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_60(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_61(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_62(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_63(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_64(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_65(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_66(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_67(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_68(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_69(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_70(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_71(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_72(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_73(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_74(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_75(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_76(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_77(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_78(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_79(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_80(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_81(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_82(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_83(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_84(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_85(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_86(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_87(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_88(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_89(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_90(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_91(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_92(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_93(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_94(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_95(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_96(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_97(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_98(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_99(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_100(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_101(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_102(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_103(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_104(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_105(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_106(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_107(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_108(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_109(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_110(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_111(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_112(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_113(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_114(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_115(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_116(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_117(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_118(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_119(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_120(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_121(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_122(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_123(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_124(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_125(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_126(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_127(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_128(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_129(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_130(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_131(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_132(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_133(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_134(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_135(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_136(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_137(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_138(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_139(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_140(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_141(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_142(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_143(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_144(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_145(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_146(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_147(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_148(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_149(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_150(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_151(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_152(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_153(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_154(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_155(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_156(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_157(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_158(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_159(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_160(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_161(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_162(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_163(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_164(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_165(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_166(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_167(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_168(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_169(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_170(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_171(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_172(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_173(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_174(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_175(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_176(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_177(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_178(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_179(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_180(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_181(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_182(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_183(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_184(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_185(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_186(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_187(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_188(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_189(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_190(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_191(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_192(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_193(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_194(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_195(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_196(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_197(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_198(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_199(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_200(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_201(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_202(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_203(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_204(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_205(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_206(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_207(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_208(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_209(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_210(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_211(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_212(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_213(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_214(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_215(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_216(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_217(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_218(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_219(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_220(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_221(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_222(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_223(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_224(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_225(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_226(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_227(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_228(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_229(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_230(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_231(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_232(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_233(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_234(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_235(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_236(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_237(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_238(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_239(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_240(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_241(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_242(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_243(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_244(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_245(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_246(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_247(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_248(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_249(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_250(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_251(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_252(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_253(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_254(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_255(s, p, o, m) 0
# define BOOST_PP_FOR_CHECK_BOOST_PP_FOR_256(s, p, o, m) 0
#
# else
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/debug/error.hpp>
# include <boost/preprocessor/facilities/empty.hpp>
# include <boost/preprocessor/logical/bool.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
# include <boost/preprocessor/config/limits.hpp>
#
# /* BOOST_PP_FOR */
#
# if 0
#    define BOOST_PP_FOR(state, pred, op, macro)
# endif
#
# if BOOST_PP_LIMIT_FOR == 256
# define BOOST_PP_FOR BOOST_PP_CAT(BOOST_PP_FOR_, BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_FOR_P, 256)))
# elif BOOST_PP_LIMIT_FOR == 512
# define BOOST_PP_FOR BOOST_PP_CAT(BOOST_PP_FOR_, BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_FOR_P, 512)))
# elif BOOST_PP_LIMIT_FOR == 1024
# define BOOST_PP_FOR BOOST_PP_CAT(BOOST_PP_FOR_, BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_FOR_P, 1024)))
# else
# error Incorrect value for the BOOST_PP_LIMIT_FOR limit
# endif
#
# define BOOST_PP_FOR_P(n) BOOST_PP_FOR_P_DEC(BOOST_PP_DEC(n))
# define BOOST_PP_FOR_P_DEC(n) BOOST_PP_CAT(BOOST_PP_FOR_CHECK_, BOOST_PP_CAT(BOOST_PP_FOR_ , n)(1, BOOST_PP_FOR_SR_P, BOOST_PP_FOR_SR_O, BOOST_PP_FOR_SR_M))
#
# define BOOST_PP_FOR_SR_P(r, s) s
# define BOOST_PP_FOR_SR_O(r, s) 0
# define BOOST_PP_FOR_SR_M(r, s) BOOST_PP_NIL
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    include <boost/preprocessor/repetition/detail/edg/for.hpp>
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    include <boost/preprocessor/repetition/detail/msvc/for.hpp>
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
#    include <boost/preprocessor/repetition/detail/dmc/for.hpp>
# else
#    include <boost/preprocessor/repetition/detail/for.hpp>
# endif
#
# if BOOST_PP_LIMIT_FOR == 256
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
# define BOOST_PP_FOR_257_PR(s, p) BOOST_PP_BOOL(p##(257, s))
# else
# define BOOST_PP_FOR_257_PR(s, p) BOOST_PP_BOOL(p(257, s))
# endif

# define BOOST_PP_FOR_257_ERROR() BOOST_PP_ERROR(0x0002)
# define BOOST_PP_FOR_257(s, p, o, m) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_FOR_257_PR(s,p), \
        BOOST_PP_FOR_257_ERROR, \
        BOOST_PP_EMPTY \
        ) \
    () \
/**/
#
# elif BOOST_PP_LIMIT_FOR == 512
#
# define BOOST_PP_FOR_513_PR(s, p) BOOST_PP_BOOL(p(513, s))

# define BOOST_PP_FOR_513_ERROR() BOOST_PP_ERROR(0x0002)
# define BOOST_PP_FOR_513(s, p, o, m) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_FOR_513_PR(s,p), \
        BOOST_PP_FOR_513_ERROR, \
        BOOST_PP_EMPTY \
        ) \
    () \
/**/
#
# elif BOOST_PP_LIMIT_FOR == 1024
#
# define BOOST_PP_FOR_1025_PR(s, p) BOOST_PP_BOOL(p(1025, s))

# define BOOST_PP_FOR_1025_ERROR() BOOST_PP_ERROR(0x0002)
# define BOOST_PP_FOR_1025(s, p, o, m) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_FOR_1025_PR(s,p), \
        BOOST_PP_FOR_1025_ERROR, \
        BOOST_PP_EMPTY \
        ) \
    () \
/**/
#
# endif
#
# define BOOST_PP_FOR_CHECK_BOOST_PP_NIL 1
#
# if BOOST_PP_LIMIT_FOR == 256
# include <boost/preprocessor/repetition/limits/for_256.hpp>
# elif BOOST_PP_LIMIT_FOR == 512
# include <boost/preprocessor/repetition/limits/for_256.hpp>
# include <boost/preprocessor/repetition/limits/for_512.hpp>
# elif BOOST_PP_LIMIT_FOR == 1024
# include <boost/preprocessor/repetition/limits/for_256.hpp>
# include <boost/preprocessor/repetition/limits/for_512.hpp>
# include <boost/preprocessor/repetition/limits/for_1024.hpp>
# endif
#
# endif
#
# endif
