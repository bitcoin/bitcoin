/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2013 RELIC Authors
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
 * Symbol renaming to a#undef clashes when simultaneous linking multiple builds.
 *
 * @ingroup core
 */

#ifndef RLC_LABEL_H
#define RLC_LABEL_H

#include "relic_conf.h"

#define RLC_PREFIX(F)			_RLC_PREFIX(LABEL, F)
#define _RLC_PREFIX(A, B)		__RLC_PREFIX(A, B)
#define __RLC_PREFIX(A, B)		A ## _ ## B

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

#ifdef LABEL

#undef first_ctx
#define first_ctx     RLC_PREFIX(first_ctx)
#undef core_ctx
#define core_ctx      RLC_PREFIX(core_ctx)

#undef core_init
#undef core_clean
#undef core_get
#undef core_set
#undef core_set_thread_initializer

#define core_init 	RLC_PREFIX(core_init)
#define core_clean 	RLC_PREFIX(core_clean)
#define core_get 	RLC_PREFIX(core_get)
#define core_set 	RLC_PREFIX(core_set)
#define core_set_thread_initializer 	RLC_PREFIX(core_set_thread_initializer)

#undef arch_init
#undef arch_clean
#undef arch_cycles
#undef arch_lzcnt
#undef arch_copy_rom

#define arch_init 	RLC_PREFIX(arch_init)
#define arch_clean 	RLC_PREFIX(arch_clean)
#define arch_cycles 	RLC_PREFIX(arch_cycles)
#define arch_lzcnt 	RLC_PREFIX(arch_lzcnt)
#define arch_copy_rom 	RLC_PREFIX(arch_copy_rom)

#undef bench_init
#undef bench_clean
#undef bench_overhead
#undef bench_reset
#undef bench_before
#undef bench_after
#undef bench_compute
#undef bench_print

#define bench_init 	RLC_PREFIX(bench_init)
#define bench_clean 	RLC_PREFIX(bench_clean)
#define bench_overhead 	RLC_PREFIX(bench_overhead)
#define bench_reset 	RLC_PREFIX(bench_reset)
#define bench_before 	RLC_PREFIX(bench_before)
#define bench_after 	RLC_PREFIX(bench_after)
#define bench_compute 	RLC_PREFIX(bench_compute)
#define bench_print 	RLC_PREFIX(bench_print)

#undef err_simple_msg
#undef err_full_msg
#undef err_get_msg
#undef err_get_code

#define err_simple_msg 	RLC_PREFIX(err_simple_msg)
#define err_full_msg 	RLC_PREFIX(err_full_msg)
#define err_get_msg 	RLC_PREFIX(err_get_msg)
#define err_get_code 	RLC_PREFIX(err_get_code)

#undef rand_init
#undef rand_clean
#undef rand_seed
#undef rand_seed
#undef rand_bytes

#define rand_init 	RLC_PREFIX(rand_init)
#define rand_clean 	RLC_PREFIX(rand_clean)
#define rand_seed 	RLC_PREFIX(rand_seed)
#define rand_seed 	RLC_PREFIX(rand_seed)
#define rand_bytes 	RLC_PREFIX(rand_bytes)

#undef test_fail
#undef test_pass

#define test_fail 	RLC_PREFIX(test_fail)
#define test_pass 	RLC_PREFIX(test_pass)

#undef util_conv_endian
#undef util_conv_big
#undef util_conv_little
#undef util_conv_char
#undef util_bits_dig
#undef util_cmp_const
#undef util_printf
#undef util_print_dig

#define util_conv_endian 	RLC_PREFIX(util_conv_endian)
#define util_conv_big 	RLC_PREFIX(util_conv_big)
#define util_conv_little 	RLC_PREFIX(util_conv_little)
#define util_conv_char 	RLC_PREFIX(util_conv_char)
#define util_bits_dig 	RLC_PREFIX(util_bits_dig)
#define util_cmp_const 	RLC_PREFIX(util_cmp_const)
#define util_printf 	RLC_PREFIX(util_printf)
#define util_print_dig 	RLC_PREFIX(util_print_dig)

#undef conf_print
#define conf_print    RLC_PREFIX(conf_print)

#undef dv_t
#define dv_t          RLC_PREFIX(dv_t)

#undef dv_print
#undef dv_zero
#undef dv_copy
#undef dv_copy_cond
#undef dv_swap_cond
#undef dv_cmp
#undef dv_cmp_const
#undef dv_new_dynam
#undef dv_free_dynam
#undef dv_lshd
#undef dv_rshd

#define dv_print 	RLC_PREFIX(dv_print)
#define dv_zero 	RLC_PREFIX(dv_zero)
#define dv_copy 	RLC_PREFIX(dv_copy)
#define dv_copy_cond 	RLC_PREFIX(dv_copy_cond)
#define dv_swap_cond 	RLC_PREFIX(dv_swap_cond)
#define dv_cmp 	RLC_PREFIX(dv_cmp)
#define dv_cmp_const 	RLC_PREFIX(dv_cmp_const)
#define dv_new_dynam 	RLC_PREFIX(dv_new_dynam)
#define dv_free_dynam 	RLC_PREFIX(dv_free_dynam)
#define dv_lshd 	RLC_PREFIX(dv_lshd)
#define dv_rshd 	RLC_PREFIX(dv_rshd)



#undef bn_st
#undef bn_t
#define bn_st     	RLC_PREFIX(bn_st)
#define bn_t      	RLC_PREFIX(bn_t)

#undef bn_make
#undef bn_clean
#undef bn_grow
#undef bn_trim
#undef bn_copy
#undef bn_abs
#undef bn_neg
#undef bn_sign
#undef bn_zero
#undef bn_is_zero
#undef bn_is_even
#undef bn_bits
#undef bn_get_bit
#undef bn_set_bit
#undef bn_ham
#undef bn_get_dig
#undef bn_set_dig
#undef bn_set_2b
#undef bn_rand
#undef bn_rand_mod
#undef bn_print
#undef bn_size_str
#undef bn_read_str
#undef bn_write_str
#undef bn_size_bin
#undef bn_read_bin
#undef bn_write_bin
#undef bn_size_raw
#undef bn_read_raw
#undef bn_write_raw
#undef bn_cmp_abs
#undef bn_cmp_dig
#undef bn_cmp
#undef bn_add
#undef bn_add_dig
#undef bn_sub
#undef bn_sub_dig
#undef bn_mul_dig
#undef bn_mul_basic
#undef bn_mul_comba
#undef bn_mul_karat
#undef bn_sqr_basic
#undef bn_sqr_comba
#undef bn_sqr_karat
#undef bn_dbl
#undef bn_hlv
#undef bn_lsh
#undef bn_rsh
#undef bn_div
#undef bn_div_rem
#undef bn_div_dig
#undef bn_div_rem_dig
#undef bn_mod_inv
#undef bn_mod_2b
#undef bn_mod_dig
#undef bn_mod_basic
#undef bn_mod_pre_barrt
#undef bn_mod_barrt
#undef bn_mod_pre_monty
#undef bn_mod_monty_conv
#undef bn_mod_monty_back
#undef bn_mod_monty_basic
#undef bn_mod_monty_comba
#undef bn_mod_pre_pmers
#undef bn_mod_pmers
#undef bn_mxp_basic
#undef bn_mxp_slide
#undef bn_mxp_monty
#undef bn_mxp_dig
#undef bn_srt
#undef bn_gcd_basic
#undef bn_gcd_lehme
#undef bn_gcd_stein
#undef bn_gcd_dig
#undef bn_gcd_ext_basic
#undef bn_gcd_ext_lehme
#undef bn_gcd_ext_stein
#undef bn_gcd_ext_mid
#undef bn_gcd_ext_dig
#undef bn_lcm
#undef bn_smb_leg
#undef bn_smb_jac
#undef bn_get_prime
#undef bn_is_prime
#undef bn_is_prime_basic
#undef bn_is_prime_rabin
#undef bn_is_prime_solov
#undef bn_gen_prime_basic
#undef bn_gen_prime_safep
#undef bn_gen_prime_stron
#undef bn_factor
#undef bn_is_factor
#undef bn_rec_win
#undef bn_rec_slw
#undef bn_rec_naf
#undef bn_rec_tnaf
#undef bn_rec_rtnaf
#undef bn_rec_tnaf_get
#undef bn_rec_tnaf_mod
#undef bn_rec_reg
#undef bn_rec_jsf
#undef bn_rec_glv

#define bn_make 	RLC_PREFIX(bn_make)
#define bn_clean 	RLC_PREFIX(bn_clean)
#define bn_grow 	RLC_PREFIX(bn_grow)
#define bn_trim 	RLC_PREFIX(bn_trim)
#define bn_copy 	RLC_PREFIX(bn_copy)
#define bn_abs 	RLC_PREFIX(bn_abs)
#define bn_neg 	RLC_PREFIX(bn_neg)
#define bn_sign 	RLC_PREFIX(bn_sign)
#define bn_zero 	RLC_PREFIX(bn_zero)
#define bn_is_zero 	RLC_PREFIX(bn_is_zero)
#define bn_is_even 	RLC_PREFIX(bn_is_even)
#define bn_bits 	RLC_PREFIX(bn_bits)
#define bn_get_bit 	RLC_PREFIX(bn_get_bit)
#define bn_set_bit 	RLC_PREFIX(bn_set_bit)
#define bn_ham 	RLC_PREFIX(bn_ham)
#define bn_get_dig 	RLC_PREFIX(bn_get_dig)
#define bn_set_dig 	RLC_PREFIX(bn_set_dig)
#define bn_set_2b 	RLC_PREFIX(bn_set_2b)
#define bn_rand 	RLC_PREFIX(bn_rand)
#define bn_rand_mod 	RLC_PREFIX(bn_rand_mod)
#define bn_print 	RLC_PREFIX(bn_print)
#define bn_size_str 	RLC_PREFIX(bn_size_str)
#define bn_read_str 	RLC_PREFIX(bn_read_str)
#define bn_write_str 	RLC_PREFIX(bn_write_str)
#define bn_size_bin 	RLC_PREFIX(bn_size_bin)
#define bn_read_bin 	RLC_PREFIX(bn_read_bin)
#define bn_write_bin 	RLC_PREFIX(bn_write_bin)
#define bn_size_raw 	RLC_PREFIX(bn_size_raw)
#define bn_read_raw 	RLC_PREFIX(bn_read_raw)
#define bn_write_raw 	RLC_PREFIX(bn_write_raw)
#define bn_cmp_abs 	RLC_PREFIX(bn_cmp_abs)
#define bn_cmp_dig 	RLC_PREFIX(bn_cmp_dig)
#define bn_cmp 	RLC_PREFIX(bn_cmp)
#define bn_add 	RLC_PREFIX(bn_add)
#define bn_add_dig 	RLC_PREFIX(bn_add_dig)
#define bn_sub 	RLC_PREFIX(bn_sub)
#define bn_sub_dig 	RLC_PREFIX(bn_sub_dig)
#define bn_mul_dig 	RLC_PREFIX(bn_mul_dig)
#define bn_mul_basic 	RLC_PREFIX(bn_mul_basic)
#define bn_mul_comba 	RLC_PREFIX(bn_mul_comba)
#define bn_mul_karat 	RLC_PREFIX(bn_mul_karat)
#define bn_sqr_basic 	RLC_PREFIX(bn_sqr_basic)
#define bn_sqr_comba 	RLC_PREFIX(bn_sqr_comba)
#define bn_sqr_karat 	RLC_PREFIX(bn_sqr_karat)
#define bn_dbl 	RLC_PREFIX(bn_dbl)
#define bn_hlv 	RLC_PREFIX(bn_hlv)
#define bn_lsh 	RLC_PREFIX(bn_lsh)
#define bn_rsh 	RLC_PREFIX(bn_rsh)
#define bn_div 	RLC_PREFIX(bn_div)
#define bn_div_rem 	RLC_PREFIX(bn_div_rem)
#define bn_div_dig 	RLC_PREFIX(bn_div_dig)
#define bn_div_rem_dig 	RLC_PREFIX(bn_div_rem_dig)
#define bn_mod_inv 	RLC_PREFIX(bn_mod_inv)
#define bn_mod_2b 	RLC_PREFIX(bn_mod_2b)
#define bn_mod_dig 	RLC_PREFIX(bn_mod_dig)
#define bn_mod_basic 	RLC_PREFIX(bn_mod_basic)
#define bn_mod_pre_barrt 	RLC_PREFIX(bn_mod_pre_barrt)
#define bn_mod_barrt 	RLC_PREFIX(bn_mod_barrt)
#define bn_mod_pre_monty 	RLC_PREFIX(bn_mod_pre_monty)
#define bn_mod_monty_conv 	RLC_PREFIX(bn_mod_monty_conv)
#define bn_mod_monty_back 	RLC_PREFIX(bn_mod_monty_back)
#define bn_mod_monty_basic 	RLC_PREFIX(bn_mod_monty_basic)
#define bn_mod_monty_comba 	RLC_PREFIX(bn_mod_monty_comba)
#define bn_mod_pre_pmers 	RLC_PREFIX(bn_mod_pre_pmers)
#define bn_mod_pmers 	RLC_PREFIX(bn_mod_pmers)
#define bn_mxp_basic 	RLC_PREFIX(bn_mxp_basic)
#define bn_mxp_slide 	RLC_PREFIX(bn_mxp_slide)
#define bn_mxp_monty 	RLC_PREFIX(bn_mxp_monty)
#define bn_mxp_dig 	RLC_PREFIX(bn_mxp_dig)
#define bn_srt 	RLC_PREFIX(bn_srt)
#define bn_gcd_basic 	RLC_PREFIX(bn_gcd_basic)
#define bn_gcd_lehme 	RLC_PREFIX(bn_gcd_lehme)
#define bn_gcd_stein 	RLC_PREFIX(bn_gcd_stein)
#define bn_gcd_dig 	RLC_PREFIX(bn_gcd_dig)
#define bn_gcd_ext_basic 	RLC_PREFIX(bn_gcd_ext_basic)
#define bn_gcd_ext_lehme 	RLC_PREFIX(bn_gcd_ext_lehme)
#define bn_gcd_ext_stein 	RLC_PREFIX(bn_gcd_ext_stein)
#define bn_gcd_ext_mid 	RLC_PREFIX(bn_gcd_ext_mid)
#define bn_gcd_ext_dig 	RLC_PREFIX(bn_gcd_ext_dig)
#define bn_lcm 	RLC_PREFIX(bn_lcm)
#define bn_smb_leg 	RLC_PREFIX(bn_smb_leg)
#define bn_smb_jac 	RLC_PREFIX(bn_smb_jac)
#define bn_get_prime 	RLC_PREFIX(bn_get_prime)
#define bn_is_prime 	RLC_PREFIX(bn_is_prime)
#define bn_is_prime_basic 	RLC_PREFIX(bn_is_prime_basic)
#define bn_is_prime_rabin 	RLC_PREFIX(bn_is_prime_rabin)
#define bn_is_prime_solov 	RLC_PREFIX(bn_is_prime_solov)
#define bn_gen_prime_basic 	RLC_PREFIX(bn_gen_prime_basic)
#define bn_gen_prime_safep 	RLC_PREFIX(bn_gen_prime_safep)
#define bn_gen_prime_stron 	RLC_PREFIX(bn_gen_prime_stron)
#define bn_factor 	RLC_PREFIX(bn_factor)
#define bn_is_factor 	RLC_PREFIX(bn_is_factor)
#define bn_rec_win 	RLC_PREFIX(bn_rec_win)
#define bn_rec_slw 	RLC_PREFIX(bn_rec_slw)
#define bn_rec_naf 	RLC_PREFIX(bn_rec_naf)
#define bn_rec_tnaf 	RLC_PREFIX(bn_rec_tnaf)
#define bn_rec_rtnaf 	RLC_PREFIX(bn_rec_rtnaf)
#define bn_rec_tnaf_get 	RLC_PREFIX(bn_rec_tnaf_get)
#define bn_rec_tnaf_mod 	RLC_PREFIX(bn_rec_tnaf_mod)
#define bn_rec_reg 	RLC_PREFIX(bn_rec_reg)
#define bn_rec_jsf 	RLC_PREFIX(bn_rec_jsf)
#define bn_rec_glv 	RLC_PREFIX(bn_rec_glv)

#undef bn_add1_low
#undef bn_addn_low
#undef bn_sub1_low
#undef bn_subn_low
#undef bn_cmp1_low
#undef bn_cmpn_low
#undef bn_lsh1_low
#undef bn_lshb_low
#undef bn_rsh1_low
#undef bn_rshb_low
#undef bn_mula_low
#undef bn_mul1_low
#undef bn_muln_low
#undef bn_muld_low
#undef bn_sqra_low
#undef bn_sqrn_low
#undef bn_divn_low
#undef bn_div1_low
#undef bn_modn_low

#define bn_add1_low 	RLC_PREFIX(bn_add1_low)
#define bn_addn_low 	RLC_PREFIX(bn_addn_low)
#define bn_sub1_low 	RLC_PREFIX(bn_sub1_low)
#define bn_subn_low 	RLC_PREFIX(bn_subn_low)
#define bn_cmp1_low 	RLC_PREFIX(bn_cmp1_low)
#define bn_cmpn_low 	RLC_PREFIX(bn_cmpn_low)
#define bn_lsh1_low 	RLC_PREFIX(bn_lsh1_low)
#define bn_lshb_low 	RLC_PREFIX(bn_lshb_low)
#define bn_rsh1_low 	RLC_PREFIX(bn_rsh1_low)
#define bn_rshb_low 	RLC_PREFIX(bn_rshb_low)
#define bn_mula_low 	RLC_PREFIX(bn_mula_low)
#define bn_mul1_low 	RLC_PREFIX(bn_mul1_low)
#define bn_muln_low 	RLC_PREFIX(bn_muln_low)
#define bn_muld_low 	RLC_PREFIX(bn_muld_low)
#define bn_sqra_low 	RLC_PREFIX(bn_sqra_low)
#define bn_sqrn_low 	RLC_PREFIX(bn_sqrn_low)
#define bn_divn_low 	RLC_PREFIX(bn_divn_low)
#define bn_div1_low 	RLC_PREFIX(bn_div1_low)
#define bn_modn_low 	RLC_PREFIX(bn_modn_low)

#undef fp_st
#undef fp_t
#define fp_st	        RLC_PREFIX(fp_st)
#define fp_t          RLC_PREFIX(fp_t)

#undef fp_prime_init
#undef fp_prime_clean
#undef fp_prime_get
#undef fp_prime_get_rdc
#undef fp_prime_get_conv
#undef fp_prime_get_mod8
#undef fp_prime_get_sps
#undef fp_prime_get_qnr
#undef fp_prime_get_cnr
#undef fp_prime_get_2ad
#undef fp_param_get
#undef fp_prime_set_dense
#undef fp_prime_set_pmers
#undef fp_prime_set_pairf
#undef fp_prime_calc
#undef fp_prime_conv
#undef fp_prime_conv_dig
#undef fp_prime_back
#undef fp_param_set
#undef fp_param_set_any
#undef fp_param_set_any_dense
#undef fp_param_set_any_pmers
#undef fp_param_set_any_tower
#undef fp_param_set_any_h2adc
#undef fp_param_print
#undef fp_prime_get_par
#undef fp_prime_get_par_sps
#undef fp_param_get_sps
#undef fp_copy
#undef fp_zero
#undef fp_is_zero
#undef fp_is_even
#undef fp_get_bit
#undef fp_set_bit
#undef fp_set_dig
#undef fp_bits
#undef fp_rand
#undef fp_print
#undef fp_size_str
#undef fp_read_str
#undef fp_write_str
#undef fp_read_bin
#undef fp_write_bin
#undef fp_cmp
#undef fp_cmp_dig
#undef fp_add_basic
#undef fp_add_integ
#undef fp_add_dig
#undef fp_sub_basic
#undef fp_sub_integ
#undef fp_sub_dig
#undef fp_neg_basic
#undef fp_neg_integ
#undef fp_dbl_basic
#undef fp_dbl_integ
#undef fp_hlv_basic
#undef fp_hlv_integ
#undef fp_mul_basic
#undef fp_mul_comba
#undef fp_mul_integ
#undef fp_mul_karat
#undef fp_mul_dig
#undef fp_sqr_basic
#undef fp_sqr_comba
#undef fp_sqr_integ
#undef fp_sqr_karat
#undef fp_lsh
#undef fp_rsh
#undef fp_rdc_basic
#undef fp_rdc_monty_basic
#undef fp_rdc_monty_comba
#undef fp_rdc_quick
#undef fp_inv_basic
#undef fp_inv_binar
#undef fp_inv_monty
#undef fp_inv_exgcd
#undef fp_inv_divst
#undef fp_inv_lower
#undef fp_inv_sim
#undef fp_exp_basic
#undef fp_exp_slide
#undef fp_exp_monty
#undef fp_srt

#define fp_prime_init 	RLC_PREFIX(fp_prime_init)
#define fp_prime_clean 	RLC_PREFIX(fp_prime_clean)
#define fp_prime_get 	RLC_PREFIX(fp_prime_get)
#define fp_prime_get_rdc 	RLC_PREFIX(fp_prime_get_rdc)
#define fp_prime_get_conv 	RLC_PREFIX(fp_prime_get_conv)
#define fp_prime_get_mod8 	RLC_PREFIX(fp_prime_get_mod8)
#define fp_prime_get_sps 	RLC_PREFIX(fp_prime_get_sps)
#define fp_prime_get_qnr 	RLC_PREFIX(fp_prime_get_qnr)
#define fp_prime_get_cnr 	RLC_PREFIX(fp_prime_get_cnr)
#define fp_prime_get_2ad 	RLC_PREFIX(fp_prime_get_2ad)
#define fp_param_get 	RLC_PREFIX(fp_param_get)
#define fp_prime_set_dense 	RLC_PREFIX(fp_prime_set_dense)
#define fp_prime_set_pmers 	RLC_PREFIX(fp_prime_set_pmers)
#define fp_prime_set_pairf 	RLC_PREFIX(fp_prime_set_pairf)
#define fp_prime_calc 	RLC_PREFIX(fp_prime_calc)
#define fp_prime_conv 	RLC_PREFIX(fp_prime_conv)
#define fp_prime_conv_dig 	RLC_PREFIX(fp_prime_conv_dig)
#define fp_prime_back 	RLC_PREFIX(fp_prime_back)
#define fp_param_set 	RLC_PREFIX(fp_param_set)
#define fp_param_set_any 	RLC_PREFIX(fp_param_set_any)
#define fp_param_set_any_dense 	RLC_PREFIX(fp_param_set_any_dense)
#define fp_param_set_any_pmers 	RLC_PREFIX(fp_param_set_any_pmers)
#define fp_param_set_any_tower 	RLC_PREFIX(fp_param_set_any_tower)
#define fp_param_set_any_h2adc 	RLC_PREFIX(fp_param_set_any_h2adc)
#define fp_param_print 	RLC_PREFIX(fp_param_print)
#define fp_prime_get_par 	RLC_PREFIX(fp_prime_get_par)
#define fp_prime_get_par_sps 	RLC_PREFIX(fp_prime_get_par_sps)
#define fp_param_get_sps 	RLC_PREFIX(fp_param_get_sps)
#define fp_copy 	RLC_PREFIX(fp_copy)
#define fp_zero 	RLC_PREFIX(fp_zero)
#define fp_is_zero 	RLC_PREFIX(fp_is_zero)
#define fp_is_even 	RLC_PREFIX(fp_is_even)
#define fp_get_bit 	RLC_PREFIX(fp_get_bit)
#define fp_set_bit 	RLC_PREFIX(fp_set_bit)
#define fp_set_dig 	RLC_PREFIX(fp_set_dig)
#define fp_bits 	RLC_PREFIX(fp_bits)
#define fp_rand 	RLC_PREFIX(fp_rand)
#define fp_print 	RLC_PREFIX(fp_print)
#define fp_size_str 	RLC_PREFIX(fp_size_str)
#define fp_read_str 	RLC_PREFIX(fp_read_str)
#define fp_write_str 	RLC_PREFIX(fp_write_str)
#define fp_read_bin 	RLC_PREFIX(fp_read_bin)
#define fp_write_bin 	RLC_PREFIX(fp_write_bin)
#define fp_cmp 	RLC_PREFIX(fp_cmp)
#define fp_cmp_dig 	RLC_PREFIX(fp_cmp_dig)
#define fp_add_basic 	RLC_PREFIX(fp_add_basic)
#define fp_add_integ 	RLC_PREFIX(fp_add_integ)
#define fp_add_dig 	RLC_PREFIX(fp_add_dig)
#define fp_sub_basic 	RLC_PREFIX(fp_sub_basic)
#define fp_sub_integ 	RLC_PREFIX(fp_sub_integ)
#define fp_sub_dig 	RLC_PREFIX(fp_sub_dig)
#define fp_neg_basic 	RLC_PREFIX(fp_neg_basic)
#define fp_neg_integ 	RLC_PREFIX(fp_neg_integ)
#define fp_dbl_basic 	RLC_PREFIX(fp_dbl_basic)
#define fp_dbl_integ 	RLC_PREFIX(fp_dbl_integ)
#define fp_hlv_basic 	RLC_PREFIX(fp_hlv_basic)
#define fp_hlv_integ 	RLC_PREFIX(fp_hlv_integ)
#define fp_mul_basic 	RLC_PREFIX(fp_mul_basic)
#define fp_mul_comba 	RLC_PREFIX(fp_mul_comba)
#define fp_mul_integ 	RLC_PREFIX(fp_mul_integ)
#define fp_mul_karat 	RLC_PREFIX(fp_mul_karat)
#define fp_mul_dig 	RLC_PREFIX(fp_mul_dig)
#define fp_sqr_basic 	RLC_PREFIX(fp_sqr_basic)
#define fp_sqr_comba 	RLC_PREFIX(fp_sqr_comba)
#define fp_sqr_integ 	RLC_PREFIX(fp_sqr_integ)
#define fp_sqr_karat 	RLC_PREFIX(fp_sqr_karat)
#define fp_lsh 	RLC_PREFIX(fp_lsh)
#define fp_rsh 	RLC_PREFIX(fp_rsh)
#define fp_rdc_basic 	RLC_PREFIX(fp_rdc_basic)
#define fp_rdc_monty_basic 	RLC_PREFIX(fp_rdc_monty_basic)
#define fp_rdc_monty_comba 	RLC_PREFIX(fp_rdc_monty_comba)
#define fp_rdc_quick 	RLC_PREFIX(fp_rdc_quick)
#define fp_inv_basic 	RLC_PREFIX(fp_inv_basic)
#define fp_inv_binar 	RLC_PREFIX(fp_inv_binar)
#define fp_inv_monty 	RLC_PREFIX(fp_inv_monty)
#define fp_inv_exgcd 	RLC_PREFIX(fp_inv_exgcd)
#define fp_inv_divst 	RLC_PREFIX(fp_inv_divst)
#define fp_inv_lower 	RLC_PREFIX(fp_inv_lower)
#define fp_inv_sim 	RLC_PREFIX(fp_inv_sim)
#define fp_exp_basic 	RLC_PREFIX(fp_exp_basic)
#define fp_exp_slide 	RLC_PREFIX(fp_exp_slide)
#define fp_exp_monty 	RLC_PREFIX(fp_exp_monty)
#define fp_srt 	RLC_PREFIX(fp_srt)

#undef fp_add1_low
#undef fp_addn_low
#undef fp_addm_low
#undef fp_addd_low
#undef fp_addc_low
#undef fp_sub1_low
#undef fp_subn_low
#undef fp_subm_low
#undef fp_subd_low
#undef fp_subc_low
#undef fp_negm_low
#undef fp_dbln_low
#undef fp_dblm_low
#undef fp_hlvm_low
#undef fp_hlvd_low
#undef fp_lsh1_low
#undef fp_lshb_low
#undef fp_rsh1_low
#undef fp_rshb_low
#undef fp_mula_low
#undef fp_mul1_low
#undef fp_muln_low
#undef fp_mulm_low
#undef fp_sqrn_low
#undef fp_sqrm_low
#undef fp_rdcs_low
#undef fp_rdcn_low
#undef fp_invm_low

#define fp_add1_low 	RLC_PREFIX(fp_add1_low)
#define fp_addn_low 	RLC_PREFIX(fp_addn_low)
#define fp_addm_low 	RLC_PREFIX(fp_addm_low)
#define fp_addd_low 	RLC_PREFIX(fp_addd_low)
#define fp_addc_low 	RLC_PREFIX(fp_addc_low)
#define fp_sub1_low 	RLC_PREFIX(fp_sub1_low)
#define fp_subn_low 	RLC_PREFIX(fp_subn_low)
#define fp_subm_low 	RLC_PREFIX(fp_subm_low)
#define fp_subd_low 	RLC_PREFIX(fp_subd_low)
#define fp_subc_low 	RLC_PREFIX(fp_subc_low)
#define fp_negm_low 	RLC_PREFIX(fp_negm_low)
#define fp_dbln_low 	RLC_PREFIX(fp_dbln_low)
#define fp_dblm_low 	RLC_PREFIX(fp_dblm_low)
#define fp_hlvm_low 	RLC_PREFIX(fp_hlvm_low)
#define fp_hlvd_low 	RLC_PREFIX(fp_hlvd_low)
#define fp_lsh1_low 	RLC_PREFIX(fp_lsh1_low)
#define fp_lshb_low 	RLC_PREFIX(fp_lshb_low)
#define fp_rsh1_low 	RLC_PREFIX(fp_rsh1_low)
#define fp_rshb_low 	RLC_PREFIX(fp_rshb_low)
#define fp_mula_low 	RLC_PREFIX(fp_mula_low)
#define fp_mul1_low 	RLC_PREFIX(fp_mul1_low)
#define fp_muln_low 	RLC_PREFIX(fp_muln_low)
#define fp_mulm_low 	RLC_PREFIX(fp_mulm_low)
#define fp_sqrn_low 	RLC_PREFIX(fp_sqrn_low)
#define fp_sqrm_low 	RLC_PREFIX(fp_sqrm_low)
#define fp_rdcs_low 	RLC_PREFIX(fp_rdcs_low)
#define fp_rdcn_low 	RLC_PREFIX(fp_rdcn_low)
#define fp_invm_low 	RLC_PREFIX(fp_invm_low)

#undef fp_st
#undef fp_t
#define fp_st	        RLC_PREFIX(fp_st)
#define fp_t          RLC_PREFIX(fp_t)

#undef fb_poly_init
#undef fb_poly_clean
#undef fb_poly_get
#undef fb_poly_set_dense
#undef fb_poly_set_trino
#undef fb_poly_set_penta
#undef fb_poly_get_srz
#undef fb_poly_tab_srz
#undef fb_poly_tab_sqr
#undef fb_poly_get_chain
#undef fb_poly_get_rdc
#undef fb_poly_get_trc
#undef fb_poly_get_slv
#undef fb_param_set
#undef fb_param_set_any
#undef fb_param_print
#undef fb_poly_add
#undef fb_copy
#undef fb_neg
#undef fb_zero
#undef fb_is_zero
#undef fb_get_bit
#undef fb_set_bit
#undef fb_set_dig
#undef fb_bits
#undef fb_rand
#undef fb_print
#undef fb_size_str
#undef fb_read_str
#undef fb_write_str
#undef fb_read_bin
#undef fb_write_bin
#undef fb_cmp
#undef fb_cmp_dig
#undef fb_add
#undef fb_add_dig
#undef fb_mul_basic
#undef fb_mul_integ
#undef fb_mul_lodah
#undef fb_mul_dig
#undef fb_mul_karat
#undef fb_sqr_basic
#undef fb_sqr_integ
#undef fb_sqr_quick
#undef fb_lsh
#undef fb_rsh
#undef fb_rdc_basic
#undef fb_rdc_quick
#undef fb_srt_basic
#undef fb_srt_quick
#undef fb_trc_basic
#undef fb_trc_quick
#undef fb_inv_basic
#undef fb_inv_binar
#undef fb_inv_exgcd
#undef fb_inv_almos
#undef fb_inv_itoht
#undef fb_inv_bruch
#undef fb_inv_ctaia
#undef fb_inv_lower
#undef fb_inv_sim
#undef fb_exp_2b
#undef fb_exp_basic
#undef fb_exp_slide
#undef fb_exp_monty
#undef fb_slv_basic
#undef fb_slv_quick
#undef fb_itr_basic
#undef fb_itr_pre_quick
#undef fb_itr_quick

#define fb_poly_init 	RLC_PREFIX(fb_poly_init)
#define fb_poly_clean 	RLC_PREFIX(fb_poly_clean)
#define fb_poly_get 	RLC_PREFIX(fb_poly_get)
#define fb_poly_set_dense 	RLC_PREFIX(fb_poly_set_dense)
#define fb_poly_set_trino 	RLC_PREFIX(fb_poly_set_trino)
#define fb_poly_set_penta 	RLC_PREFIX(fb_poly_set_penta)
#define fb_poly_get_srz 	RLC_PREFIX(fb_poly_get_srz)
#define fb_poly_tab_srz 	RLC_PREFIX(fb_poly_tab_srz)
#define fb_poly_tab_sqr 	RLC_PREFIX(fb_poly_tab_sqr)
#define fb_poly_get_chain 	RLC_PREFIX(fb_poly_get_chain)
#define fb_poly_get_rdc 	RLC_PREFIX(fb_poly_get_rdc)
#define fb_poly_get_trc 	RLC_PREFIX(fb_poly_get_trc)
#define fb_poly_get_slv 	RLC_PREFIX(fb_poly_get_slv)
#define fb_param_set 	RLC_PREFIX(fb_param_set)
#define fb_param_set_any 	RLC_PREFIX(fb_param_set_any)
#define fb_param_print 	RLC_PREFIX(fb_param_print)
#define fb_poly_add 	RLC_PREFIX(fb_poly_add)
#define fb_copy 	RLC_PREFIX(fb_copy)
#define fb_neg 	RLC_PREFIX(fb_neg)
#define fb_zero 	RLC_PREFIX(fb_zero)
#define fb_is_zero 	RLC_PREFIX(fb_is_zero)
#define fb_get_bit 	RLC_PREFIX(fb_get_bit)
#define fb_set_bit 	RLC_PREFIX(fb_set_bit)
#define fb_set_dig 	RLC_PREFIX(fb_set_dig)
#define fb_bits 	RLC_PREFIX(fb_bits)
#define fb_rand 	RLC_PREFIX(fb_rand)
#define fb_print 	RLC_PREFIX(fb_print)
#define fb_size_str 	RLC_PREFIX(fb_size_str)
#define fb_read_str 	RLC_PREFIX(fb_read_str)
#define fb_write_str 	RLC_PREFIX(fb_write_str)
#define fb_read_bin 	RLC_PREFIX(fb_read_bin)
#define fb_write_bin 	RLC_PREFIX(fb_write_bin)
#define fb_cmp 	RLC_PREFIX(fb_cmp)
#define fb_cmp_dig 	RLC_PREFIX(fb_cmp_dig)
#define fb_add 	RLC_PREFIX(fb_add)
#define fb_add_dig 	RLC_PREFIX(fb_add_dig)
#define fb_mul_basic 	RLC_PREFIX(fb_mul_basic)
#define fb_mul_integ 	RLC_PREFIX(fb_mul_integ)
#define fb_mul_lodah 	RLC_PREFIX(fb_mul_lodah)
#define fb_mul_dig 	RLC_PREFIX(fb_mul_dig)
#define fb_mul_karat 	RLC_PREFIX(fb_mul_karat)
#define fb_sqr_basic 	RLC_PREFIX(fb_sqr_basic)
#define fb_sqr_integ 	RLC_PREFIX(fb_sqr_integ)
#define fb_sqr_quick 	RLC_PREFIX(fb_sqr_quick)
#define fb_lsh 	RLC_PREFIX(fb_lsh)
#define fb_rsh 	RLC_PREFIX(fb_rsh)
#define fb_rdc_basic 	RLC_PREFIX(fb_rdc_basic)
#define fb_rdc_quick 	RLC_PREFIX(fb_rdc_quick)
#define fb_srt_basic 	RLC_PREFIX(fb_srt_basic)
#define fb_srt_quick 	RLC_PREFIX(fb_srt_quick)
#define fb_trc_basic 	RLC_PREFIX(fb_trc_basic)
#define fb_trc_quick 	RLC_PREFIX(fb_trc_quick)
#define fb_inv_basic 	RLC_PREFIX(fb_inv_basic)
#define fb_inv_binar 	RLC_PREFIX(fb_inv_binar)
#define fb_inv_exgcd 	RLC_PREFIX(fb_inv_exgcd)
#define fb_inv_almos 	RLC_PREFIX(fb_inv_almos)
#define fb_inv_itoht 	RLC_PREFIX(fb_inv_itoht)
#define fb_inv_bruch 	RLC_PREFIX(fb_inv_bruch)
#define fb_inv_ctaia 	RLC_PREFIX(fb_inv_ctaia)
#define fb_inv_lower 	RLC_PREFIX(fb_inv_lower)
#define fb_inv_sim 	RLC_PREFIX(fb_inv_sim)
#define fb_exp_2b 	RLC_PREFIX(fb_exp_2b)
#define fb_exp_basic 	RLC_PREFIX(fb_exp_basic)
#define fb_exp_slide 	RLC_PREFIX(fb_exp_slide)
#define fb_exp_monty 	RLC_PREFIX(fb_exp_monty)
#define fb_slv_basic 	RLC_PREFIX(fb_slv_basic)
#define fb_slv_quick 	RLC_PREFIX(fb_slv_quick)
#define fb_itr_basic 	RLC_PREFIX(fb_itr_basic)
#define fb_itr_pre_quick 	RLC_PREFIX(fb_itr_pre_quick)
#define fb_itr_quick 	RLC_PREFIX(fb_itr_quick)

#undef fb_add1_low
#undef fb_addn_low
#undef fb_addd_low
#undef fb_lsh1_low
#undef fb_lshb_low
#undef fb_rsh1_low
#undef fb_rshb_low
#undef fb_lsha_low
#undef fb_mul1_low
#undef fb_muln_low
#undef fb_muld_low
#undef fb_mulm_low
#undef fb_sqrn_low
#undef fb_sqrl_low
#undef fb_sqrm_low
#undef fb_itrn_low
#undef fb_srtn_low
#undef fb_slvn_low
#undef fb_trcn_low
#undef fb_rdcn_low
#undef fb_rdc1_low
#undef fb_invn_low

#define fb_add1_low 	RLC_PREFIX(fb_add1_low)
#define fb_addn_low 	RLC_PREFIX(fb_addn_low)
#define fb_addd_low 	RLC_PREFIX(fb_addd_low)
#define fb_lsh1_low 	RLC_PREFIX(fb_lsh1_low)
#define fb_lshb_low 	RLC_PREFIX(fb_lshb_low)
#define fb_rsh1_low 	RLC_PREFIX(fb_rsh1_low)
#define fb_rshb_low 	RLC_PREFIX(fb_rshb_low)
#define fb_lsha_low 	RLC_PREFIX(fb_lsha_low)
#define fb_mul1_low 	RLC_PREFIX(fb_mul1_low)
#define fb_muln_low 	RLC_PREFIX(fb_muln_low)
#define fb_muld_low 	RLC_PREFIX(fb_muld_low)
#define fb_mulm_low 	RLC_PREFIX(fb_mulm_low)
#define fb_sqrn_low 	RLC_PREFIX(fb_sqrn_low)
#define fb_sqrl_low 	RLC_PREFIX(fb_sqrl_low)
#define fb_sqrm_low 	RLC_PREFIX(fb_sqrm_low)
#define fb_itrn_low 	RLC_PREFIX(fb_itrn_low)
#define fb_srtn_low 	RLC_PREFIX(fb_srtn_low)
#define fb_slvn_low 	RLC_PREFIX(fb_slvn_low)
#define fb_trcn_low 	RLC_PREFIX(fb_trcn_low)
#define fb_rdcn_low 	RLC_PREFIX(fb_rdcn_low)
#define fb_rdc1_low 	RLC_PREFIX(fb_rdc1_low)
#define fb_invn_low 	RLC_PREFIX(fb_invn_low)

#undef ep_st
#undef ep_t
#define ep_st         RLC_PREFIX(ep_st)
#define ep_t          RLC_PREFIX(ep_t)

#undef ep_curve_init
#undef ep_curve_clean
#undef ep_curve_get_a
#undef ep_curve_get_b
#undef ep_curve_get_b3
#undef ep_curve_get_beta
#undef ep_curve_get_v1
#undef ep_curve_get_v2
#undef ep_curve_opt_a
#undef ep_curve_opt_b
#undef ep_curve_opt_b3
#undef ep_curve_mul_a
#undef ep_curve_mul_b
#undef ep_curve_mul_b3
#undef ep_curve_is_endom
#undef ep_curve_is_super
#undef ep_curve_is_pairf
#undef ep_curve_is_ctmap
#undef ep_curve_get_gen
#undef ep_curve_get_tab
#undef ep_curve_get_ord
#undef ep_curve_get_cof
#undef ep_curve_get_iso
#undef ep_curve_set_plain
#undef ep_curve_set_super
#undef ep_curve_set_endom
#undef ep_param_set
#undef ep_param_set_any
#undef ep_param_set_any_plain
#undef ep_param_set_any_endom
#undef ep_param_set_any_super
#undef ep_param_set_any_pairf
#undef ep_param_get
#undef ep_param_print
#undef ep_param_level
#undef ep_param_embed
#undef ep_is_infty
#undef ep_set_infty
#undef ep_copy
#undef ep_cmp
#undef ep_rand
#undef ep_blind
#undef ep_rhs
#undef ep_on_curve
#undef ep_tab
#undef ep_print
#undef ep_size_bin
#undef ep_read_bin
#undef ep_write_bin
#undef ep_neg
#undef ep_add_basic
#undef ep_add_slp_basic
#undef ep_add_projc
#undef ep_add_jacob
#undef ep_sub
#undef ep_dbl_basic
#undef ep_dbl_slp_basic
#undef ep_dbl_projc
#undef ep_dbl_jacob
#undef ep_psi
#undef ep_mul_basic
#undef ep_mul_slide
#undef ep_mul_monty
#undef ep_mul_lwnaf
#undef ep_mul_lwreg
#undef ep_mul_gen
#undef ep_mul_dig
#undef ep_mul_pre_basic
#undef ep_mul_pre_yaowi
#undef ep_mul_pre_nafwi
#undef ep_mul_pre_combs
#undef ep_mul_pre_combd
#undef ep_mul_pre_lwnaf
#undef ep_mul_fix_basic
#undef ep_mul_fix_yaowi
#undef ep_mul_fix_nafwi
#undef ep_mul_fix_combs
#undef ep_mul_fix_combd
#undef ep_mul_fix_lwnaf
#undef ep_mul_sim_basic
#undef ep_mul_sim_trick
#undef ep_mul_sim_inter
#undef ep_mul_sim_joint
#undef ep_mul_sim_lot
#undef ep_mul_sim_gen
#undef ep_mul_sim_dig
#undef ep_norm
#undef ep_norm_sim
#undef ep_map
#undef ep_map_dst
#undef ep_map_dst
#undef ep_pck
#undef ep_upk

  multiplication function, #define ep_mul_basic 	RLC_PREFIX(ep_mul_basic)
#define ep_curve_init 	RLC_PREFIX(ep_curve_init)
#define ep_curve_clean 	RLC_PREFIX(ep_curve_clean)
#define ep_curve_get_a 	RLC_PREFIX(ep_curve_get_a)
#define ep_curve_get_b 	RLC_PREFIX(ep_curve_get_b)
#define ep_curve_get_b3 	RLC_PREFIX(ep_curve_get_b3)
#define ep_curve_get_beta 	RLC_PREFIX(ep_curve_get_beta)
#define ep_curve_get_v1 	RLC_PREFIX(ep_curve_get_v1)
#define ep_curve_get_v2 	RLC_PREFIX(ep_curve_get_v2)
#define ep_curve_opt_a 	RLC_PREFIX(ep_curve_opt_a)
#define ep_curve_opt_b 	RLC_PREFIX(ep_curve_opt_b)
#define ep_curve_opt_b3 	RLC_PREFIX(ep_curve_opt_b3)
#define ep_curve_mul_a 	RLC_PREFIX(ep_curve_mul_a)
#define ep_curve_mul_b 	RLC_PREFIX(ep_curve_mul_b)
#define ep_curve_mul_b3 	RLC_PREFIX(ep_curve_mul_b3)
#define ep_curve_is_endom 	RLC_PREFIX(ep_curve_is_endom)
#define ep_curve_is_super 	RLC_PREFIX(ep_curve_is_super)
#define ep_curve_is_pairf 	RLC_PREFIX(ep_curve_is_pairf)
#define ep_curve_is_ctmap 	RLC_PREFIX(ep_curve_is_ctmap)
#define ep_curve_get_gen 	RLC_PREFIX(ep_curve_get_gen)
#define ep_curve_get_tab 	RLC_PREFIX(ep_curve_get_tab)
#define ep_curve_get_ord 	RLC_PREFIX(ep_curve_get_ord)
#define ep_curve_get_cof 	RLC_PREFIX(ep_curve_get_cof)
#define ep_curve_get_iso 	RLC_PREFIX(ep_curve_get_iso)
#define ep_curve_set_plain 	RLC_PREFIX(ep_curve_set_plain)
#define ep_curve_set_super 	RLC_PREFIX(ep_curve_set_super)
#define ep_curve_set_endom 	RLC_PREFIX(ep_curve_set_endom)
#define ep_param_set 	RLC_PREFIX(ep_param_set)
#define ep_param_set_any 	RLC_PREFIX(ep_param_set_any)
#define ep_param_set_any_plain 	RLC_PREFIX(ep_param_set_any_plain)
#define ep_param_set_any_endom 	RLC_PREFIX(ep_param_set_any_endom)
#define ep_param_set_any_super 	RLC_PREFIX(ep_param_set_any_super)
#define ep_param_set_any_pairf 	RLC_PREFIX(ep_param_set_any_pairf)
#define ep_param_get 	RLC_PREFIX(ep_param_get)
#define ep_param_print 	RLC_PREFIX(ep_param_print)
#define ep_param_level 	RLC_PREFIX(ep_param_level)
#define ep_param_embed 	RLC_PREFIX(ep_param_embed)
#define ep_is_infty 	RLC_PREFIX(ep_is_infty)
#define ep_set_infty 	RLC_PREFIX(ep_set_infty)
#define ep_copy 	RLC_PREFIX(ep_copy)
#define ep_cmp 	RLC_PREFIX(ep_cmp)
#define ep_rand 	RLC_PREFIX(ep_rand)
#define ep_blind 	RLC_PREFIX(ep_blind)
#define ep_rhs 	RLC_PREFIX(ep_rhs)
#define ep_on_curve 	RLC_PREFIX(ep_on_curve)
#define ep_tab 	RLC_PREFIX(ep_tab)
#define ep_print 	RLC_PREFIX(ep_print)
#define ep_size_bin 	RLC_PREFIX(ep_size_bin)
#define ep_read_bin 	RLC_PREFIX(ep_read_bin)
#define ep_write_bin 	RLC_PREFIX(ep_write_bin)
#define ep_neg 	RLC_PREFIX(ep_neg)
#define ep_add_basic 	RLC_PREFIX(ep_add_basic)
#define ep_add_slp_basic 	RLC_PREFIX(ep_add_slp_basic)
#define ep_add_projc 	RLC_PREFIX(ep_add_projc)
#define ep_add_jacob 	RLC_PREFIX(ep_add_jacob)
#define ep_sub 	RLC_PREFIX(ep_sub)
#define ep_dbl_basic 	RLC_PREFIX(ep_dbl_basic)
#define ep_dbl_slp_basic 	RLC_PREFIX(ep_dbl_slp_basic)
#define ep_dbl_projc 	RLC_PREFIX(ep_dbl_projc)
#define ep_dbl_jacob 	RLC_PREFIX(ep_dbl_jacob)
#define ep_psi 	RLC_PREFIX(ep_psi)
#define ep_mul_basic 	RLC_PREFIX(ep_mul_basic)
#define ep_mul_slide 	RLC_PREFIX(ep_mul_slide)
#define ep_mul_monty 	RLC_PREFIX(ep_mul_monty)
#define ep_mul_lwnaf 	RLC_PREFIX(ep_mul_lwnaf)
#define ep_mul_lwreg 	RLC_PREFIX(ep_mul_lwreg)
#define ep_mul_gen 	RLC_PREFIX(ep_mul_gen)
#define ep_mul_dig 	RLC_PREFIX(ep_mul_dig)
#define ep_mul_pre_basic 	RLC_PREFIX(ep_mul_pre_basic)
#define ep_mul_pre_yaowi 	RLC_PREFIX(ep_mul_pre_yaowi)
#define ep_mul_pre_nafwi 	RLC_PREFIX(ep_mul_pre_nafwi)
#define ep_mul_pre_combs 	RLC_PREFIX(ep_mul_pre_combs)
#define ep_mul_pre_combd 	RLC_PREFIX(ep_mul_pre_combd)
#define ep_mul_pre_lwnaf 	RLC_PREFIX(ep_mul_pre_lwnaf)
#define ep_mul_fix_basic 	RLC_PREFIX(ep_mul_fix_basic)
#define ep_mul_fix_yaowi 	RLC_PREFIX(ep_mul_fix_yaowi)
#define ep_mul_fix_nafwi 	RLC_PREFIX(ep_mul_fix_nafwi)
#define ep_mul_fix_combs 	RLC_PREFIX(ep_mul_fix_combs)
#define ep_mul_fix_combd 	RLC_PREFIX(ep_mul_fix_combd)
#define ep_mul_fix_lwnaf 	RLC_PREFIX(ep_mul_fix_lwnaf)
#define ep_mul_sim_basic 	RLC_PREFIX(ep_mul_sim_basic)
#define ep_mul_sim_trick 	RLC_PREFIX(ep_mul_sim_trick)
#define ep_mul_sim_inter 	RLC_PREFIX(ep_mul_sim_inter)
#define ep_mul_sim_joint 	RLC_PREFIX(ep_mul_sim_joint)
#define ep_mul_sim_lot 	RLC_PREFIX(ep_mul_sim_lot)
#define ep_mul_sim_gen 	RLC_PREFIX(ep_mul_sim_gen)
#define ep_mul_sim_dig 	RLC_PREFIX(ep_mul_sim_dig)
#define ep_norm 	RLC_PREFIX(ep_norm)
#define ep_norm_sim 	RLC_PREFIX(ep_norm_sim)
#define ep_map 	RLC_PREFIX(ep_map)
#define ep_map_dst 	RLC_PREFIX(ep_map_dst)
#define ep_map_dst 	RLC_PREFIX(ep_map_dst)
#define ep_pck 	RLC_PREFIX(ep_pck)
#define ep_upk 	RLC_PREFIX(ep_upk)

#undef ed_st
#undef ed_t
#define ed_st         RLC_PREFIX(ed_st)
#define ed_t          RLC_PREFIX(ed_t)

#undef ed_param_set
#undef ed_param_set_any
#undef ed_param_get
#undef ed_curve_get_ord
#undef ed_curve_get_gen
#undef ed_curve_get_tab
#undef ed_curve_get_cof
#undef ed_param_print
#undef ed_param_level
#undef ed_projc_to_extnd
#undef ed_rand
#undef ed_blind
#undef ed_rhs
#undef ed_copy
#undef ed_cmp
#undef ed_set_infty
#undef ed_is_infty
#undef ed_neg_basic
#undef ed_neg_projc
#undef ed_add_basic
#undef ed_add_projc
#undef ed_add_extnd
#undef ed_sub_basic
#undef ed_sub_projc
#undef ed_sub_extnd
#undef ed_dbl_basic
#undef ed_dbl_projc
#undef ed_dbl_extnd
#undef ed_norm
#undef ed_norm_sim
#undef ed_map
#undef ed_map_dst
#undef ed_curve_init
#undef ed_curve_clean
#undef ed_mul_pre_basic
#undef ed_mul_pre_yaowi
#undef ed_mul_pre_nafwi
#undef ed_mul_pre_combs
#undef ed_mul_pre_combd
#undef ed_mul_pre_lwnaf
#undef ed_mul_fix_basic
#undef ed_mul_fix_yaowi
#undef ed_mul_fix_nafwi
#undef ed_mul_fix_combs
#undef ed_mul_fix_combd
#undef ed_mul_fix_lwnaf
#undef ed_mul_fix_lwnaf_mixed
#undef ed_mul_gen
#undef ed_mul_dig
#undef ed_mul_sim_basic
#undef ed_mul_sim_trick
#undef ed_mul_sim_inter
#undef ed_mul_sim_joint
#undef ed_mul_sim_gen
#undef ed_tab
#undef ed_print
#undef ed_on_curve
#undef ed_size_bin
#undef ed_read_bin
#undef ed_write_bin
#undef ed_mul_basic
#undef ed_mul_slide
#undef ed_mul_monty
#undef ed_mul_lwnaf
#undef ed_mul_lwreg
#undef ed_pck
#undef ed_upk

#define ed_param_set 	RLC_PREFIX(ed_param_set)
#define ed_param_set_any 	RLC_PREFIX(ed_param_set_any)
#define ed_param_get 	RLC_PREFIX(ed_param_get)
#define ed_curve_get_ord 	RLC_PREFIX(ed_curve_get_ord)
#define ed_curve_get_gen 	RLC_PREFIX(ed_curve_get_gen)
#define ed_curve_get_tab 	RLC_PREFIX(ed_curve_get_tab)
#define ed_curve_get_cof 	RLC_PREFIX(ed_curve_get_cof)
#define ed_param_print 	RLC_PREFIX(ed_param_print)
#define ed_param_level 	RLC_PREFIX(ed_param_level)
#define ed_projc_to_extnd 	RLC_PREFIX(ed_projc_to_extnd)
#define ed_rand 	RLC_PREFIX(ed_rand)
#define ed_blind 	RLC_PREFIX(ed_blind)
#define ed_rhs 	RLC_PREFIX(ed_rhs)
#define ed_copy 	RLC_PREFIX(ed_copy)
#define ed_cmp 	RLC_PREFIX(ed_cmp)
#define ed_set_infty 	RLC_PREFIX(ed_set_infty)
#define ed_is_infty 	RLC_PREFIX(ed_is_infty)
#define ed_neg_basic 	RLC_PREFIX(ed_neg_basic)
#define ed_neg_projc 	RLC_PREFIX(ed_neg_projc)
#define ed_add_basic 	RLC_PREFIX(ed_add_basic)
#define ed_add_projc 	RLC_PREFIX(ed_add_projc)
#define ed_add_extnd 	RLC_PREFIX(ed_add_extnd)
#define ed_sub_basic 	RLC_PREFIX(ed_sub_basic)
#define ed_sub_projc 	RLC_PREFIX(ed_sub_projc)
#define ed_sub_extnd 	RLC_PREFIX(ed_sub_extnd)
#define ed_dbl_basic 	RLC_PREFIX(ed_dbl_basic)
#define ed_dbl_projc 	RLC_PREFIX(ed_dbl_projc)
#define ed_dbl_extnd 	RLC_PREFIX(ed_dbl_extnd)
#define ed_norm 	RLC_PREFIX(ed_norm)
#define ed_norm_sim 	RLC_PREFIX(ed_norm_sim)
#define ed_map 	RLC_PREFIX(ed_map)
#define ed_map_dst 	RLC_PREFIX(ed_map_dst)
#define ed_curve_init 	RLC_PREFIX(ed_curve_init)
#define ed_curve_clean 	RLC_PREFIX(ed_curve_clean)
#define ed_mul_pre_basic 	RLC_PREFIX(ed_mul_pre_basic)
#define ed_mul_pre_yaowi 	RLC_PREFIX(ed_mul_pre_yaowi)
#define ed_mul_pre_nafwi 	RLC_PREFIX(ed_mul_pre_nafwi)
#define ed_mul_pre_combs 	RLC_PREFIX(ed_mul_pre_combs)
#define ed_mul_pre_combd 	RLC_PREFIX(ed_mul_pre_combd)
#define ed_mul_pre_lwnaf 	RLC_PREFIX(ed_mul_pre_lwnaf)
#define ed_mul_fix_basic 	RLC_PREFIX(ed_mul_fix_basic)
#define ed_mul_fix_yaowi 	RLC_PREFIX(ed_mul_fix_yaowi)
#define ed_mul_fix_nafwi 	RLC_PREFIX(ed_mul_fix_nafwi)
#define ed_mul_fix_combs 	RLC_PREFIX(ed_mul_fix_combs)
#define ed_mul_fix_combd 	RLC_PREFIX(ed_mul_fix_combd)
#define ed_mul_fix_lwnaf 	RLC_PREFIX(ed_mul_fix_lwnaf)
#define ed_mul_fix_lwnaf_mixed 	RLC_PREFIX(ed_mul_fix_lwnaf_mixed)
#define ed_mul_gen 	RLC_PREFIX(ed_mul_gen)
#define ed_mul_dig 	RLC_PREFIX(ed_mul_dig)
#define ed_mul_sim_basic 	RLC_PREFIX(ed_mul_sim_basic)
#define ed_mul_sim_trick 	RLC_PREFIX(ed_mul_sim_trick)
#define ed_mul_sim_inter 	RLC_PREFIX(ed_mul_sim_inter)
#define ed_mul_sim_joint 	RLC_PREFIX(ed_mul_sim_joint)
#define ed_mul_sim_gen 	RLC_PREFIX(ed_mul_sim_gen)
#define ed_tab 	RLC_PREFIX(ed_tab)
#define ed_print 	RLC_PREFIX(ed_print)
#define ed_on_curve 	RLC_PREFIX(ed_on_curve)
#define ed_size_bin 	RLC_PREFIX(ed_size_bin)
#define ed_read_bin 	RLC_PREFIX(ed_read_bin)
#define ed_write_bin 	RLC_PREFIX(ed_write_bin)
#define ed_mul_basic 	RLC_PREFIX(ed_mul_basic)
#define ed_mul_slide 	RLC_PREFIX(ed_mul_slide)
#define ed_mul_monty 	RLC_PREFIX(ed_mul_monty)
#define ed_mul_lwnaf 	RLC_PREFIX(ed_mul_lwnaf)
#define ed_mul_lwreg 	RLC_PREFIX(ed_mul_lwreg)
#define ed_pck 	RLC_PREFIX(ed_pck)
#define ed_upk 	RLC_PREFIX(ed_upk)

#undef eb_st
#undef eb_t
#define eb_st         RLC_PREFIX(eb_st)
#define eb_t          RLC_PREFIX(eb_t)

#undef eb_curve_init
#undef eb_curve_clean
#undef eb_curve_get_a
#undef eb_curve_get_b
#undef eb_curve_opt_a
#undef eb_curve_opt_b
#undef eb_curve_is_kbltz
#undef eb_curve_get_gen
#undef eb_curve_get_tab
#undef eb_curve_get_ord
#undef eb_curve_get_cof
#undef eb_curve_set
#undef eb_param_set
#undef eb_param_set_any
#undef eb_param_set_any_plain
#undef eb_param_set_any_kbltz
#undef eb_param_get
#undef eb_param_print
#undef eb_param_level
#undef eb_is_infty
#undef eb_set_infty
#undef eb_copy
#undef eb_cmp
#undef eb_rand
#undef eb_blind
#undef eb_rhs
#undef eb_on_curve
#undef eb_tab
#undef eb_print
#undef eb_size_bin
#undef eb_read_bin
#undef eb_write_bin
#undef eb_neg_basic
#undef eb_neg_projc
#undef eb_add_basic
#undef eb_add_projc
#undef eb_sub_basic
#undef eb_sub_projc
#undef eb_dbl_basic
#undef eb_dbl_projc
#undef eb_hlv
#undef eb_frb
#undef eb_mul_basic
#undef eb_mul_lodah
#undef eb_mul_lwnaf
#undef eb_mul_rwnaf
#undef eb_mul_halve
#undef eb_mul_gen
#undef eb_mul_dig
#undef eb_mul_pre_basic
#undef eb_mul_pre_yaowi
#undef eb_mul_pre_nafwi
#undef eb_mul_pre_combs
#undef eb_mul_pre_combd
#undef eb_mul_pre_lwnaf
#undef eb_mul_fix_basic
#undef eb_mul_fix_yaowi
#undef eb_mul_fix_nafwi
#undef eb_mul_fix_combs
#undef eb_mul_fix_combd
#undef eb_mul_fix_lwnaf
#undef eb_mul_sim_basic
#undef eb_mul_sim_trick
#undef eb_mul_sim_inter
#undef eb_mul_sim_joint
#undef eb_mul_sim_gen
#undef eb_norm
#undef eb_norm_sim
#undef eb_map
#undef eb_pck
#undef eb_upk

#define eb_curve_init 	RLC_PREFIX(eb_curve_init)
#define eb_curve_clean 	RLC_PREFIX(eb_curve_clean)
#define eb_curve_get_a 	RLC_PREFIX(eb_curve_get_a)
#define eb_curve_get_b 	RLC_PREFIX(eb_curve_get_b)
#define eb_curve_opt_a 	RLC_PREFIX(eb_curve_opt_a)
#define eb_curve_opt_b 	RLC_PREFIX(eb_curve_opt_b)
#define eb_curve_is_kbltz 	RLC_PREFIX(eb_curve_is_kbltz)
#define eb_curve_get_gen 	RLC_PREFIX(eb_curve_get_gen)
#define eb_curve_get_tab 	RLC_PREFIX(eb_curve_get_tab)
#define eb_curve_get_ord 	RLC_PREFIX(eb_curve_get_ord)
#define eb_curve_get_cof 	RLC_PREFIX(eb_curve_get_cof)
#define eb_curve_set 	RLC_PREFIX(eb_curve_set)
#define eb_param_set 	RLC_PREFIX(eb_param_set)
#define eb_param_set_any 	RLC_PREFIX(eb_param_set_any)
#define eb_param_set_any_plain 	RLC_PREFIX(eb_param_set_any_plain)
#define eb_param_set_any_kbltz 	RLC_PREFIX(eb_param_set_any_kbltz)
#define eb_param_get 	RLC_PREFIX(eb_param_get)
#define eb_param_print 	RLC_PREFIX(eb_param_print)
#define eb_param_level 	RLC_PREFIX(eb_param_level)
#define eb_is_infty 	RLC_PREFIX(eb_is_infty)
#define eb_set_infty 	RLC_PREFIX(eb_set_infty)
#define eb_copy 	RLC_PREFIX(eb_copy)
#define eb_cmp 	RLC_PREFIX(eb_cmp)
#define eb_rand 	RLC_PREFIX(eb_rand)
#define eb_blind 	RLC_PREFIX(eb_blind)
#define eb_rhs 	RLC_PREFIX(eb_rhs)
#define eb_on_curve 	RLC_PREFIX(eb_on_curve)
#define eb_tab 	RLC_PREFIX(eb_tab)
#define eb_print 	RLC_PREFIX(eb_print)
#define eb_size_bin 	RLC_PREFIX(eb_size_bin)
#define eb_read_bin 	RLC_PREFIX(eb_read_bin)
#define eb_write_bin 	RLC_PREFIX(eb_write_bin)
#define eb_neg_basic 	RLC_PREFIX(eb_neg_basic)
#define eb_neg_projc 	RLC_PREFIX(eb_neg_projc)
#define eb_add_basic 	RLC_PREFIX(eb_add_basic)
#define eb_add_projc 	RLC_PREFIX(eb_add_projc)
#define eb_sub_basic 	RLC_PREFIX(eb_sub_basic)
#define eb_sub_projc 	RLC_PREFIX(eb_sub_projc)
#define eb_dbl_basic 	RLC_PREFIX(eb_dbl_basic)
#define eb_dbl_projc 	RLC_PREFIX(eb_dbl_projc)
#define eb_hlv 	RLC_PREFIX(eb_hlv)
#define eb_frb 	RLC_PREFIX(eb_frb)
#define eb_mul_basic 	RLC_PREFIX(eb_mul_basic)
#define eb_mul_lodah 	RLC_PREFIX(eb_mul_lodah)
#define eb_mul_lwnaf 	RLC_PREFIX(eb_mul_lwnaf)
#define eb_mul_rwnaf 	RLC_PREFIX(eb_mul_rwnaf)
#define eb_mul_halve 	RLC_PREFIX(eb_mul_halve)
#define eb_mul_gen 	RLC_PREFIX(eb_mul_gen)
#define eb_mul_dig 	RLC_PREFIX(eb_mul_dig)
#define eb_mul_pre_basic 	RLC_PREFIX(eb_mul_pre_basic)
#define eb_mul_pre_yaowi 	RLC_PREFIX(eb_mul_pre_yaowi)
#define eb_mul_pre_nafwi 	RLC_PREFIX(eb_mul_pre_nafwi)
#define eb_mul_pre_combs 	RLC_PREFIX(eb_mul_pre_combs)
#define eb_mul_pre_combd 	RLC_PREFIX(eb_mul_pre_combd)
#define eb_mul_pre_lwnaf 	RLC_PREFIX(eb_mul_pre_lwnaf)
#define eb_mul_fix_basic 	RLC_PREFIX(eb_mul_fix_basic)
#define eb_mul_fix_yaowi 	RLC_PREFIX(eb_mul_fix_yaowi)
#define eb_mul_fix_nafwi 	RLC_PREFIX(eb_mul_fix_nafwi)
#define eb_mul_fix_combs 	RLC_PREFIX(eb_mul_fix_combs)
#define eb_mul_fix_combd 	RLC_PREFIX(eb_mul_fix_combd)
#define eb_mul_fix_lwnaf 	RLC_PREFIX(eb_mul_fix_lwnaf)
#define eb_mul_sim_basic 	RLC_PREFIX(eb_mul_sim_basic)
#define eb_mul_sim_trick 	RLC_PREFIX(eb_mul_sim_trick)
#define eb_mul_sim_inter 	RLC_PREFIX(eb_mul_sim_inter)
#define eb_mul_sim_joint 	RLC_PREFIX(eb_mul_sim_joint)
#define eb_mul_sim_gen 	RLC_PREFIX(eb_mul_sim_gen)
#define eb_norm 	RLC_PREFIX(eb_norm)
#define eb_norm_sim 	RLC_PREFIX(eb_norm_sim)
#define eb_map 	RLC_PREFIX(eb_map)
#define eb_pck 	RLC_PREFIX(eb_pck)
#define eb_upk 	RLC_PREFIX(eb_upk)

#undef ep2_st
#undef ep2_t
#define ep2_st        RLC_PREFIX(ep2_st)
#define ep2_t         RLC_PREFIX(ep2_t)

#undef ep2_curve_init
#undef ep2_curve_clean
#undef ep2_curve_get_a
#undef ep2_curve_get_b
#undef ep2_curve_get_vs
#undef ep2_curve_opt_a
#undef ep2_curve_opt_b
#undef ep2_curve_is_twist
#undef ep2_curve_is_ctmap
#undef ep2_curve_get_gen
#undef ep2_curve_get_tab
#undef ep2_curve_get_ord
#undef ep2_curve_get_cof
#undef ep2_curve_get_iso
#undef ep2_curve_set
#undef ep2_curve_set_twist
#undef ep2_is_infty
#undef ep2_set_infty
#undef ep2_copy
#undef ep2_cmp
#undef ep2_rand
#undef ep2_blind
#undef ep2_rhs
#undef ep2_on_curve
#undef ep2_tab
#undef ep2_print
#undef ep2_size_bin
#undef ep2_read_bin
#undef ep2_write_bin
#undef ep2_neg
#undef ep2_add_basic
#undef ep2_add_slp_basic
#undef ep2_add_projc
 #undef ep2_sub
#undef ep2_dbl_basic
#undef ep2_dbl_slp_basic
#undef ep2_dbl_projc
#undef ep2_mul_basic
#undef ep2_mul_slide
#undef ep2_mul_monty
#undef ep2_mul_lwnaf
#undef ep2_mul_lwreg
#undef ep2_mul_gen
#undef ep2_mul_dig
#undef ep2_mul_cof
#undef ep2_mul_pre_basic
#undef ep2_mul_pre_yaowi
#undef ep2_mul_pre_nafwi
#undef ep2_mul_pre_combs
#undef ep2_mul_pre_combd
#undef ep2_mul_pre_lwnaf
#undef ep2_mul_fix_basic
#undef ep2_mul_fix_yaowi
#undef ep2_mul_fix_nafwi
#undef ep2_mul_fix_combs
#undef ep2_mul_fix_combd
#undef ep2_mul_fix_lwnaf
#undef ep2_mul_sim_basic
#undef ep2_mul_sim_trick
#undef ep2_mul_sim_inter
#undef ep2_mul_sim_joint
#undef ep2_mul_sim_lot
#undef ep2_mul_sim_gen
#undef ep2_mul_sim_dig
#undef ep2_norm
#undef ep2_norm_sim
#undef ep2_map
#undef ep2_map_dst
#undef ep2_frb
#undef ep2_pck
#undef ep2_upk

#define ep2_curve_init 	RLC_PREFIX(ep2_curve_init)
#define ep2_curve_clean 	RLC_PREFIX(ep2_curve_clean)
#define ep2_curve_get_a 	RLC_PREFIX(ep2_curve_get_a)
#define ep2_curve_get_b 	RLC_PREFIX(ep2_curve_get_b)
#define ep2_curve_get_vs 	RLC_PREFIX(ep2_curve_get_vs)
#define ep2_curve_opt_a 	RLC_PREFIX(ep2_curve_opt_a)
#define ep2_curve_opt_b 	RLC_PREFIX(ep2_curve_opt_b)
#define ep2_curve_is_twist 	RLC_PREFIX(ep2_curve_is_twist)
#define ep2_curve_is_ctmap 	RLC_PREFIX(ep2_curve_is_ctmap)
#define ep2_curve_get_gen 	RLC_PREFIX(ep2_curve_get_gen)
#define ep2_curve_get_tab 	RLC_PREFIX(ep2_curve_get_tab)
#define ep2_curve_get_ord 	RLC_PREFIX(ep2_curve_get_ord)
#define ep2_curve_get_cof 	RLC_PREFIX(ep2_curve_get_cof)
#define ep2_curve_get_iso 	RLC_PREFIX(ep2_curve_get_iso)
#define ep2_curve_set 	RLC_PREFIX(ep2_curve_set)
#define ep2_curve_set_twist 	RLC_PREFIX(ep2_curve_set_twist)
#define ep2_is_infty 	RLC_PREFIX(ep2_is_infty)
#define ep2_set_infty 	RLC_PREFIX(ep2_set_infty)
#define ep2_copy 	RLC_PREFIX(ep2_copy)
#define ep2_cmp 	RLC_PREFIX(ep2_cmp)
#define ep2_rand 	RLC_PREFIX(ep2_rand)
#define ep2_blind 	RLC_PREFIX(ep2_blind)
#define ep2_rhs 	RLC_PREFIX(ep2_rhs)
#define ep2_on_curve 	RLC_PREFIX(ep2_on_curve)
#define ep2_tab 	RLC_PREFIX(ep2_tab)
#define ep2_print 	RLC_PREFIX(ep2_print)
#define ep2_size_bin 	RLC_PREFIX(ep2_size_bin)
#define ep2_read_bin 	RLC_PREFIX(ep2_read_bin)
#define ep2_write_bin 	RLC_PREFIX(ep2_write_bin)
#define ep2_neg 	RLC_PREFIX(ep2_neg)
#define ep2_add_basic 	RLC_PREFIX(ep2_add_basic)
#define ep2_add_slp_basic 	RLC_PREFIX(ep2_add_slp_basic)
#define ep2_add_projc 	RLC_PREFIX(ep2_add_projc)
 #define ep2_sub 	RLC_PREFIX(ep2_sub)
#define ep2_dbl_basic 	RLC_PREFIX(ep2_dbl_basic)
#define ep2_dbl_slp_basic 	RLC_PREFIX(ep2_dbl_slp_basic)
#define ep2_dbl_projc 	RLC_PREFIX(ep2_dbl_projc)
#define ep2_mul_basic 	RLC_PREFIX(ep2_mul_basic)
#define ep2_mul_slide 	RLC_PREFIX(ep2_mul_slide)
#define ep2_mul_monty 	RLC_PREFIX(ep2_mul_monty)
#define ep2_mul_lwnaf 	RLC_PREFIX(ep2_mul_lwnaf)
#define ep2_mul_lwreg 	RLC_PREFIX(ep2_mul_lwreg)
#define ep2_mul_gen 	RLC_PREFIX(ep2_mul_gen)
#define ep2_mul_dig 	RLC_PREFIX(ep2_mul_dig)
#define ep2_mul_cof 	RLC_PREFIX(ep2_mul_cof)
#define ep2_mul_pre_basic 	RLC_PREFIX(ep2_mul_pre_basic)
#define ep2_mul_pre_yaowi 	RLC_PREFIX(ep2_mul_pre_yaowi)
#define ep2_mul_pre_nafwi 	RLC_PREFIX(ep2_mul_pre_nafwi)
#define ep2_mul_pre_combs 	RLC_PREFIX(ep2_mul_pre_combs)
#define ep2_mul_pre_combd 	RLC_PREFIX(ep2_mul_pre_combd)
#define ep2_mul_pre_lwnaf 	RLC_PREFIX(ep2_mul_pre_lwnaf)
#define ep2_mul_fix_basic 	RLC_PREFIX(ep2_mul_fix_basic)
#define ep2_mul_fix_yaowi 	RLC_PREFIX(ep2_mul_fix_yaowi)
#define ep2_mul_fix_nafwi 	RLC_PREFIX(ep2_mul_fix_nafwi)
#define ep2_mul_fix_combs 	RLC_PREFIX(ep2_mul_fix_combs)
#define ep2_mul_fix_combd 	RLC_PREFIX(ep2_mul_fix_combd)
#define ep2_mul_fix_lwnaf 	RLC_PREFIX(ep2_mul_fix_lwnaf)
#define ep2_mul_sim_basic 	RLC_PREFIX(ep2_mul_sim_basic)
#define ep2_mul_sim_trick 	RLC_PREFIX(ep2_mul_sim_trick)
#define ep2_mul_sim_inter 	RLC_PREFIX(ep2_mul_sim_inter)
#define ep2_mul_sim_joint 	RLC_PREFIX(ep2_mul_sim_joint)
#define ep2_mul_sim_lot 	RLC_PREFIX(ep2_mul_sim_lot)
#define ep2_mul_sim_gen 	RLC_PREFIX(ep2_mul_sim_gen)
#define ep2_mul_sim_dig 	RLC_PREFIX(ep2_mul_sim_dig)
#define ep2_norm 	RLC_PREFIX(ep2_norm)
#define ep2_norm_sim 	RLC_PREFIX(ep2_norm_sim)
#define ep2_map 	RLC_PREFIX(ep2_map)
#define ep2_map_dst 	RLC_PREFIX(ep2_map_dst)
#define ep2_frb 	RLC_PREFIX(ep2_frb)
#define ep2_pck 	RLC_PREFIX(ep2_pck)
#define ep2_upk 	RLC_PREFIX(ep2_upk)

#undef fp2_st
#undef fp2_t
#undef dv2_t
#define fp2_st        RLC_PREFIX(fp2_st)
#define fp2_t         RLC_PREFIX(fp2_t)
#define dv2_t         RLC_PREFIX(dv2_t)
#undef fp3_st
#undef fp3_t
#undef dv3_t
#define fp3_st        RLC_PREFIX(fp3_st)
#define fp3_t         RLC_PREFIX(fp3_t)
#define dv3_t         RLC_PREFIX(dv3_t)
#undef fp6_st
#undef fp6_t
#undef dv6_t
#define fp6_st        RLC_PREFIX(fp6_st)
#define fp6_t         RLC_PREFIX(fp6_t)
#define dv6_t         RLC_PREFIX(dv6_t)
#undef fp9_st
#undef fp8_t
#undef dv8_t
#define fp8_st        RLC_PREFIX(fp8_st)
#define fp8_t         RLC_PREFIX(fp8_t)
#define dv8_t         RLC_PREFIX(dv8_t)
#undef fp9_st
#undef fp9_t
#undef dv9_t
#define fp9_st        RLC_PREFIX(fp9_st)
#define fp9_t         RLC_PREFIX(fp9_t)
#define dv9_t         RLC_PREFIX(dv9_t)
#undef fp12_st
#undef fp12_t
#undef dv12_t
#define fp12_st        RLC_PREFIX(fp12_st)
#define fp12_t         RLC_PREFIX(fp12_t)
#define dv12_t         RLC_PREFIX(dv12_t)
#undef fp18_st
#undef fp18_t
#undef dv18_t
#define fp18_st        RLC_PREFIX(fp18_st)
#define fp18_t         RLC_PREFIX(fp18_t)
#define dv18_t         RLC_PREFIX(dv18_t)
#undef fp24_st
#undef fp24_t
#undef dv24_t
#define fp24_st        RLC_PREFIX(fp24_st)
#define fp24_t         RLC_PREFIX(fp24_t)
#define dv24_t         RLC_PREFIX(dv24_t)
#undef fp48_st
#undef fp48_t
#undef dv48_t
#define fp48_st        RLC_PREFIX(fp48_st)
#define fp48_t         RLC_PREFIX(fp48_t)
#define dv48_t         RLC_PREFIX(dv48_t)
#undef fp54_st
#undef fp54_t
#undef dv54_t
#define fp54_st        RLC_PREFIX(fp54_st)
#define fp54_t         RLC_PREFIX(fp54_t)
#define dv54_t         RLC_PREFIX(dv54_t)

#undef fp2_add_dig
#undef fp2_sub_dig
#undef fp2_field_init
#undef fp2_field_get_qnr
#undef fp2_copy
#undef fp2_zero
#undef fp2_is_zero
#undef fp2_rand
#undef fp2_print
#undef fp2_size_bin
#undef fp2_read_bin
#undef fp2_write_bin
#undef fp2_cmp
#undef fp2_cmp_dig
#undef fp2_set_dig
#undef fp2_add_basic
#undef fp2_add_integ
#undef fp2_sub_basic
#undef fp2_sub_integ
#undef fp2_neg
#undef fp2_dbl_basic
#undef fp2_dbl_integ
#undef fp2_mul_basic
#undef fp2_mul_integ
#undef fp2_mul_art
#undef fp2_mul_nor_basic
#undef fp2_mul_nor_integ
#undef fp2_mul_frb
#undef fp2_mul_dig
#undef fp2_sqr_basic
#undef fp2_sqr_integ
#undef fp2_inv
#undef fp2_inv_cyc
#undef fp2_inv_sim
#undef fp2_test_cyc
#undef fp2_conv_cyc
#undef fp2_exp
#undef fp2_exp_dig
#undef fp2_exp_cyc
#undef fp2_frb
#undef fp2_srt
#undef fp2_pck
#undef fp2_upk
#undef fp2_exp_cyc_sim

#define fp2_add_dig 	RLC_PREFIX(fp2_add_dig)
#define fp2_sub_dig 	RLC_PREFIX(fp2_sub_dig)
#define fp2_field_init 	RLC_PREFIX(fp2_field_init)
#define fp2_field_get_qnr 	RLC_PREFIX(fp2_field_get_qnr)
#define fp2_copy 	RLC_PREFIX(fp2_copy)
#define fp2_zero 	RLC_PREFIX(fp2_zero)
#define fp2_is_zero 	RLC_PREFIX(fp2_is_zero)
#define fp2_rand 	RLC_PREFIX(fp2_rand)
#define fp2_print 	RLC_PREFIX(fp2_print)
#define fp2_size_bin 	RLC_PREFIX(fp2_size_bin)
#define fp2_read_bin 	RLC_PREFIX(fp2_read_bin)
#define fp2_write_bin 	RLC_PREFIX(fp2_write_bin)
#define fp2_cmp 	RLC_PREFIX(fp2_cmp)
#define fp2_cmp_dig 	RLC_PREFIX(fp2_cmp_dig)
#define fp2_set_dig 	RLC_PREFIX(fp2_set_dig)
#define fp2_add_basic 	RLC_PREFIX(fp2_add_basic)
#define fp2_add_integ 	RLC_PREFIX(fp2_add_integ)
#define fp2_sub_basic 	RLC_PREFIX(fp2_sub_basic)
#define fp2_sub_integ 	RLC_PREFIX(fp2_sub_integ)
#define fp2_neg 	RLC_PREFIX(fp2_neg)
#define fp2_dbl_basic 	RLC_PREFIX(fp2_dbl_basic)
#define fp2_dbl_integ 	RLC_PREFIX(fp2_dbl_integ)
#define fp2_mul_basic 	RLC_PREFIX(fp2_mul_basic)
#define fp2_mul_integ 	RLC_PREFIX(fp2_mul_integ)
#define fp2_mul_art 	RLC_PREFIX(fp2_mul_art)
#define fp2_mul_nor_basic 	RLC_PREFIX(fp2_mul_nor_basic)
#define fp2_mul_nor_integ 	RLC_PREFIX(fp2_mul_nor_integ)
#define fp2_mul_frb 	RLC_PREFIX(fp2_mul_frb)
#define fp2_mul_dig 	RLC_PREFIX(fp2_mul_dig)
#define fp2_sqr_basic 	RLC_PREFIX(fp2_sqr_basic)
#define fp2_sqr_integ 	RLC_PREFIX(fp2_sqr_integ)
#define fp2_inv 	RLC_PREFIX(fp2_inv)
#define fp2_inv_cyc 	RLC_PREFIX(fp2_inv_cyc)
#define fp2_inv_sim 	RLC_PREFIX(fp2_inv_sim)
#define fp2_test_cyc 	RLC_PREFIX(fp2_test_cyc)
#define fp2_conv_cyc 	RLC_PREFIX(fp2_conv_cyc)
#define fp2_exp 	RLC_PREFIX(fp2_exp)
#define fp2_exp_dig 	RLC_PREFIX(fp2_exp_dig)
#define fp2_exp_cyc 	RLC_PREFIX(fp2_exp_cyc)
#define fp2_frb 	RLC_PREFIX(fp2_frb)
#define fp2_srt 	RLC_PREFIX(fp2_srt)
#define fp2_pck 	RLC_PREFIX(fp2_pck)
#define fp2_upk 	RLC_PREFIX(fp2_upk)
#define fp2_exp_cyc_sim 	RLC_PREFIX(fp2_exp_cyc_sim)

#undef fp2_addn_low
#undef fp2_addm_low
#undef fp2_addd_low
#undef fp2_addc_low
#undef fp2_subn_low
#undef fp2_subm_low
#undef fp2_subd_low
#undef fp2_subc_low
#undef fp2_dbln_low
#undef fp2_dblm_low
#undef fp2_norm_low
#undef fp2_norh_low
#undef fp2_nord_low
#undef fp2_muln_low
#undef fp2_mulc_low
#undef fp2_mulm_low
#undef fp2_sqrn_low
#undef fp2_sqrm_low
#undef fp2_rdcn_low

#define fp2_addn_low 	RLC_PREFIX(fp2_addn_low)
#define fp2_addm_low 	RLC_PREFIX(fp2_addm_low)
#define fp2_addd_low 	RLC_PREFIX(fp2_addd_low)
#define fp2_addc_low 	RLC_PREFIX(fp2_addc_low)
#define fp2_subn_low 	RLC_PREFIX(fp2_subn_low)
#define fp2_subm_low 	RLC_PREFIX(fp2_subm_low)
#define fp2_subd_low 	RLC_PREFIX(fp2_subd_low)
#define fp2_subc_low 	RLC_PREFIX(fp2_subc_low)
#define fp2_dbln_low 	RLC_PREFIX(fp2_dbln_low)
#define fp2_dblm_low 	RLC_PREFIX(fp2_dblm_low)
#define fp2_norm_low 	RLC_PREFIX(fp2_norm_low)
#define fp2_norh_low 	RLC_PREFIX(fp2_norh_low)
#define fp2_nord_low 	RLC_PREFIX(fp2_nord_low)
#define fp2_muln_low 	RLC_PREFIX(fp2_muln_low)
#define fp2_mulc_low 	RLC_PREFIX(fp2_mulc_low)
#define fp2_mulm_low 	RLC_PREFIX(fp2_mulm_low)
#define fp2_sqrn_low 	RLC_PREFIX(fp2_sqrn_low)
#define fp2_sqrm_low 	RLC_PREFIX(fp2_sqrm_low)
#define fp2_rdcn_low 	RLC_PREFIX(fp2_rdcn_low)

#undef fp3_field_init
#undef fp3_copy
#undef fp3_zero
#undef fp3_is_zero
#undef fp3_rand
#undef fp3_print
#undef fp3_size_bin
#undef fp3_read_bin
#undef fp3_write_bin
#undef fp3_cmp
#undef fp3_cmp_dig
#undef fp3_set_dig
#undef fp3_add_basic
#undef fp3_add_integ
#undef fp3_sub_basic
#undef fp3_sub_integ
#undef fp3_neg
#undef fp3_dbl_basic
#undef fp3_dbl_integ
#undef fp3_mul_basic
#undef fp3_mul_integ
#undef fp3_mul_nor
#undef fp3_mul_frb
#undef fp3_sqr_basic
#undef fp3_sqr_integ
#undef fp3_inv
#undef fp3_inv_sim
#undef fp3_exp
#undef fp3_frb
#undef fp3_srt

#define fp3_field_init 	RLC_PREFIX(fp3_field_init)
#define fp3_copy 	RLC_PREFIX(fp3_copy)
#define fp3_zero 	RLC_PREFIX(fp3_zero)
#define fp3_is_zero 	RLC_PREFIX(fp3_is_zero)
#define fp3_rand 	RLC_PREFIX(fp3_rand)
#define fp3_print 	RLC_PREFIX(fp3_print)
#define fp3_size_bin 	RLC_PREFIX(fp3_size_bin)
#define fp3_read_bin 	RLC_PREFIX(fp3_read_bin)
#define fp3_write_bin 	RLC_PREFIX(fp3_write_bin)
#define fp3_cmp 	RLC_PREFIX(fp3_cmp)
#define fp3_cmp_dig 	RLC_PREFIX(fp3_cmp_dig)
#define fp3_set_dig 	RLC_PREFIX(fp3_set_dig)
#define fp3_add_basic 	RLC_PREFIX(fp3_add_basic)
#define fp3_add_integ 	RLC_PREFIX(fp3_add_integ)
#define fp3_sub_basic 	RLC_PREFIX(fp3_sub_basic)
#define fp3_sub_integ 	RLC_PREFIX(fp3_sub_integ)
#define fp3_neg 	RLC_PREFIX(fp3_neg)
#define fp3_dbl_basic 	RLC_PREFIX(fp3_dbl_basic)
#define fp3_dbl_integ 	RLC_PREFIX(fp3_dbl_integ)
#define fp3_mul_basic 	RLC_PREFIX(fp3_mul_basic)
#define fp3_mul_integ 	RLC_PREFIX(fp3_mul_integ)
#define fp3_mul_nor 	RLC_PREFIX(fp3_mul_nor)
#define fp3_mul_frb 	RLC_PREFIX(fp3_mul_frb)
#define fp3_sqr_basic 	RLC_PREFIX(fp3_sqr_basic)
#define fp3_sqr_integ 	RLC_PREFIX(fp3_sqr_integ)
#define fp3_inv 	RLC_PREFIX(fp3_inv)
#define fp3_inv_sim 	RLC_PREFIX(fp3_inv_sim)
#define fp3_exp 	RLC_PREFIX(fp3_exp)
#define fp3_frb 	RLC_PREFIX(fp3_frb)
#define fp3_srt 	RLC_PREFIX(fp3_srt)

#undef fp3_addn_low
#undef fp3_addm_low
#undef fp3_addd_low
#undef fp3_addc_low
#undef fp3_subn_low
#undef fp3_subm_low
#undef fp3_subd_low
#undef fp3_subc_low
#undef fp3_dbln_low
#undef fp3_dblm_low
#undef fp3_nord_low
#undef fp3_muln_low
#undef fp3_mulc_low
#undef fp3_mulm_low
#undef fp3_sqrn_low
#undef fp3_sqrm_low
#undef fp3_rdcn_low

#define fp3_addn_low 	RLC_PREFIX(fp3_addn_low)
#define fp3_addm_low 	RLC_PREFIX(fp3_addm_low)
#define fp3_addd_low 	RLC_PREFIX(fp3_addd_low)
#define fp3_addc_low 	RLC_PREFIX(fp3_addc_low)
#define fp3_subn_low 	RLC_PREFIX(fp3_subn_low)
#define fp3_subm_low 	RLC_PREFIX(fp3_subm_low)
#define fp3_subd_low 	RLC_PREFIX(fp3_subd_low)
#define fp3_subc_low 	RLC_PREFIX(fp3_subc_low)
#define fp3_dbln_low 	RLC_PREFIX(fp3_dbln_low)
#define fp3_dblm_low 	RLC_PREFIX(fp3_dblm_low)
#define fp3_nord_low 	RLC_PREFIX(fp3_nord_low)
#define fp3_muln_low 	RLC_PREFIX(fp3_muln_low)
#define fp3_mulc_low 	RLC_PREFIX(fp3_mulc_low)
#define fp3_mulm_low 	RLC_PREFIX(fp3_mulm_low)
#define fp3_sqrn_low 	RLC_PREFIX(fp3_sqrn_low)
#define fp3_sqrm_low 	RLC_PREFIX(fp3_sqrm_low)
#define fp3_rdcn_low 	RLC_PREFIX(fp3_rdcn_low)

#undef fp4_copy
#undef fp4_zero
#undef fp4_is_zero
#undef fp4_rand
#undef fp4_print
#undef fp4_size_bin
#undef fp4_read_bin
#undef fp4_write_bin
#undef fp4_cmp
#undef fp4_cmp_dig
#undef fp4_set_dig
#undef fp4_add
#undef fp4_sub
#undef fp4_neg
#undef fp4_dbl
#undef fp4_mul_unr
#undef fp4_mul_basic
#undef fp4_mul_lazyr
#undef fp4_mul_art
#undef fp4_mul_dxs
#undef fp4_sqr_unr
#undef fp4_sqr_basic
#undef fp4_sqr_lazyr
#undef fp4_inv
#undef fp4_inv_cyc
#undef fp4_exp
#undef fp4_frb

#define fp4_copy 	RLC_PREFIX(fp4_copy)
#define fp4_zero 	RLC_PREFIX(fp4_zero)
#define fp4_is_zero 	RLC_PREFIX(fp4_is_zero)
#define fp4_rand 	RLC_PREFIX(fp4_rand)
#define fp4_print 	RLC_PREFIX(fp4_print)
#define fp4_size_bin 	RLC_PREFIX(fp4_size_bin)
#define fp4_read_bin 	RLC_PREFIX(fp4_read_bin)
#define fp4_write_bin 	RLC_PREFIX(fp4_write_bin)
#define fp4_cmp 	RLC_PREFIX(fp4_cmp)
#define fp4_cmp_dig 	RLC_PREFIX(fp4_cmp_dig)
#define fp4_set_dig 	RLC_PREFIX(fp4_set_dig)
#define fp4_add 	RLC_PREFIX(fp4_add)
#define fp4_sub 	RLC_PREFIX(fp4_sub)
#define fp4_neg 	RLC_PREFIX(fp4_neg)
#define fp4_dbl 	RLC_PREFIX(fp4_dbl)
#define fp4_mul_unr 	RLC_PREFIX(fp4_mul_unr)
#define fp4_mul_basic 	RLC_PREFIX(fp4_mul_basic)
#define fp4_mul_lazyr 	RLC_PREFIX(fp4_mul_lazyr)
#define fp4_mul_art 	RLC_PREFIX(fp4_mul_art)
#define fp4_mul_dxs 	RLC_PREFIX(fp4_mul_dxs)
#define fp4_sqr_unr 	RLC_PREFIX(fp4_sqr_unr)
#define fp4_sqr_basic 	RLC_PREFIX(fp4_sqr_basic)
#define fp4_sqr_lazyr 	RLC_PREFIX(fp4_sqr_lazyr)
#define fp4_inv 	RLC_PREFIX(fp4_inv)
#define fp4_inv_cyc 	RLC_PREFIX(fp4_inv_cyc)
#define fp4_exp 	RLC_PREFIX(fp4_exp)
#define fp4_frb 	RLC_PREFIX(fp4_frb)

#undef fp6_copy
#undef fp6_zero
#undef fp6_is_zero
#undef fp6_rand
#undef fp6_print
#undef fp6_size_bin
#undef fp6_read_bin
#undef fp6_write_bin
#undef fp6_cmp
#undef fp6_cmp_dig
#undef fp6_set_dig
#undef fp6_add
#undef fp6_sub
#undef fp6_neg
#undef fp6_dbl
#undef fp6_mul_unr
#undef fp6_mul_basic
#undef fp6_mul_lazyr
#undef fp6_mul_art
#undef fp6_mul_dxs
#undef fp6_sqr_unr
#undef fp6_sqr_basic
#undef fp6_sqr_lazyr
#undef fp6_inv
#undef fp6_exp
#undef fp6_frb

#define fp6_copy 	RLC_PREFIX(fp6_copy)
#define fp6_zero 	RLC_PREFIX(fp6_zero)
#define fp6_is_zero 	RLC_PREFIX(fp6_is_zero)
#define fp6_rand 	RLC_PREFIX(fp6_rand)
#define fp6_print 	RLC_PREFIX(fp6_print)
#define fp6_size_bin 	RLC_PREFIX(fp6_size_bin)
#define fp6_read_bin 	RLC_PREFIX(fp6_read_bin)
#define fp6_write_bin 	RLC_PREFIX(fp6_write_bin)
#define fp6_cmp 	RLC_PREFIX(fp6_cmp)
#define fp6_cmp_dig 	RLC_PREFIX(fp6_cmp_dig)
#define fp6_set_dig 	RLC_PREFIX(fp6_set_dig)
#define fp6_add 	RLC_PREFIX(fp6_add)
#define fp6_sub 	RLC_PREFIX(fp6_sub)
#define fp6_neg 	RLC_PREFIX(fp6_neg)
#define fp6_dbl 	RLC_PREFIX(fp6_dbl)
#define fp6_mul_unr 	RLC_PREFIX(fp6_mul_unr)
#define fp6_mul_basic 	RLC_PREFIX(fp6_mul_basic)
#define fp6_mul_lazyr 	RLC_PREFIX(fp6_mul_lazyr)
#define fp6_mul_art 	RLC_PREFIX(fp6_mul_art)
#define fp6_mul_dxs 	RLC_PREFIX(fp6_mul_dxs)
#define fp6_sqr_unr 	RLC_PREFIX(fp6_sqr_unr)
#define fp6_sqr_basic 	RLC_PREFIX(fp6_sqr_basic)
#define fp6_sqr_lazyr 	RLC_PREFIX(fp6_sqr_lazyr)
#define fp6_inv 	RLC_PREFIX(fp6_inv)
#define fp6_exp 	RLC_PREFIX(fp6_exp)
#define fp6_frb 	RLC_PREFIX(fp6_frb)

#undef fp8_copy
#undef fp8_zero
#undef fp8_is_zero
#undef fp8_rand
#undef fp8_print
#undef fp8_size_bin
#undef fp8_read_bin
#undef fp8_write_bin
#undef fp8_cmp
#undef fp8_cmp_dig
#undef fp8_set_dig
#undef fp8_add
#undef fp8_sub
#undef fp8_neg
#undef fp8_dbl
#undef fp8_mul_unr
#undef fp8_mul_basic
#undef fp8_mul_lazyr
#undef fp8_mul_art
#undef fp8_mul_dxs
#undef fp8_sqr_unr
#undef fp8_sqr_basic
#undef fp8_sqr_lazyr
#undef fp8_sqr_cyc
#undef fp8_inv
#undef fp8_inv_cyc
#undef fp8_inv_sim
#undef fp8_test_cyc
#undef fp8_conv_cyc
#undef fp8_exp
#undef fp8_exp_cyc
#undef fp8_frb

#define fp8_copy 	RLC_PREFIX(fp8_copy)
#define fp8_zero 	RLC_PREFIX(fp8_zero)
#define fp8_is_zero 	RLC_PREFIX(fp8_is_zero)
#define fp8_rand 	RLC_PREFIX(fp8_rand)
#define fp8_print 	RLC_PREFIX(fp8_print)
#define fp8_size_bin 	RLC_PREFIX(fp8_size_bin)
#define fp8_read_bin 	RLC_PREFIX(fp8_read_bin)
#define fp8_write_bin 	RLC_PREFIX(fp8_write_bin)
#define fp8_cmp 	RLC_PREFIX(fp8_cmp)
#define fp8_cmp_dig 	RLC_PREFIX(fp8_cmp_dig)
#define fp8_set_dig 	RLC_PREFIX(fp8_set_dig)
#define fp8_add 	RLC_PREFIX(fp8_add)
#define fp8_sub 	RLC_PREFIX(fp8_sub)
#define fp8_neg 	RLC_PREFIX(fp8_neg)
#define fp8_dbl 	RLC_PREFIX(fp8_dbl)
#define fp8_mul_unr 	RLC_PREFIX(fp8_mul_unr)
#define fp8_mul_basic 	RLC_PREFIX(fp8_mul_basic)
#define fp8_mul_lazyr 	RLC_PREFIX(fp8_mul_lazyr)
#define fp8_mul_art 	RLC_PREFIX(fp8_mul_art)
#define fp8_mul_dxs 	RLC_PREFIX(fp8_mul_dxs)
#define fp8_sqr_unr 	RLC_PREFIX(fp8_sqr_unr)
#define fp8_sqr_basic 	RLC_PREFIX(fp8_sqr_basic)
#define fp8_sqr_lazyr 	RLC_PREFIX(fp8_sqr_lazyr)
#define fp8_sqr_cyc 	RLC_PREFIX(fp8_sqr_cyc)
#define fp8_inv 	RLC_PREFIX(fp8_inv)
#define fp8_inv_cyc 	RLC_PREFIX(fp8_inv_cyc)
#define fp8_inv_sim 	RLC_PREFIX(fp8_inv_sim)
#define fp8_test_cyc 	RLC_PREFIX(fp8_test_cyc)
#define fp8_conv_cyc 	RLC_PREFIX(fp8_conv_cyc)
#define fp8_exp 	RLC_PREFIX(fp8_exp)
#define fp8_exp_cyc 	RLC_PREFIX(fp8_exp_cyc)
#define fp8_frb 	RLC_PREFIX(fp8_frb)

#undef fp9_copy
#undef fp9_zero
#undef fp9_is_zero
#undef fp9_rand
#undef fp9_print
#undef fp9_size_bin
#undef fp9_read_bin
#undef fp9_write_bin
#undef fp9_cmp
#undef fp9_cmp_dig
#undef fp9_set_dig
#undef fp9_add
#undef fp9_sub
#undef fp9_neg
#undef fp9_dbl
#undef fp9_mul_unr
#undef fp9_mul_basic
#undef fp9_mul_lazyr
#undef fp9_mul_art
#undef fp9_mul_dxs
#undef fp9_sqr_unr
#undef fp9_sqr_basic
#undef fp9_sqr_lazyr
#undef fp9_inv
#undef fp9_inv_sim
#undef fp9_exp
#undef fp9_frb

#define fp9_copy 	RLC_PREFIX(fp9_copy)
#define fp9_zero 	RLC_PREFIX(fp9_zero)
#define fp9_is_zero 	RLC_PREFIX(fp9_is_zero)
#define fp9_rand 	RLC_PREFIX(fp9_rand)
#define fp9_print 	RLC_PREFIX(fp9_print)
#define fp9_size_bin 	RLC_PREFIX(fp9_size_bin)
#define fp9_read_bin 	RLC_PREFIX(fp9_read_bin)
#define fp9_write_bin 	RLC_PREFIX(fp9_write_bin)
#define fp9_cmp 	RLC_PREFIX(fp9_cmp)
#define fp9_cmp_dig 	RLC_PREFIX(fp9_cmp_dig)
#define fp9_set_dig 	RLC_PREFIX(fp9_set_dig)
#define fp9_add 	RLC_PREFIX(fp9_add)
#define fp9_sub 	RLC_PREFIX(fp9_sub)
#define fp9_neg 	RLC_PREFIX(fp9_neg)
#define fp9_dbl 	RLC_PREFIX(fp9_dbl)
#define fp9_mul_unr 	RLC_PREFIX(fp9_mul_unr)
#define fp9_mul_basic 	RLC_PREFIX(fp9_mul_basic)
#define fp9_mul_lazyr 	RLC_PREFIX(fp9_mul_lazyr)
#define fp9_mul_art 	RLC_PREFIX(fp9_mul_art)
#define fp9_mul_dxs 	RLC_PREFIX(fp9_mul_dxs)
#define fp9_sqr_unr 	RLC_PREFIX(fp9_sqr_unr)
#define fp9_sqr_basic 	RLC_PREFIX(fp9_sqr_basic)
#define fp9_sqr_lazyr 	RLC_PREFIX(fp9_sqr_lazyr)
#define fp9_inv 	RLC_PREFIX(fp9_inv)
#define fp9_inv_sim 	RLC_PREFIX(fp9_inv_sim)
#define fp9_exp 	RLC_PREFIX(fp9_exp)
#define fp9_frb 	RLC_PREFIX(fp9_frb)

#undef fp12_copy
#undef fp12_zero
#undef fp12_is_zero
#undef fp12_rand
#undef fp12_print
#undef fp12_size_bin
#undef fp12_read_bin
#undef fp12_write_bin
#undef fp12_cmp
#undef fp12_cmp_dig
#undef fp12_set_dig
#undef fp12_add
#undef fp12_sub
#undef fp12_neg
#undef fp12_dbl
#undef fp12_mul_unr
#undef fp12_mul_basic
#undef fp12_mul_lazyr
#undef fp12_mul_art
#undef fp12_mul_dxs_basic
#undef fp12_mul_dxs_lazyr
#undef fp12_sqr_unr
#undef fp12_sqr_basic
#undef fp12_sqr_lazyr
#undef fp12_sqr_cyc_basic
#undef fp12_sqr_cyc_lazyr
#undef fp12_sqr_pck_basic
#undef fp12_sqr_pck_lazyr
#undef fp12_test_cyc
#undef fp12_conv_cyc
#undef fp12_back_cyc
#undef fp12_back_cyc_sim
#undef fp12_inv
#undef fp12_inv_cyc
#undef fp12_frb
#undef fp12_exp
#undef fp12_exp_dig
#undef fp12_exp_cyc
#undef fp12_exp_cyc_sim
#undef fp12_exp_cyc_sps
#undef fp12_pck
#undef fp12_upk
#undef fp12_pck_max
#undef fp12_upk_max

#define fp12_copy 	RLC_PREFIX(fp12_copy)
#define fp12_zero 	RLC_PREFIX(fp12_zero)
#define fp12_is_zero 	RLC_PREFIX(fp12_is_zero)
#define fp12_rand 	RLC_PREFIX(fp12_rand)
#define fp12_print 	RLC_PREFIX(fp12_print)
#define fp12_size_bin 	RLC_PREFIX(fp12_size_bin)
#define fp12_read_bin 	RLC_PREFIX(fp12_read_bin)
#define fp12_write_bin 	RLC_PREFIX(fp12_write_bin)
#define fp12_cmp 	RLC_PREFIX(fp12_cmp)
#define fp12_cmp_dig 	RLC_PREFIX(fp12_cmp_dig)
#define fp12_set_dig 	RLC_PREFIX(fp12_set_dig)
#define fp12_add 	RLC_PREFIX(fp12_add)
#define fp12_sub 	RLC_PREFIX(fp12_sub)
#define fp12_neg 	RLC_PREFIX(fp12_neg)
#define fp12_dbl 	RLC_PREFIX(fp12_dbl)
#define fp12_mul_unr 	RLC_PREFIX(fp12_mul_unr)
#define fp12_mul_basic 	RLC_PREFIX(fp12_mul_basic)
#define fp12_mul_lazyr 	RLC_PREFIX(fp12_mul_lazyr)
#define fp12_mul_art 	RLC_PREFIX(fp12_mul_art)
#define fp12_mul_dxs_basic 	RLC_PREFIX(fp12_mul_dxs_basic)
#define fp12_mul_dxs_lazyr 	RLC_PREFIX(fp12_mul_dxs_lazyr)
#define fp12_sqr_unr 	RLC_PREFIX(fp12_sqr_unr)
#define fp12_sqr_basic 	RLC_PREFIX(fp12_sqr_basic)
#define fp12_sqr_lazyr 	RLC_PREFIX(fp12_sqr_lazyr)
#define fp12_sqr_cyc_basic 	RLC_PREFIX(fp12_sqr_cyc_basic)
#define fp12_sqr_cyc_lazyr 	RLC_PREFIX(fp12_sqr_cyc_lazyr)
#define fp12_sqr_pck_basic 	RLC_PREFIX(fp12_sqr_pck_basic)
#define fp12_sqr_pck_lazyr 	RLC_PREFIX(fp12_sqr_pck_lazyr)
#define fp12_test_cyc 	RLC_PREFIX(fp12_test_cyc)
#define fp12_conv_cyc 	RLC_PREFIX(fp12_conv_cyc)
#define fp12_back_cyc 	RLC_PREFIX(fp12_back_cyc)
#define fp12_back_cyc_sim 	RLC_PREFIX(fp12_back_cyc_sim)
#define fp12_inv 	RLC_PREFIX(fp12_inv)
#define fp12_inv_cyc 	RLC_PREFIX(fp12_inv_cyc)
#define fp12_frb 	RLC_PREFIX(fp12_frb)
#define fp12_exp 	RLC_PREFIX(fp12_exp)
#define fp12_exp_dig 	RLC_PREFIX(fp12_exp_dig)
#define fp12_exp_cyc 	RLC_PREFIX(fp12_exp_cyc)
#define fp12_exp_cyc_sim 	RLC_PREFIX(fp12_exp_cyc_sim)
#define fp12_exp_cyc_sps 	RLC_PREFIX(fp12_exp_cyc_sps)
#define fp12_pck 	RLC_PREFIX(fp12_pck)
#define fp12_upk 	RLC_PREFIX(fp12_upk)
#define fp12_pck_max 	RLC_PREFIX(fp12_pck_max)
#define fp12_upk_max 	RLC_PREFIX(fp12_upk_max)

#undef fp18_copy
#undef fp18_zero
#undef fp18_is_zero
#undef fp18_rand
#undef fp18_print
#undef fp18_size_bin
#undef fp18_read_bin
#undef fp18_write_bin
#undef fp18_cmp
#undef fp18_cmp_dig
#undef fp18_set_dig
#undef fp18_add
#undef fp18_sub
#undef fp18_neg
#undef fp18_dbl
#undef fp18_mul_unr
#undef fp18_mul_basic
#undef fp18_mul_lazyr
#undef fp18_mul_art
#undef fp18_mul_dxs_basic
#undef fp18_mul_dxs_lazyr
#undef fp18_sqr_unr
#undef fp18_sqr_basic
#undef fp18_sqr_lazyr
#undef fp18_inv
#undef fp18_inv_cyc
#undef fp18_conv_cyc
#undef fp18_frb
#undef fp18_exp

#define fp18_copy 	RLC_PREFIX(fp18_copy)
#define fp18_zero 	RLC_PREFIX(fp18_zero)
#define fp18_is_zero 	RLC_PREFIX(fp18_is_zero)
#define fp18_rand 	RLC_PREFIX(fp18_rand)
#define fp18_print 	RLC_PREFIX(fp18_print)
#define fp18_size_bin 	RLC_PREFIX(fp18_size_bin)
#define fp18_read_bin 	RLC_PREFIX(fp18_read_bin)
#define fp18_write_bin 	RLC_PREFIX(fp18_write_bin)
#define fp18_cmp 	RLC_PREFIX(fp18_cmp)
#define fp18_cmp_dig 	RLC_PREFIX(fp18_cmp_dig)
#define fp18_set_dig 	RLC_PREFIX(fp18_set_dig)
#define fp18_add 	RLC_PREFIX(fp18_add)
#define fp18_sub 	RLC_PREFIX(fp18_sub)
#define fp18_neg 	RLC_PREFIX(fp18_neg)
#define fp18_dbl 	RLC_PREFIX(fp18_dbl)
#define fp18_mul_unr 	RLC_PREFIX(fp18_mul_unr)
#define fp18_mul_basic 	RLC_PREFIX(fp18_mul_basic)
#define fp18_mul_lazyr 	RLC_PREFIX(fp18_mul_lazyr)
#define fp18_mul_art 	RLC_PREFIX(fp18_mul_art)
#define fp18_mul_dxs_basic 	RLC_PREFIX(fp18_mul_dxs_basic)
#define fp18_mul_dxs_lazyr 	RLC_PREFIX(fp18_mul_dxs_lazyr)
#define fp18_sqr_unr 	RLC_PREFIX(fp18_sqr_unr)
#define fp18_sqr_basic 	RLC_PREFIX(fp18_sqr_basic)
#define fp18_sqr_lazyr 	RLC_PREFIX(fp18_sqr_lazyr)
#define fp18_inv 	RLC_PREFIX(fp18_inv)
#define fp18_inv_cyc 	RLC_PREFIX(fp18_inv_cyc)
#define fp18_conv_cyc 	RLC_PREFIX(fp18_conv_cyc)
#define fp18_frb 	RLC_PREFIX(fp18_frb)
#define fp18_exp 	RLC_PREFIX(fp18_exp)

#undef fp24_copy
#undef fp24_zero
#undef fp24_is_zero
#undef fp24_rand
#undef fp24_print
#undef fp24_size_bin
#undef fp24_read_bin
#undef fp24_write_bin
#undef fp24_cmp
#undef fp24_cmp_dig
#undef fp24_set_dig
#undef fp24_add
#undef fp24_sub
#undef fp24_neg
#undef fp24_dbl
#undef fp24_mul_unr
#undef fp24_mul_basic
#undef fp24_mul_lazyr
#undef fp24_mul_art
#undef fp24_mul_dxs
#undef fp24_sqr_unr
#undef fp24_sqr_basic
#undef fp24_sqr_lazyr
#undef fp24_inv
#undef fp24_frb
#undef fp24_exp

#define fp24_copy 	RLC_PREFIX(fp24_copy)
#define fp24_zero 	RLC_PREFIX(fp24_zero)
#define fp24_is_zero 	RLC_PREFIX(fp24_is_zero)
#define fp24_rand 	RLC_PREFIX(fp24_rand)
#define fp24_print 	RLC_PREFIX(fp24_print)
#define fp24_size_bin 	RLC_PREFIX(fp24_size_bin)
#define fp24_read_bin 	RLC_PREFIX(fp24_read_bin)
#define fp24_write_bin 	RLC_PREFIX(fp24_write_bin)
#define fp24_cmp 	RLC_PREFIX(fp24_cmp)
#define fp24_cmp_dig 	RLC_PREFIX(fp24_cmp_dig)
#define fp24_set_dig 	RLC_PREFIX(fp24_set_dig)
#define fp24_add 	RLC_PREFIX(fp24_add)
#define fp24_sub 	RLC_PREFIX(fp24_sub)
#define fp24_neg 	RLC_PREFIX(fp24_neg)
#define fp24_dbl 	RLC_PREFIX(fp24_dbl)
#define fp24_mul_unr 	RLC_PREFIX(fp24_mul_unr)
#define fp24_mul_basic 	RLC_PREFIX(fp24_mul_basic)
#define fp24_mul_lazyr 	RLC_PREFIX(fp24_mul_lazyr)
#define fp24_mul_art 	RLC_PREFIX(fp24_mul_art)
#define fp24_mul_dxs 	RLC_PREFIX(fp24_mul_dxs)
#define fp24_sqr_unr 	RLC_PREFIX(fp24_sqr_unr)
#define fp24_sqr_basic 	RLC_PREFIX(fp24_sqr_basic)
#define fp24_sqr_lazyr 	RLC_PREFIX(fp24_sqr_lazyr)
#define fp24_inv 	RLC_PREFIX(fp24_inv)
#define fp24_frb 	RLC_PREFIX(fp24_frb)
#define fp24_exp 	RLC_PREFIX(fp24_exp)

#undef fp48_copy
#undef fp48_zero
#undef fp48_is_zero
#undef fp48_rand
#undef fp48_print
#undef fp48_size_bin
#undef fp48_read_bin
#undef fp48_write_bin
#undef fp48_cmp
#undef fp48_cmp_dig
#undef fp48_set_dig
#undef fp48_add
#undef fp48_sub
#undef fp48_neg
#undef fp48_dbl
#undef fp48_mul_basic
#undef fp48_mul_lazyr
#undef fp48_mul_art
#undef fp48_mul_dxs
#undef fp48_sqr_basic
#undef fp48_sqr_lazyr
#undef fp48_sqr_cyc_basic
#undef fp48_sqr_cyc_lazyr
#undef fp48_sqr_pck_basic
#undef fp48_sqr_pck_lazyr
#undef fp48_test_cyc
#undef fp48_conv_cyc
#undef fp48_back_cyc
#undef fp48_back_cyc_sim
#undef fp48_inv
#undef fp48_inv_cyc
#undef fp48_conv_cyc
#undef fp48_frb
#undef fp48_exp
#undef fp48_exp_dig
#undef fp48_exp_cyc
#undef fp48_exp_cyc_sps
#undef fp48_pck
#undef fp48_upk

#define fp48_copy 	RLC_PREFIX(fp48_copy)
#define fp48_zero 	RLC_PREFIX(fp48_zero)
#define fp48_is_zero 	RLC_PREFIX(fp48_is_zero)
#define fp48_rand 	RLC_PREFIX(fp48_rand)
#define fp48_print 	RLC_PREFIX(fp48_print)
#define fp48_size_bin 	RLC_PREFIX(fp48_size_bin)
#define fp48_read_bin 	RLC_PREFIX(fp48_read_bin)
#define fp48_write_bin 	RLC_PREFIX(fp48_write_bin)
#define fp48_cmp 	RLC_PREFIX(fp48_cmp)
#define fp48_cmp_dig 	RLC_PREFIX(fp48_cmp_dig)
#define fp48_set_dig 	RLC_PREFIX(fp48_set_dig)
#define fp48_add 	RLC_PREFIX(fp48_add)
#define fp48_sub 	RLC_PREFIX(fp48_sub)
#define fp48_neg 	RLC_PREFIX(fp48_neg)
#define fp48_dbl 	RLC_PREFIX(fp48_dbl)
#define fp48_mul_basic 	RLC_PREFIX(fp48_mul_basic)
#define fp48_mul_lazyr 	RLC_PREFIX(fp48_mul_lazyr)
#define fp48_mul_art 	RLC_PREFIX(fp48_mul_art)
#define fp48_mul_dxs 	RLC_PREFIX(fp48_mul_dxs)
#define fp48_sqr_basic 	RLC_PREFIX(fp48_sqr_basic)
#define fp48_sqr_lazyr 	RLC_PREFIX(fp48_sqr_lazyr)
#define fp48_sqr_cyc_basic 	RLC_PREFIX(fp48_sqr_cyc_basic)
#define fp48_sqr_cyc_lazyr 	RLC_PREFIX(fp48_sqr_cyc_lazyr)
#define fp48_sqr_pck_basic 	RLC_PREFIX(fp48_sqr_pck_basic)
#define fp48_sqr_pck_lazyr 	RLC_PREFIX(fp48_sqr_pck_lazyr)
#define fp48_test_cyc 	RLC_PREFIX(fp48_test_cyc)
#define fp48_conv_cyc 	RLC_PREFIX(fp48_conv_cyc)
#define fp48_back_cyc 	RLC_PREFIX(fp48_back_cyc)
#define fp48_back_cyc_sim 	RLC_PREFIX(fp48_back_cyc_sim)
#define fp48_inv 	RLC_PREFIX(fp48_inv)
#define fp48_inv_cyc 	RLC_PREFIX(fp48_inv_cyc)
#define fp48_conv_cyc 	RLC_PREFIX(fp48_conv_cyc)
#define fp48_frb 	RLC_PREFIX(fp48_frb)
#define fp48_exp 	RLC_PREFIX(fp48_exp)
#define fp48_exp_dig 	RLC_PREFIX(fp48_exp_dig)
#define fp48_exp_cyc 	RLC_PREFIX(fp48_exp_cyc)
#define fp48_exp_cyc_sps 	RLC_PREFIX(fp48_exp_cyc_sps)
#define fp48_pck 	RLC_PREFIX(fp48_pck)
#define fp48_upk 	RLC_PREFIX(fp48_upk)

#undef fp54_copy
#undef fp54_zero
#undef fp54_is_zero
#undef fp54_rand
#undef fp54_print
#undef fp54_size_bin
#undef fp54_read_bin
#undef fp54_write_bin
#undef fp54_cmp
#undef fp54_cmp_dig
#undef fp54_set_dig
#undef fp54_add
#undef fp54_sub
#undef fp54_neg
#undef fp54_dbl
#undef fp54_mul_basic
#undef fp54_mul_lazyr
#undef fp54_mul_art
#undef fp54_mul_dxs
#undef fp54_sqr_basic
#undef fp54_sqr_lazyr
#undef fp54_sqr_cyc_basic
#undef fp54_sqr_cyc_lazyr
#undef fp54_sqr_pck_basic
#undef fp54_sqr_pck_lazyr
#undef fp54_test_cyc
#undef fp54_conv_cyc
#undef fp54_back_cyc
#undef fp54_back_cyc_sim
#undef fp54_inv
#undef fp54_inv_cyc
#undef fp54_conv_cyc
#undef fp54_frb
#undef fp54_exp
#undef fp54_exp_dig
#undef fp54_exp_cyc
#undef fp54_exp_cyc_sps
#undef fp54_pck
#undef fp54_upk

#define fp54_copy 	RLC_PREFIX(fp54_copy)
#define fp54_zero 	RLC_PREFIX(fp54_zero)
#define fp54_is_zero 	RLC_PREFIX(fp54_is_zero)
#define fp54_rand 	RLC_PREFIX(fp54_rand)
#define fp54_print 	RLC_PREFIX(fp54_print)
#define fp54_size_bin 	RLC_PREFIX(fp54_size_bin)
#define fp54_read_bin 	RLC_PREFIX(fp54_read_bin)
#define fp54_write_bin 	RLC_PREFIX(fp54_write_bin)
#define fp54_cmp 	RLC_PREFIX(fp54_cmp)
#define fp54_cmp_dig 	RLC_PREFIX(fp54_cmp_dig)
#define fp54_set_dig 	RLC_PREFIX(fp54_set_dig)
#define fp54_add 	RLC_PREFIX(fp54_add)
#define fp54_sub 	RLC_PREFIX(fp54_sub)
#define fp54_neg 	RLC_PREFIX(fp54_neg)
#define fp54_dbl 	RLC_PREFIX(fp54_dbl)
#define fp54_mul_basic 	RLC_PREFIX(fp54_mul_basic)
#define fp54_mul_lazyr 	RLC_PREFIX(fp54_mul_lazyr)
#define fp54_mul_art 	RLC_PREFIX(fp54_mul_art)
#define fp54_mul_dxs 	RLC_PREFIX(fp54_mul_dxs)
#define fp54_sqr_basic 	RLC_PREFIX(fp54_sqr_basic)
#define fp54_sqr_lazyr 	RLC_PREFIX(fp54_sqr_lazyr)
#define fp54_sqr_cyc_basic 	RLC_PREFIX(fp54_sqr_cyc_basic)
#define fp54_sqr_cyc_lazyr 	RLC_PREFIX(fp54_sqr_cyc_lazyr)
#define fp54_sqr_pck_basic 	RLC_PREFIX(fp54_sqr_pck_basic)
#define fp54_sqr_pck_lazyr 	RLC_PREFIX(fp54_sqr_pck_lazyr)
#define fp54_test_cyc 	RLC_PREFIX(fp54_test_cyc)
#define fp54_conv_cyc 	RLC_PREFIX(fp54_conv_cyc)
#define fp54_back_cyc 	RLC_PREFIX(fp54_back_cyc)
#define fp54_back_cyc_sim 	RLC_PREFIX(fp54_back_cyc_sim)
#define fp54_inv 	RLC_PREFIX(fp54_inv)
#define fp54_inv_cyc 	RLC_PREFIX(fp54_inv_cyc)
#define fp54_conv_cyc 	RLC_PREFIX(fp54_conv_cyc)
#define fp54_frb 	RLC_PREFIX(fp54_frb)
#define fp54_exp 	RLC_PREFIX(fp54_exp)
#define fp54_exp_dig 	RLC_PREFIX(fp54_exp_dig)
#define fp54_exp_cyc 	RLC_PREFIX(fp54_exp_cyc)
#define fp54_exp_cyc_sps 	RLC_PREFIX(fp54_exp_cyc_sps)
#define fp54_pck 	RLC_PREFIX(fp54_pck)
#define fp54_upk 	RLC_PREFIX(fp54_upk)

#undef fb2_mul
 #undef fb2_mul_nor
#undef fb2_sqr
#undef fb2_slv
#undef fb2_inv

#define fb2_mul 	RLC_PREFIX(fb2_mul)
 #define fb2_mul_nor 	RLC_PREFIX(fb2_mul_nor)
#define fb2_sqr 	RLC_PREFIX(fb2_sqr)
#define fb2_slv 	RLC_PREFIX(fb2_slv)
#define fb2_inv 	RLC_PREFIX(fb2_inv)



#undef pp_map_init
#undef pp_map_clean
#undef pp_add_k2_basic
#undef pp_add_k2_projc_basic
#undef pp_add_k2_projc_lazyr
#undef pp_add_k8_basic
#undef pp_add_k8_projc_basic
#undef pp_add_k8_projc_lazyr
#undef pp_add_k12_basic
#undef pp_add_k12_projc_basic
#undef pp_add_k12_projc_lazyr
#undef pp_add_lit_k12
#undef pp_add_k48_basic
#undef pp_add_k48_projc
#undef pp_add_k54_basic
#undef pp_add_k54_projc
#undef pp_dbl_k2_basic
#undef pp_dbl_k2_projc_basic
#undef pp_dbl_k2_projc_lazyr
#undef pp_dbl_k8_basic
#undef pp_dbl_k8_projc_basic
#undef pp_dbl_k8_projc_lazyr
#undef pp_dbl_k12_basic
#undef pp_dbl_k12_projc_basic
#undef pp_dbl_k12_projc_lazyr
#undef pp_dbl_k48_basic
#undef pp_dbl_k48_projc
#undef pp_dbl_k54_basic
#undef pp_dbl_k54_projc
#undef pp_dbl_lit_k12
#undef pp_exp_k2
#undef pp_exp_k8
#undef pp_exp_k12
#undef pp_exp_k48
#undef pp_exp_k54
#undef pp_norm_k2
#undef pp_norm_k8
#undef pp_norm_k12
#undef pp_map_tatep_k2
#undef pp_map_sim_tatep_k2
#undef pp_map_weilp_k2
#undef pp_map_oatep_k8
#undef pp_map_sim_weilp_k2
#undef pp_map_tatep_k12
#undef pp_map_sim_tatep_k12
#undef pp_map_weilp_k12
#undef pp_map_sim_weilp_k12
#undef pp_map_oatep_k12
#undef pp_map_sim_oatep_k12
#undef pp_map_k48
#undef pp_map_k54

#define pp_map_init 	RLC_PREFIX(pp_map_init)
#define pp_map_clean 	RLC_PREFIX(pp_map_clean)
#define pp_add_k2_basic 	RLC_PREFIX(pp_add_k2_basic)
#define pp_add_k2_projc_basic 	RLC_PREFIX(pp_add_k2_projc_basic)
#define pp_add_k2_projc_lazyr 	RLC_PREFIX(pp_add_k2_projc_lazyr)
#define pp_add_k8_basic 	RLC_PREFIX(pp_add_k8_basic)
#define pp_add_k8_projc_basic 	RLC_PREFIX(pp_add_k8_projc_basic)
#define pp_add_k8_projc_lazyr 	RLC_PREFIX(pp_add_k8_projc_lazyr)
#define pp_add_k12_basic 	RLC_PREFIX(pp_add_k12_basic)
#define pp_add_k12_projc_basic 	RLC_PREFIX(pp_add_k12_projc_basic)
#define pp_add_k12_projc_lazyr 	RLC_PREFIX(pp_add_k12_projc_lazyr)
#define pp_add_lit_k12 	RLC_PREFIX(pp_add_lit_k12)
#define pp_add_k48_basic 	RLC_PREFIX(pp_add_k48_basic)
#define pp_add_k48_projc 	RLC_PREFIX(pp_add_k48_projc)
#define pp_add_k54_basic 	RLC_PREFIX(pp_add_k54_basic)
#define pp_add_k54_projc 	RLC_PREFIX(pp_add_k54_projc)
#define pp_dbl_k2_basic 	RLC_PREFIX(pp_dbl_k2_basic)
#define pp_dbl_k2_projc_basic 	RLC_PREFIX(pp_dbl_k2_projc_basic)
#define pp_dbl_k2_projc_lazyr 	RLC_PREFIX(pp_dbl_k2_projc_lazyr)
#define pp_dbl_k8_basic 	RLC_PREFIX(pp_dbl_k8_basic)
#define pp_dbl_k8_projc_basic 	RLC_PREFIX(pp_dbl_k8_projc_basic)
#define pp_dbl_k8_projc_lazyr 	RLC_PREFIX(pp_dbl_k8_projc_lazyr)
#define pp_dbl_k12_basic 	RLC_PREFIX(pp_dbl_k12_basic)
#define pp_dbl_k12_projc_basic 	RLC_PREFIX(pp_dbl_k12_projc_basic)
#define pp_dbl_k12_projc_lazyr 	RLC_PREFIX(pp_dbl_k12_projc_lazyr)
#define pp_dbl_k48_basic 	RLC_PREFIX(pp_dbl_k48_basic)
#define pp_dbl_k48_projc 	RLC_PREFIX(pp_dbl_k48_projc)
#define pp_dbl_k54_basic 	RLC_PREFIX(pp_dbl_k54_basic)
#define pp_dbl_k54_projc 	RLC_PREFIX(pp_dbl_k54_projc)
#define pp_dbl_lit_k12 	RLC_PREFIX(pp_dbl_lit_k12)
#define pp_exp_k2 	RLC_PREFIX(pp_exp_k2)
#define pp_exp_k8 	RLC_PREFIX(pp_exp_k8)
#define pp_exp_k12 	RLC_PREFIX(pp_exp_k12)
#define pp_exp_k48 	RLC_PREFIX(pp_exp_k48)
#define pp_exp_k54 	RLC_PREFIX(pp_exp_k54)
#define pp_norm_k2 	RLC_PREFIX(pp_norm_k2)
#define pp_norm_k8 	RLC_PREFIX(pp_norm_k8)
#define pp_norm_k12 	RLC_PREFIX(pp_norm_k12)
#define pp_map_tatep_k2 	RLC_PREFIX(pp_map_tatep_k2)
#define pp_map_sim_tatep_k2 	RLC_PREFIX(pp_map_sim_tatep_k2)
#define pp_map_weilp_k2 	RLC_PREFIX(pp_map_weilp_k2)
#define pp_map_oatep_k8 	RLC_PREFIX(pp_map_oatep_k8)
#define pp_map_sim_weilp_k2 	RLC_PREFIX(pp_map_sim_weilp_k2)
#define pp_map_tatep_k12 	RLC_PREFIX(pp_map_tatep_k12)
#define pp_map_sim_tatep_k12 	RLC_PREFIX(pp_map_sim_tatep_k12)
#define pp_map_weilp_k12 	RLC_PREFIX(pp_map_weilp_k12)
#define pp_map_sim_weilp_k12 	RLC_PREFIX(pp_map_sim_weilp_k12)
#define pp_map_oatep_k12 	RLC_PREFIX(pp_map_oatep_k12)
#define pp_map_sim_oatep_k12 	RLC_PREFIX(pp_map_sim_oatep_k12)
#define pp_map_k48 	RLC_PREFIX(pp_map_k48)
#define pp_map_k54 	RLC_PREFIX(pp_map_k54)

#undef crt_t
#undef rsa_t
#undef rabin_t
#undef phpe_t
#undef bdpe_t
#undef sokaka_t
#define crt_t		RLC_PREFIX(crt_t)
#define rsa_t		RLC_PREFIX(rsa_t)
#define rabin_t	RLC_PREFIX(rabin_t)
#define phpe_t	RLC_PREFIX(phpe_t)
#define bdpe_t	RLC_PREFIX(bdpe_t)
#define sokaka_t	RLC_PREFIX(sokaka_t)

#undef cp_rsa_gen
#undef cp_rsa_enc
#undef cp_rsa_dec
#undef cp_rsa_sig
#undef cp_rsa_ver
#undef cp_rabin_gen
#undef cp_rabin_enc
#undef cp_rabin_dec
#undef cp_bdpe_gen
#undef cp_bdpe_enc
#undef cp_bdpe_dec
#undef cp_phpe_gen
#undef cp_phpe_enc
#undef cp_phpe_dec
#undef cp_ghpe_gen
#undef cp_ghpe_enc
#undef cp_ghpe_dec
#undef cp_ecdh_gen
#undef cp_ecdh_key
#undef cp_ecmqv_gen
#undef cp_ecmqv_key
#undef cp_ecies_gen
#undef cp_ecies_enc
#undef cp_ecies_dec
#undef cp_ecdsa_gen
#undef cp_ecdsa_sig
#undef cp_ecdsa_ver
#undef cp_ecss_gen
#undef cp_ecss_sig
#undef cp_ecss_ver
#undef cp_sokaka_gen
#undef cp_sokaka_gen_prv
#undef cp_sokaka_key
#undef cp_bgn_gen
#undef cp_bgn_enc1
#undef cp_bgn_dec1
#undef cp_bgn_enc2
#undef cp_bgn_dec2
#undef cp_bgn_add
#undef cp_bgn_mul
#undef cp_bgn_dec
#undef cp_ibe_gen
#undef cp_ibe_gen_prv
#undef cp_ibe_enc
#undef cp_ibe_dec
#undef cp_bls_gen
#undef cp_bls_sig
#undef cp_bls_ver
#undef cp_bbs_gen
#undef cp_bbs_sig
#undef cp_bbs_ver
#undef cp_cls_gen
#undef cp_cls_sig
#undef cp_cls_ver
#undef cp_cli_gen
#undef cp_cli_sig
#undef cp_cli_ver
#undef cp_clb_gen
#undef cp_clb_sig
#undef cp_clb_ver
#undef cp_pss_gen
#undef cp_pss_sig
#undef cp_pss_ver
#undef cp_mpss_gen
#undef cp_mpss_sig
#undef cp_mpss_bct
#undef cp_mpss_ver
#undef cp_psb_gen
#undef cp_psb_sig
#undef cp_psb_ver
#undef cp_mpsb_gen
#undef cp_mpsb_sig
#undef cp_mpsb_bct
#undef cp_mpsb_ver
#undef cp_zss_gen
#undef cp_zss_sig
#undef cp_zss_ver
#undef cp_vbnn_gen
#undef cp_vbnn_gen_prv
#undef cp_vbnn_sig
#undef cp_vbnn_ver
#undef cp_cmlhs_init
#undef cp_cmlhs_gen
#undef cp_cmlhs_sig
#undef cp_cmlhs_fun
#undef cp_cmlhs_evl
#undef cp_cmlhs_ver
#undef cp_cmlhs_off
#undef cp_cmlhs_onv
#undef cp_mklhs_gen
#undef cp_mklhs_sig
#undef cp_mklhs_fun
#undef cp_mklhs_evl
#undef cp_mklhs_ver
#undef cp_mklhs_off
#undef cp_mklhs_onv

#define cp_rsa_gen 	RLC_PREFIX(cp_rsa_gen)
#define cp_rsa_enc 	RLC_PREFIX(cp_rsa_enc)
#define cp_rsa_dec 	RLC_PREFIX(cp_rsa_dec)
#define cp_rsa_sig 	RLC_PREFIX(cp_rsa_sig)
#define cp_rsa_ver 	RLC_PREFIX(cp_rsa_ver)
#define cp_rabin_gen 	RLC_PREFIX(cp_rabin_gen)
#define cp_rabin_enc 	RLC_PREFIX(cp_rabin_enc)
#define cp_rabin_dec 	RLC_PREFIX(cp_rabin_dec)
#define cp_bdpe_gen 	RLC_PREFIX(cp_bdpe_gen)
#define cp_bdpe_enc 	RLC_PREFIX(cp_bdpe_enc)
#define cp_bdpe_dec 	RLC_PREFIX(cp_bdpe_dec)
#define cp_phpe_gen 	RLC_PREFIX(cp_phpe_gen)
#define cp_phpe_enc 	RLC_PREFIX(cp_phpe_enc)
#define cp_phpe_dec 	RLC_PREFIX(cp_phpe_dec)
#define cp_ghpe_gen 	RLC_PREFIX(cp_ghpe_gen)
#define cp_ghpe_enc 	RLC_PREFIX(cp_ghpe_enc)
#define cp_ghpe_dec 	RLC_PREFIX(cp_ghpe_dec)
#define cp_ecdh_gen 	RLC_PREFIX(cp_ecdh_gen)
#define cp_ecdh_key 	RLC_PREFIX(cp_ecdh_key)
#define cp_ecmqv_gen 	RLC_PREFIX(cp_ecmqv_gen)
#define cp_ecmqv_key 	RLC_PREFIX(cp_ecmqv_key)
#define cp_ecies_gen 	RLC_PREFIX(cp_ecies_gen)
#define cp_ecies_enc 	RLC_PREFIX(cp_ecies_enc)
#define cp_ecies_dec 	RLC_PREFIX(cp_ecies_dec)
#define cp_ecdsa_gen 	RLC_PREFIX(cp_ecdsa_gen)
#define cp_ecdsa_sig 	RLC_PREFIX(cp_ecdsa_sig)
#define cp_ecdsa_ver 	RLC_PREFIX(cp_ecdsa_ver)
#define cp_ecss_gen 	RLC_PREFIX(cp_ecss_gen)
#define cp_ecss_sig 	RLC_PREFIX(cp_ecss_sig)
#define cp_ecss_ver 	RLC_PREFIX(cp_ecss_ver)
#define cp_sokaka_gen 	RLC_PREFIX(cp_sokaka_gen)
#define cp_sokaka_gen_prv 	RLC_PREFIX(cp_sokaka_gen_prv)
#define cp_sokaka_key 	RLC_PREFIX(cp_sokaka_key)
#define cp_bgn_gen 	RLC_PREFIX(cp_bgn_gen)
#define cp_bgn_enc1 	RLC_PREFIX(cp_bgn_enc1)
#define cp_bgn_dec1 	RLC_PREFIX(cp_bgn_dec1)
#define cp_bgn_enc2 	RLC_PREFIX(cp_bgn_enc2)
#define cp_bgn_dec2 	RLC_PREFIX(cp_bgn_dec2)
#define cp_bgn_add 	RLC_PREFIX(cp_bgn_add)
#define cp_bgn_mul 	RLC_PREFIX(cp_bgn_mul)
#define cp_bgn_dec 	RLC_PREFIX(cp_bgn_dec)
#define cp_ibe_gen 	RLC_PREFIX(cp_ibe_gen)
#define cp_ibe_gen_prv 	RLC_PREFIX(cp_ibe_gen_prv)
#define cp_ibe_enc 	RLC_PREFIX(cp_ibe_enc)
#define cp_ibe_dec 	RLC_PREFIX(cp_ibe_dec)
#define cp_bls_gen 	RLC_PREFIX(cp_bls_gen)
#define cp_bls_sig 	RLC_PREFIX(cp_bls_sig)
#define cp_bls_ver 	RLC_PREFIX(cp_bls_ver)
#define cp_bbs_gen 	RLC_PREFIX(cp_bbs_gen)
#define cp_bbs_sig 	RLC_PREFIX(cp_bbs_sig)
#define cp_bbs_ver 	RLC_PREFIX(cp_bbs_ver)
#define cp_cls_gen 	RLC_PREFIX(cp_cls_gen)
#define cp_cls_sig 	RLC_PREFIX(cp_cls_sig)
#define cp_cls_ver 	RLC_PREFIX(cp_cls_ver)
#define cp_cli_gen 	RLC_PREFIX(cp_cli_gen)
#define cp_cli_sig 	RLC_PREFIX(cp_cli_sig)
#define cp_cli_ver 	RLC_PREFIX(cp_cli_ver)
#define cp_clb_gen 	RLC_PREFIX(cp_clb_gen)
#define cp_clb_sig 	RLC_PREFIX(cp_clb_sig)
#define cp_clb_ver 	RLC_PREFIX(cp_clb_ver)
#define cp_pss_gen 	RLC_PREFIX(cp_pss_gen)
#define cp_pss_sig 	RLC_PREFIX(cp_pss_sig)
#define cp_pss_ver 	RLC_PREFIX(cp_pss_ver)
#define cp_mpss_gen 	RLC_PREFIX(cp_mpss_gen)
#define cp_mpss_sig 	RLC_PREFIX(cp_mpss_sig)
#define cp_mpss_bct 	RLC_PREFIX(cp_mpss_bct)
#define cp_mpss_ver 	RLC_PREFIX(cp_mpss_ver)
#define cp_psb_gen 	RLC_PREFIX(cp_psb_gen)
#define cp_psb_sig 	RLC_PREFIX(cp_psb_sig)
#define cp_psb_ver 	RLC_PREFIX(cp_psb_ver)
#define cp_mpsb_gen 	RLC_PREFIX(cp_mpsb_gen)
#define cp_mpsb_sig 	RLC_PREFIX(cp_mpsb_sig)
#define cp_mpsb_bct 	RLC_PREFIX(cp_mpsb_bct)
#define cp_mpsb_ver 	RLC_PREFIX(cp_mpsb_ver)
#define cp_zss_gen 	RLC_PREFIX(cp_zss_gen)
#define cp_zss_sig 	RLC_PREFIX(cp_zss_sig)
#define cp_zss_ver 	RLC_PREFIX(cp_zss_ver)
#define cp_vbnn_gen 	RLC_PREFIX(cp_vbnn_gen)
#define cp_vbnn_gen_prv 	RLC_PREFIX(cp_vbnn_gen_prv)
#define cp_vbnn_sig 	RLC_PREFIX(cp_vbnn_sig)
#define cp_vbnn_ver 	RLC_PREFIX(cp_vbnn_ver)
#define cp_cmlhs_init 	RLC_PREFIX(cp_cmlhs_init)
#define cp_cmlhs_gen 	RLC_PREFIX(cp_cmlhs_gen)
#define cp_cmlhs_sig 	RLC_PREFIX(cp_cmlhs_sig)
#define cp_cmlhs_fun 	RLC_PREFIX(cp_cmlhs_fun)
#define cp_cmlhs_evl 	RLC_PREFIX(cp_cmlhs_evl)
#define cp_cmlhs_ver 	RLC_PREFIX(cp_cmlhs_ver)
#define cp_cmlhs_off 	RLC_PREFIX(cp_cmlhs_off)
#define cp_cmlhs_onv 	RLC_PREFIX(cp_cmlhs_onv)
#define cp_mklhs_gen 	RLC_PREFIX(cp_mklhs_gen)
#define cp_mklhs_sig 	RLC_PREFIX(cp_mklhs_sig)
#define cp_mklhs_fun 	RLC_PREFIX(cp_mklhs_fun)
#define cp_mklhs_evl 	RLC_PREFIX(cp_mklhs_evl)
#define cp_mklhs_ver 	RLC_PREFIX(cp_mklhs_ver)
#define cp_mklhs_off 	RLC_PREFIX(cp_mklhs_off)
#define cp_mklhs_onv 	RLC_PREFIX(cp_mklhs_onv)

#undef md_map_sh224
#undef md_map_sh256
#undef md_map_sh384
#undef md_map_sh512
#undef md_map_b2s160
#undef md_map_b2s256
#undef md_kdf
#undef md_mgf
#undef md_hmac
#undef md_xmd_sh224
#undef md_xmd_sh256
#undef md_xmd_sh384
#undef md_xmd_sh512

#define md_map_sh224 	RLC_PREFIX(md_map_sh224)
#define md_map_sh256 	RLC_PREFIX(md_map_sh256)
#define md_map_sh384 	RLC_PREFIX(md_map_sh384)
#define md_map_sh512 	RLC_PREFIX(md_map_sh512)
#define md_map_b2s160 	RLC_PREFIX(md_map_b2s160)
#define md_map_b2s256 	RLC_PREFIX(md_map_b2s256)
#define md_kdf 	RLC_PREFIX(md_kdf)
#define md_mgf 	RLC_PREFIX(md_mgf)
#define md_hmac 	RLC_PREFIX(md_hmac)
#define md_xmd_sh224 	RLC_PREFIX(md_xmd_sh224)
#define md_xmd_sh256 	RLC_PREFIX(md_xmd_sh256)
#define md_xmd_sh384 	RLC_PREFIX(md_xmd_sh384)
#define md_xmd_sh512 	RLC_PREFIX(md_xmd_sh512)

#endif /* LABEL */

#endif /* !RLC_LABEL_H */
