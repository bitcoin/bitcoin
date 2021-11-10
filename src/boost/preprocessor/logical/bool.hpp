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
# ifndef BOOST_PREPROCESSOR_LOGICAL_BOOL_HPP
# define BOOST_PREPROCESSOR_LOGICAL_BOOL_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# /* BOOST_PP_BOOL */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_BOOL(x) BOOST_PP_BOOL_I(x)
# else
#    define BOOST_PP_BOOL(x) BOOST_PP_BOOL_OO((x))
#    define BOOST_PP_BOOL_OO(par) BOOST_PP_BOOL_I ## par
# endif
#
# define BOOST_PP_BOOL_I(x) BOOST_PP_BOOL_ ## x
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# define BOOST_PP_BOOL_0 0
# define BOOST_PP_BOOL_1 1
# define BOOST_PP_BOOL_2 1
# define BOOST_PP_BOOL_3 1
# define BOOST_PP_BOOL_4 1
# define BOOST_PP_BOOL_5 1
# define BOOST_PP_BOOL_6 1
# define BOOST_PP_BOOL_7 1
# define BOOST_PP_BOOL_8 1
# define BOOST_PP_BOOL_9 1
# define BOOST_PP_BOOL_10 1
# define BOOST_PP_BOOL_11 1
# define BOOST_PP_BOOL_12 1
# define BOOST_PP_BOOL_13 1
# define BOOST_PP_BOOL_14 1
# define BOOST_PP_BOOL_15 1
# define BOOST_PP_BOOL_16 1
# define BOOST_PP_BOOL_17 1
# define BOOST_PP_BOOL_18 1
# define BOOST_PP_BOOL_19 1
# define BOOST_PP_BOOL_20 1
# define BOOST_PP_BOOL_21 1
# define BOOST_PP_BOOL_22 1
# define BOOST_PP_BOOL_23 1
# define BOOST_PP_BOOL_24 1
# define BOOST_PP_BOOL_25 1
# define BOOST_PP_BOOL_26 1
# define BOOST_PP_BOOL_27 1
# define BOOST_PP_BOOL_28 1
# define BOOST_PP_BOOL_29 1
# define BOOST_PP_BOOL_30 1
# define BOOST_PP_BOOL_31 1
# define BOOST_PP_BOOL_32 1
# define BOOST_PP_BOOL_33 1
# define BOOST_PP_BOOL_34 1
# define BOOST_PP_BOOL_35 1
# define BOOST_PP_BOOL_36 1
# define BOOST_PP_BOOL_37 1
# define BOOST_PP_BOOL_38 1
# define BOOST_PP_BOOL_39 1
# define BOOST_PP_BOOL_40 1
# define BOOST_PP_BOOL_41 1
# define BOOST_PP_BOOL_42 1
# define BOOST_PP_BOOL_43 1
# define BOOST_PP_BOOL_44 1
# define BOOST_PP_BOOL_45 1
# define BOOST_PP_BOOL_46 1
# define BOOST_PP_BOOL_47 1
# define BOOST_PP_BOOL_48 1
# define BOOST_PP_BOOL_49 1
# define BOOST_PP_BOOL_50 1
# define BOOST_PP_BOOL_51 1
# define BOOST_PP_BOOL_52 1
# define BOOST_PP_BOOL_53 1
# define BOOST_PP_BOOL_54 1
# define BOOST_PP_BOOL_55 1
# define BOOST_PP_BOOL_56 1
# define BOOST_PP_BOOL_57 1
# define BOOST_PP_BOOL_58 1
# define BOOST_PP_BOOL_59 1
# define BOOST_PP_BOOL_60 1
# define BOOST_PP_BOOL_61 1
# define BOOST_PP_BOOL_62 1
# define BOOST_PP_BOOL_63 1
# define BOOST_PP_BOOL_64 1
# define BOOST_PP_BOOL_65 1
# define BOOST_PP_BOOL_66 1
# define BOOST_PP_BOOL_67 1
# define BOOST_PP_BOOL_68 1
# define BOOST_PP_BOOL_69 1
# define BOOST_PP_BOOL_70 1
# define BOOST_PP_BOOL_71 1
# define BOOST_PP_BOOL_72 1
# define BOOST_PP_BOOL_73 1
# define BOOST_PP_BOOL_74 1
# define BOOST_PP_BOOL_75 1
# define BOOST_PP_BOOL_76 1
# define BOOST_PP_BOOL_77 1
# define BOOST_PP_BOOL_78 1
# define BOOST_PP_BOOL_79 1
# define BOOST_PP_BOOL_80 1
# define BOOST_PP_BOOL_81 1
# define BOOST_PP_BOOL_82 1
# define BOOST_PP_BOOL_83 1
# define BOOST_PP_BOOL_84 1
# define BOOST_PP_BOOL_85 1
# define BOOST_PP_BOOL_86 1
# define BOOST_PP_BOOL_87 1
# define BOOST_PP_BOOL_88 1
# define BOOST_PP_BOOL_89 1
# define BOOST_PP_BOOL_90 1
# define BOOST_PP_BOOL_91 1
# define BOOST_PP_BOOL_92 1
# define BOOST_PP_BOOL_93 1
# define BOOST_PP_BOOL_94 1
# define BOOST_PP_BOOL_95 1
# define BOOST_PP_BOOL_96 1
# define BOOST_PP_BOOL_97 1
# define BOOST_PP_BOOL_98 1
# define BOOST_PP_BOOL_99 1
# define BOOST_PP_BOOL_100 1
# define BOOST_PP_BOOL_101 1
# define BOOST_PP_BOOL_102 1
# define BOOST_PP_BOOL_103 1
# define BOOST_PP_BOOL_104 1
# define BOOST_PP_BOOL_105 1
# define BOOST_PP_BOOL_106 1
# define BOOST_PP_BOOL_107 1
# define BOOST_PP_BOOL_108 1
# define BOOST_PP_BOOL_109 1
# define BOOST_PP_BOOL_110 1
# define BOOST_PP_BOOL_111 1
# define BOOST_PP_BOOL_112 1
# define BOOST_PP_BOOL_113 1
# define BOOST_PP_BOOL_114 1
# define BOOST_PP_BOOL_115 1
# define BOOST_PP_BOOL_116 1
# define BOOST_PP_BOOL_117 1
# define BOOST_PP_BOOL_118 1
# define BOOST_PP_BOOL_119 1
# define BOOST_PP_BOOL_120 1
# define BOOST_PP_BOOL_121 1
# define BOOST_PP_BOOL_122 1
# define BOOST_PP_BOOL_123 1
# define BOOST_PP_BOOL_124 1
# define BOOST_PP_BOOL_125 1
# define BOOST_PP_BOOL_126 1
# define BOOST_PP_BOOL_127 1
# define BOOST_PP_BOOL_128 1
# define BOOST_PP_BOOL_129 1
# define BOOST_PP_BOOL_130 1
# define BOOST_PP_BOOL_131 1
# define BOOST_PP_BOOL_132 1
# define BOOST_PP_BOOL_133 1
# define BOOST_PP_BOOL_134 1
# define BOOST_PP_BOOL_135 1
# define BOOST_PP_BOOL_136 1
# define BOOST_PP_BOOL_137 1
# define BOOST_PP_BOOL_138 1
# define BOOST_PP_BOOL_139 1
# define BOOST_PP_BOOL_140 1
# define BOOST_PP_BOOL_141 1
# define BOOST_PP_BOOL_142 1
# define BOOST_PP_BOOL_143 1
# define BOOST_PP_BOOL_144 1
# define BOOST_PP_BOOL_145 1
# define BOOST_PP_BOOL_146 1
# define BOOST_PP_BOOL_147 1
# define BOOST_PP_BOOL_148 1
# define BOOST_PP_BOOL_149 1
# define BOOST_PP_BOOL_150 1
# define BOOST_PP_BOOL_151 1
# define BOOST_PP_BOOL_152 1
# define BOOST_PP_BOOL_153 1
# define BOOST_PP_BOOL_154 1
# define BOOST_PP_BOOL_155 1
# define BOOST_PP_BOOL_156 1
# define BOOST_PP_BOOL_157 1
# define BOOST_PP_BOOL_158 1
# define BOOST_PP_BOOL_159 1
# define BOOST_PP_BOOL_160 1
# define BOOST_PP_BOOL_161 1
# define BOOST_PP_BOOL_162 1
# define BOOST_PP_BOOL_163 1
# define BOOST_PP_BOOL_164 1
# define BOOST_PP_BOOL_165 1
# define BOOST_PP_BOOL_166 1
# define BOOST_PP_BOOL_167 1
# define BOOST_PP_BOOL_168 1
# define BOOST_PP_BOOL_169 1
# define BOOST_PP_BOOL_170 1
# define BOOST_PP_BOOL_171 1
# define BOOST_PP_BOOL_172 1
# define BOOST_PP_BOOL_173 1
# define BOOST_PP_BOOL_174 1
# define BOOST_PP_BOOL_175 1
# define BOOST_PP_BOOL_176 1
# define BOOST_PP_BOOL_177 1
# define BOOST_PP_BOOL_178 1
# define BOOST_PP_BOOL_179 1
# define BOOST_PP_BOOL_180 1
# define BOOST_PP_BOOL_181 1
# define BOOST_PP_BOOL_182 1
# define BOOST_PP_BOOL_183 1
# define BOOST_PP_BOOL_184 1
# define BOOST_PP_BOOL_185 1
# define BOOST_PP_BOOL_186 1
# define BOOST_PP_BOOL_187 1
# define BOOST_PP_BOOL_188 1
# define BOOST_PP_BOOL_189 1
# define BOOST_PP_BOOL_190 1
# define BOOST_PP_BOOL_191 1
# define BOOST_PP_BOOL_192 1
# define BOOST_PP_BOOL_193 1
# define BOOST_PP_BOOL_194 1
# define BOOST_PP_BOOL_195 1
# define BOOST_PP_BOOL_196 1
# define BOOST_PP_BOOL_197 1
# define BOOST_PP_BOOL_198 1
# define BOOST_PP_BOOL_199 1
# define BOOST_PP_BOOL_200 1
# define BOOST_PP_BOOL_201 1
# define BOOST_PP_BOOL_202 1
# define BOOST_PP_BOOL_203 1
# define BOOST_PP_BOOL_204 1
# define BOOST_PP_BOOL_205 1
# define BOOST_PP_BOOL_206 1
# define BOOST_PP_BOOL_207 1
# define BOOST_PP_BOOL_208 1
# define BOOST_PP_BOOL_209 1
# define BOOST_PP_BOOL_210 1
# define BOOST_PP_BOOL_211 1
# define BOOST_PP_BOOL_212 1
# define BOOST_PP_BOOL_213 1
# define BOOST_PP_BOOL_214 1
# define BOOST_PP_BOOL_215 1
# define BOOST_PP_BOOL_216 1
# define BOOST_PP_BOOL_217 1
# define BOOST_PP_BOOL_218 1
# define BOOST_PP_BOOL_219 1
# define BOOST_PP_BOOL_220 1
# define BOOST_PP_BOOL_221 1
# define BOOST_PP_BOOL_222 1
# define BOOST_PP_BOOL_223 1
# define BOOST_PP_BOOL_224 1
# define BOOST_PP_BOOL_225 1
# define BOOST_PP_BOOL_226 1
# define BOOST_PP_BOOL_227 1
# define BOOST_PP_BOOL_228 1
# define BOOST_PP_BOOL_229 1
# define BOOST_PP_BOOL_230 1
# define BOOST_PP_BOOL_231 1
# define BOOST_PP_BOOL_232 1
# define BOOST_PP_BOOL_233 1
# define BOOST_PP_BOOL_234 1
# define BOOST_PP_BOOL_235 1
# define BOOST_PP_BOOL_236 1
# define BOOST_PP_BOOL_237 1
# define BOOST_PP_BOOL_238 1
# define BOOST_PP_BOOL_239 1
# define BOOST_PP_BOOL_240 1
# define BOOST_PP_BOOL_241 1
# define BOOST_PP_BOOL_242 1
# define BOOST_PP_BOOL_243 1
# define BOOST_PP_BOOL_244 1
# define BOOST_PP_BOOL_245 1
# define BOOST_PP_BOOL_246 1
# define BOOST_PP_BOOL_247 1
# define BOOST_PP_BOOL_248 1
# define BOOST_PP_BOOL_249 1
# define BOOST_PP_BOOL_250 1
# define BOOST_PP_BOOL_251 1
# define BOOST_PP_BOOL_252 1
# define BOOST_PP_BOOL_253 1
# define BOOST_PP_BOOL_254 1
# define BOOST_PP_BOOL_255 1
# define BOOST_PP_BOOL_256 1
#
# else
#
# include <boost/preprocessor/config/limits.hpp>
#
# if BOOST_PP_LIMIT_MAG == 256
# include <boost/preprocessor/logical/limits/bool_256.hpp>
# elif BOOST_PP_LIMIT_MAG == 512
# include <boost/preprocessor/logical/limits/bool_256.hpp>
# include <boost/preprocessor/logical/limits/bool_512.hpp>
# elif BOOST_PP_LIMIT_MAG == 1024
# include <boost/preprocessor/logical/limits/bool_256.hpp>
# include <boost/preprocessor/logical/limits/bool_512.hpp>
# include <boost/preprocessor/logical/limits/bool_1024.hpp>
# else
# error Incorrect value for the BOOST_PP_LIMIT_MAG limit
# endif
#
# endif
#
# endif
