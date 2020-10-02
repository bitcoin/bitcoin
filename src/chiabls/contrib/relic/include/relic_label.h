/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2017 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Symbol renaming to a#undef clashes when simultaneous linking multiple builds.
 *
 * @ingroup core
 */

#ifndef RELIC_LABEL_H
#define RELIC_LABEL_H

#include "relic_conf.h"

#define PREFIX(F)		_PREFIX(LABEL, F)
#define _PREFIX(A, B)		__PREFIX(A, B)
#define __PREFIX(A, B)		A ## _ ## B

/*============================================================================*/
/* Macro definitions                                                          */
/*============================================================================*/

#ifdef LABEL

#undef first_ctx
#define first_ctx	PREFIX(first_ctx)
#undef core_ctx
#define core_ctx	PREFIX(core_ctx)

#undef core_init
#undef core_clean
#undef core_get
#undef core_set

#define core_init 	PREFIX(core_init)
#define core_clean 	PREFIX(core_clean)
#define core_get 	PREFIX(core_get)
#define core_set 	PREFIX(core_set)

#undef arch_init
#undef arch_clean
#undef arch_cycles
#undef arch_copy_rom

#define arch_init 	PREFIX(arch_init)
#define arch_clean 	PREFIX(arch_clean)
#define arch_cycles 	PREFIX(arch_cycles)
#define arch_copy_rom 	PREFIX(arch_copy_rom)

#undef bench_overhead
#undef bench_reset
#undef bench_before
#undef bench_after
#undef bench_compute
#undef bench_print
#undef bench_total

#define bench_overhead 	PREFIX(bench_overhead)
#define bench_reset 	PREFIX(bench_reset)
#define bench_before 	PREFIX(bench_before)
#define bench_after 	PREFIX(bench_after)
#define bench_compute 	PREFIX(bench_compute)
#define bench_print 	PREFIX(bench_print)
#define bench_total 	PREFIX(bench_total)

#undef err_simple_msg
#undef err_full_msg
#undef err_get_msg
#undef err_get_code

#define err_simple_msg 	PREFIX(err_simple_msg)
#define err_full_msg 	PREFIX(err_full_msg)
#define err_get_msg 	PREFIX(err_get_msg)
#define err_get_code 	PREFIX(err_get_code)

#undef rand_init
#undef rand_clean
#undef rand_seed
#undef rand_seed
#undef rand_bytes

#define rand_init 	PREFIX(rand_init)
#define rand_clean 	PREFIX(rand_clean)
#define rand_seed 	PREFIX(rand_seed)
#define rand_seed 	PREFIX(rand_seed)
#define rand_bytes 	PREFIX(rand_bytes)

#undef pool_get
#undef pool_put

#define pool_get 	PREFIX(pool_get)
#define pool_put 	PREFIX(pool_put)

#undef test_fail
#undef test_pass

#define test_fail 	PREFIX(test_fail)
#define test_pass 	PREFIX(test_pass)

#undef trace_enter
#undef trace_exit

#define trace_enter 	PREFIX(trace_enter)
#define trace_exit 	PREFIX(trace_exit)

#undef util_conv_endian
#undef util_conv_big
#undef util_conv_little
#undef util_conv_char
#undef util_bits_dig
#undef util_cmp_const
#undef util_printf
#undef util_print_dig

#define util_conv_endian 	PREFIX(util_conv_endian)
#define util_conv_big 	PREFIX(util_conv_big)
#define util_conv_little 	PREFIX(util_conv_little)
#define util_conv_char 	PREFIX(util_conv_char)
#define util_bits_dig 	PREFIX(util_bits_dig)
#define util_cmp_const 	PREFIX(util_cmp_const)
#define util_printf 	PREFIX(util_printf)
#define util_print_dig 	PREFIX(util_print_dig)

#undef conf_print
#define conf_print       PREFIX(conf_print)

#undef dv_t
#define dv_t	PREFIX(dv_t)

#undef dv_print
#undef dv_zero
#undef dv_copy
#undef dv_copy_cond
#undef dv_swap_cond
#undef dv_cmp_const
#undef dv_new_dynam
#undef dv_new_statc
#undef dv_free_dynam
#undef dv_free_statc

#define dv_print 	PREFIX(dv_print)
#define dv_zero 	PREFIX(dv_zero)
#define dv_copy 	PREFIX(dv_copy)
#define dv_copy_cond 	PREFIX(dv_copy_cond)
#define dv_swap_cond 	PREFIX(dv_swap_cond)
#define dv_cmp_const 	PREFIX(dv_cmp_const)
#define dv_new_dynam 	PREFIX(dv_new_dynam)
#define dv_new_statc 	PREFIX(dv_new_statc)
#define dv_free_dynam 	PREFIX(dv_free_dynam)
#define dv_free_statc 	PREFIX(dv_free_statc)



#undef bn_st
#undef bn_t
#define bn_st	PREFIX(bn_st)
#define bn_t	PREFIX(bn_t)

#undef bn_init
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

#define bn_init 	PREFIX(bn_init)
#define bn_clean 	PREFIX(bn_clean)
#define bn_grow 	PREFIX(bn_grow)
#define bn_trim 	PREFIX(bn_trim)
#define bn_copy 	PREFIX(bn_copy)
#define bn_abs 	PREFIX(bn_abs)
#define bn_neg 	PREFIX(bn_neg)
#define bn_sign 	PREFIX(bn_sign)
#define bn_zero 	PREFIX(bn_zero)
#define bn_is_zero 	PREFIX(bn_is_zero)
#define bn_is_even 	PREFIX(bn_is_even)
#define bn_bits 	PREFIX(bn_bits)
#define bn_get_bit 	PREFIX(bn_get_bit)
#define bn_set_bit 	PREFIX(bn_set_bit)
#define bn_ham 	PREFIX(bn_ham)
#define bn_get_dig 	PREFIX(bn_get_dig)
#define bn_set_dig 	PREFIX(bn_set_dig)
#define bn_set_2b 	PREFIX(bn_set_2b)
#define bn_rand 	PREFIX(bn_rand)
#define bn_rand_mod 	PREFIX(bn_rand_mod)
#define bn_print 	PREFIX(bn_print)
#define bn_size_str 	PREFIX(bn_size_str)
#define bn_read_str 	PREFIX(bn_read_str)
#define bn_write_str 	PREFIX(bn_write_str)
#define bn_size_bin 	PREFIX(bn_size_bin)
#define bn_read_bin 	PREFIX(bn_read_bin)
#define bn_write_bin 	PREFIX(bn_write_bin)
#define bn_size_raw 	PREFIX(bn_size_raw)
#define bn_read_raw 	PREFIX(bn_read_raw)
#define bn_write_raw 	PREFIX(bn_write_raw)
#define bn_cmp_abs 	PREFIX(bn_cmp_abs)
#define bn_cmp_dig 	PREFIX(bn_cmp_dig)
#define bn_cmp 	PREFIX(bn_cmp)
#define bn_add 	PREFIX(bn_add)
#define bn_add_dig 	PREFIX(bn_add_dig)
#define bn_sub 	PREFIX(bn_sub)
#define bn_sub_dig 	PREFIX(bn_sub_dig)
#define bn_mul_dig 	PREFIX(bn_mul_dig)
#define bn_mul_basic 	PREFIX(bn_mul_basic)
#define bn_mul_comba 	PREFIX(bn_mul_comba)
#define bn_mul_karat 	PREFIX(bn_mul_karat)
#define bn_sqr_basic 	PREFIX(bn_sqr_basic)
#define bn_sqr_comba 	PREFIX(bn_sqr_comba)
#define bn_sqr_karat 	PREFIX(bn_sqr_karat)
#define bn_dbl 	PREFIX(bn_dbl)
#define bn_hlv 	PREFIX(bn_hlv)
#define bn_lsh 	PREFIX(bn_lsh)
#define bn_rsh 	PREFIX(bn_rsh)
#define bn_div 	PREFIX(bn_div)
#define bn_div_rem 	PREFIX(bn_div_rem)
#define bn_div_dig 	PREFIX(bn_div_dig)
#define bn_div_rem_dig 	PREFIX(bn_div_rem_dig)
#define bn_mod_2b 	PREFIX(bn_mod_2b)
#define bn_mod_dig 	PREFIX(bn_mod_dig)
#define bn_mod_basic 	PREFIX(bn_mod_basic)
#define bn_mod_pre_barrt 	PREFIX(bn_mod_pre_barrt)
#define bn_mod_barrt 	PREFIX(bn_mod_barrt)
#define bn_mod_pre_monty 	PREFIX(bn_mod_pre_monty)
#define bn_mod_monty_conv 	PREFIX(bn_mod_monty_conv)
#define bn_mod_monty_back 	PREFIX(bn_mod_monty_back)
#define bn_mod_monty_basic 	PREFIX(bn_mod_monty_basic)
#define bn_mod_monty_comba 	PREFIX(bn_mod_monty_comba)
#define bn_mod_pre_pmers 	PREFIX(bn_mod_pre_pmers)
#define bn_mod_pmers 	PREFIX(bn_mod_pmers)
#define bn_mxp_basic 	PREFIX(bn_mxp_basic)
#define bn_mxp_slide 	PREFIX(bn_mxp_slide)
#define bn_mxp_monty 	PREFIX(bn_mxp_monty)
#define bn_mxp_dig 	PREFIX(bn_mxp_dig)
#define bn_srt 	PREFIX(bn_srt)
#define bn_gcd_basic 	PREFIX(bn_gcd_basic)
#define bn_gcd_lehme 	PREFIX(bn_gcd_lehme)
#define bn_gcd_stein 	PREFIX(bn_gcd_stein)
#define bn_gcd_dig 	PREFIX(bn_gcd_dig)
#define bn_gcd_ext_basic 	PREFIX(bn_gcd_ext_basic)
#define bn_gcd_ext_lehme 	PREFIX(bn_gcd_ext_lehme)
#define bn_gcd_ext_stein 	PREFIX(bn_gcd_ext_stein)
#define bn_gcd_ext_mid 	PREFIX(bn_gcd_ext_mid)
#define bn_gcd_ext_dig 	PREFIX(bn_gcd_ext_dig)
#define bn_lcm 	PREFIX(bn_lcm)
#define bn_smb_leg 	PREFIX(bn_smb_leg)
#define bn_smb_jac 	PREFIX(bn_smb_jac)
#define bn_get_prime 	PREFIX(bn_get_prime)
#define bn_is_prime 	PREFIX(bn_is_prime)
#define bn_is_prime_basic 	PREFIX(bn_is_prime_basic)
#define bn_is_prime_rabin 	PREFIX(bn_is_prime_rabin)
#define bn_is_prime_solov 	PREFIX(bn_is_prime_solov)
#define bn_gen_prime_basic 	PREFIX(bn_gen_prime_basic)
#define bn_gen_prime_safep 	PREFIX(bn_gen_prime_safep)
#define bn_gen_prime_stron 	PREFIX(bn_gen_prime_stron)
#define bn_factor 	PREFIX(bn_factor)
#define bn_is_factor 	PREFIX(bn_is_factor)
#define bn_rec_win 	PREFIX(bn_rec_win)
#define bn_rec_slw 	PREFIX(bn_rec_slw)
#define bn_rec_naf 	PREFIX(bn_rec_naf)
#define bn_rec_tnaf 	PREFIX(bn_rec_tnaf)
#define bn_rec_rtnaf 	PREFIX(bn_rec_rtnaf)
#define bn_rec_tnaf_get 	PREFIX(bn_rec_tnaf_get)
#define bn_rec_tnaf_mod 	PREFIX(bn_rec_tnaf_mod)
#define bn_rec_reg 	PREFIX(bn_rec_reg)
#define bn_rec_jsf 	PREFIX(bn_rec_jsf)
#define bn_rec_glv 	PREFIX(bn_rec_glv)

#undef bn_add1_low
#undef bn_addn_low
#undef bn_sub1_low
#undef bn_subn_low
#undef bn_cmp1_low
#undef bn_cmpn_low
#undef bn_lsh1_low
#undef bn_lshb_low
#undef bn_lshd_low
#undef bn_rsh1_low
#undef bn_rshb_low
#undef bn_rshd_low
#undef bn_mula_low
#undef bn_mul1_low
#undef bn_muln_low
#undef bn_muld_low
#undef bn_sqra_low
#undef bn_sqrn_low
#undef bn_divn_low
#undef bn_div1_low
#undef bn_modn_low

#define bn_add1_low 	PREFIX(bn_add1_low)
#define bn_addn_low 	PREFIX(bn_addn_low)
#define bn_sub1_low 	PREFIX(bn_sub1_low)
#define bn_subn_low 	PREFIX(bn_subn_low)
#define bn_cmp1_low 	PREFIX(bn_cmp1_low)
#define bn_cmpn_low 	PREFIX(bn_cmpn_low)
#define bn_lsh1_low 	PREFIX(bn_lsh1_low)
#define bn_lshb_low 	PREFIX(bn_lshb_low)
#define bn_lshd_low 	PREFIX(bn_lshd_low)
#define bn_rsh1_low 	PREFIX(bn_rsh1_low)
#define bn_rshb_low 	PREFIX(bn_rshb_low)
#define bn_rshd_low 	PREFIX(bn_rshd_low)
#define bn_mula_low 	PREFIX(bn_mula_low)
#define bn_mul1_low 	PREFIX(bn_mul1_low)
#define bn_muln_low 	PREFIX(bn_muln_low)
#define bn_muld_low 	PREFIX(bn_muld_low)
#define bn_sqra_low 	PREFIX(bn_sqra_low)
#define bn_sqrn_low 	PREFIX(bn_sqrn_low)
#define bn_divn_low 	PREFIX(bn_divn_low)
#define bn_div1_low 	PREFIX(bn_div1_low)
#define bn_modn_low 	PREFIX(bn_modn_low)

#undef fp_st
#undef fp_t
#define fp_st	PREFIX(fp_st)
#define fp_t	PREFIX(fp_t)

#undef fp_prime_init
#undef fp_prime_clean
#undef fp_prime_get
#undef fp_prime_get_rdc
#undef fp_prime_get_conv
#undef fp_prime_get_mod8
#undef fp_prime_get_sps
#undef fp_prime_get_qnr
#undef fp_prime_get_cnr
#undef fp_param_get
#undef fp_prime_set_dense
#undef fp_prime_set_pmers
#undef fp_prime_calc
#undef fp_prime_conv
#undef fp_prime_conv_dig
#undef fp_prime_back
#undef fp_param_set
#undef fp_param_set_any
#undef fp_param_set_any_dense
#undef fp_param_set_any_pmers
#undef fp_param_set_any_tower
#undef fp_param_print
#undef fp_param_get_var
#undef fp_param_get_sps
#undef fp_param_get_map
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
#undef fp_inv_lower
#undef fp_inv_sim
#undef fp_exp_basic
#undef fp_exp_slide
#undef fp_exp_monty
#undef fp_srt

#define fp_prime_init 	PREFIX(fp_prime_init)
#define fp_prime_clean 	PREFIX(fp_prime_clean)
#define fp_prime_get 	PREFIX(fp_prime_get)
#define fp_prime_get_rdc 	PREFIX(fp_prime_get_rdc)
#define fp_prime_get_conv 	PREFIX(fp_prime_get_conv)
#define fp_prime_get_mod8 	PREFIX(fp_prime_get_mod8)
#define fp_prime_get_sps 	PREFIX(fp_prime_get_sps)
#define fp_prime_get_qnr 	PREFIX(fp_prime_get_qnr)
#define fp_prime_get_cnr 	PREFIX(fp_prime_get_cnr)
#define fp_param_get 	PREFIX(fp_param_get)
#define fp_prime_set_dense 	PREFIX(fp_prime_set_dense)
#define fp_prime_set_pmers 	PREFIX(fp_prime_set_pmers)
#define fp_prime_calc 	PREFIX(fp_prime_calc)
#define fp_prime_conv 	PREFIX(fp_prime_conv)
#define fp_prime_conv_dig 	PREFIX(fp_prime_conv_dig)
#define fp_prime_back 	PREFIX(fp_prime_back)
#define fp_param_set 	PREFIX(fp_param_set)
#define fp_param_set_any 	PREFIX(fp_param_set_any)
#define fp_param_set_any_dense 	PREFIX(fp_param_set_any_dense)
#define fp_param_set_any_pmers 	PREFIX(fp_param_set_any_pmers)
#define fp_param_set_any_tower 	PREFIX(fp_param_set_any_tower)
#define fp_param_print 	PREFIX(fp_param_print)
#define fp_param_get_var 	PREFIX(fp_param_get_var)
#define fp_param_get_sps 	PREFIX(fp_param_get_sps)
#define fp_param_get_map 	PREFIX(fp_param_get_map)
#define fp_copy 	PREFIX(fp_copy)
#define fp_zero 	PREFIX(fp_zero)
#define fp_is_zero 	PREFIX(fp_is_zero)
#define fp_is_even 	PREFIX(fp_is_even)
#define fp_get_bit 	PREFIX(fp_get_bit)
#define fp_set_bit 	PREFIX(fp_set_bit)
#define fp_set_dig 	PREFIX(fp_set_dig)
#define fp_bits 	PREFIX(fp_bits)
#define fp_rand 	PREFIX(fp_rand)
#define fp_print 	PREFIX(fp_print)
#define fp_size_str 	PREFIX(fp_size_str)
#define fp_read_str 	PREFIX(fp_read_str)
#define fp_write_str 	PREFIX(fp_write_str)
#define fp_read_bin 	PREFIX(fp_read_bin)
#define fp_write_bin 	PREFIX(fp_write_bin)
#define fp_cmp 	PREFIX(fp_cmp)
#define fp_cmp_dig 	PREFIX(fp_cmp_dig)
#define fp_add_basic 	PREFIX(fp_add_basic)
#define fp_add_integ 	PREFIX(fp_add_integ)
#define fp_add_dig 	PREFIX(fp_add_dig)
#define fp_sub_basic 	PREFIX(fp_sub_basic)
#define fp_sub_integ 	PREFIX(fp_sub_integ)
#define fp_sub_dig 	PREFIX(fp_sub_dig)
#define fp_neg_basic 	PREFIX(fp_neg_basic)
#define fp_neg_integ 	PREFIX(fp_neg_integ)
#define fp_dbl_basic 	PREFIX(fp_dbl_basic)
#define fp_dbl_integ 	PREFIX(fp_dbl_integ)
#define fp_hlv_basic 	PREFIX(fp_hlv_basic)
#define fp_hlv_integ 	PREFIX(fp_hlv_integ)
#define fp_mul_basic 	PREFIX(fp_mul_basic)
#define fp_mul_comba 	PREFIX(fp_mul_comba)
#define fp_mul_integ 	PREFIX(fp_mul_integ)
#define fp_mul_karat 	PREFIX(fp_mul_karat)
#define fp_mul_dig 	PREFIX(fp_mul_dig)
#define fp_sqr_basic 	PREFIX(fp_sqr_basic)
#define fp_sqr_comba 	PREFIX(fp_sqr_comba)
#define fp_sqr_integ 	PREFIX(fp_sqr_integ)
#define fp_sqr_karat 	PREFIX(fp_sqr_karat)
#define fp_lsh 	PREFIX(fp_lsh)
#define fp_rsh 	PREFIX(fp_rsh)
#define fp_rdc_basic 	PREFIX(fp_rdc_basic)
#define fp_rdc_monty_basic 	PREFIX(fp_rdc_monty_basic)
#define fp_rdc_monty_comba 	PREFIX(fp_rdc_monty_comba)
#define fp_rdc_quick 	PREFIX(fp_rdc_quick)
#define fp_inv_basic 	PREFIX(fp_inv_basic)
#define fp_inv_binar 	PREFIX(fp_inv_binar)
#define fp_inv_monty 	PREFIX(fp_inv_monty)
#define fp_inv_exgcd 	PREFIX(fp_inv_exgcd)
#define fp_inv_lower 	PREFIX(fp_inv_lower)
#define fp_inv_sim 	PREFIX(fp_inv_sim)
#define fp_exp_basic 	PREFIX(fp_exp_basic)
#define fp_exp_slide 	PREFIX(fp_exp_slide)
#define fp_exp_monty 	PREFIX(fp_exp_monty)
#define fp_srt 	PREFIX(fp_srt)

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
#undef fp_cmp1_low
#undef fp_cmpn_low
#undef fp_lsh1_low
#undef fp_lshb_low
#undef fp_lshd_low
#undef fp_rsh1_low
#undef fp_rshb_low
#undef fp_rshd_low
#undef fp_mula_low
#undef fp_mul1_low
#undef fp_muln_low
#undef fp_mulm_low
#undef fp_sqrn_low
#undef fp_sqrm_low
#undef fp_rdcs_low
#undef fp_rdcn_low
#undef fp_invn_low

#define fp_add1_low 	PREFIX(fp_add1_low)
#define fp_addn_low 	PREFIX(fp_addn_low)
#define fp_addm_low 	PREFIX(fp_addm_low)
#define fp_addd_low 	PREFIX(fp_addd_low)
#define fp_addc_low 	PREFIX(fp_addc_low)
#define fp_sub1_low 	PREFIX(fp_sub1_low)
#define fp_subn_low 	PREFIX(fp_subn_low)
#define fp_subm_low 	PREFIX(fp_subm_low)
#define fp_subd_low 	PREFIX(fp_subd_low)
#define fp_subc_low 	PREFIX(fp_subc_low)
#define fp_negm_low 	PREFIX(fp_negm_low)
#define fp_dbln_low 	PREFIX(fp_dbln_low)
#define fp_dblm_low 	PREFIX(fp_dblm_low)
#define fp_hlvm_low 	PREFIX(fp_hlvm_low)
#define fp_hlvd_low 	PREFIX(fp_hlvd_low)
#define fp_cmp1_low 	PREFIX(fp_cmp1_low)
#define fp_cmpn_low 	PREFIX(fp_cmpn_low)
#define fp_lsh1_low 	PREFIX(fp_lsh1_low)
#define fp_lshb_low 	PREFIX(fp_lshb_low)
#define fp_lshd_low 	PREFIX(fp_lshd_low)
#define fp_rsh1_low 	PREFIX(fp_rsh1_low)
#define fp_rshb_low 	PREFIX(fp_rshb_low)
#define fp_rshd_low 	PREFIX(fp_rshd_low)
#define fp_mula_low 	PREFIX(fp_mula_low)
#define fp_mul1_low 	PREFIX(fp_mul1_low)
#define fp_muln_low 	PREFIX(fp_muln_low)
#define fp_mulm_low 	PREFIX(fp_mulm_low)
#define fp_sqrn_low 	PREFIX(fp_sqrn_low)
#define fp_sqrm_low 	PREFIX(fp_sqrm_low)
#define fp_rdcs_low 	PREFIX(fp_rdcs_low)
#define fp_rdcn_low 	PREFIX(fp_rdcn_low)
#define fp_invn_low 	PREFIX(fp_invn_low)

#undef fp_st
#undef fp_t
#define fp_st	PREFIX(fp_st)
#define fp_t	PREFIX(fp_t)

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
#undef fb_poly_sub
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
#undef fb_sub
#undef fb_sub_dig
#undef fb_mul_basic
#undef fb_mul_integ
#undef fb_mul_lcomb
#undef fb_mul_rcomb
#undef fb_mul_lodah
#undef fb_mul_dig
#undef fb_mul_karat
#undef fb_sqr_basic
#undef fb_sqr_integ
#undef fb_sqr_lutbl
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

#define fb_poly_init 	PREFIX(fb_poly_init)
#define fb_poly_clean 	PREFIX(fb_poly_clean)
#define fb_poly_get 	PREFIX(fb_poly_get)
#define fb_poly_set_dense 	PREFIX(fb_poly_set_dense)
#define fb_poly_set_trino 	PREFIX(fb_poly_set_trino)
#define fb_poly_set_penta 	PREFIX(fb_poly_set_penta)
#define fb_poly_get_srz 	PREFIX(fb_poly_get_srz)
#define fb_poly_tab_srz 	PREFIX(fb_poly_tab_srz)
#define fb_poly_tab_sqr 	PREFIX(fb_poly_tab_sqr)
#define fb_poly_get_chain 	PREFIX(fb_poly_get_chain)
#define fb_poly_get_rdc 	PREFIX(fb_poly_get_rdc)
#define fb_poly_get_trc 	PREFIX(fb_poly_get_trc)
#define fb_poly_get_slv 	PREFIX(fb_poly_get_slv)
#define fb_param_set 	PREFIX(fb_param_set)
#define fb_param_set_any 	PREFIX(fb_param_set_any)
#define fb_param_print 	PREFIX(fb_param_print)
#define fb_poly_add 	PREFIX(fb_poly_add)
#define fb_poly_sub 	PREFIX(fb_poly_sub)
#define fb_copy 	PREFIX(fb_copy)
#define fb_neg 	PREFIX(fb_neg)
#define fb_zero 	PREFIX(fb_zero)
#define fb_is_zero 	PREFIX(fb_is_zero)
#define fb_get_bit 	PREFIX(fb_get_bit)
#define fb_set_bit 	PREFIX(fb_set_bit)
#define fb_set_dig 	PREFIX(fb_set_dig)
#define fb_bits 	PREFIX(fb_bits)
#define fb_rand 	PREFIX(fb_rand)
#define fb_print 	PREFIX(fb_print)
#define fb_size_str 	PREFIX(fb_size_str)
#define fb_read_str 	PREFIX(fb_read_str)
#define fb_write_str 	PREFIX(fb_write_str)
#define fb_read_bin 	PREFIX(fb_read_bin)
#define fb_write_bin 	PREFIX(fb_write_bin)
#define fb_cmp 	PREFIX(fb_cmp)
#define fb_cmp_dig 	PREFIX(fb_cmp_dig)
#define fb_add 	PREFIX(fb_add)
#define fb_add_dig 	PREFIX(fb_add_dig)
#define fb_sub 	PREFIX(fb_sub)
#define fb_sub_dig 	PREFIX(fb_sub_dig)
#define fb_mul_basic 	PREFIX(fb_mul_basic)
#define fb_mul_integ 	PREFIX(fb_mul_integ)
#define fb_mul_lcomb 	PREFIX(fb_mul_lcomb)
#define fb_mul_rcomb 	PREFIX(fb_mul_rcomb)
#define fb_mul_lodah 	PREFIX(fb_mul_lodah)
#define fb_mul_dig 	PREFIX(fb_mul_dig)
#define fb_mul_karat 	PREFIX(fb_mul_karat)
#define fb_sqr_basic 	PREFIX(fb_sqr_basic)
#define fb_sqr_integ 	PREFIX(fb_sqr_integ)
#define fb_sqr_lutbl 	PREFIX(fb_sqr_lutbl)
#define fb_lsh 	PREFIX(fb_lsh)
#define fb_rsh 	PREFIX(fb_rsh)
#define fb_rdc_basic 	PREFIX(fb_rdc_basic)
#define fb_rdc_quick 	PREFIX(fb_rdc_quick)
#define fb_srt_basic 	PREFIX(fb_srt_basic)
#define fb_srt_quick 	PREFIX(fb_srt_quick)
#define fb_trc_basic 	PREFIX(fb_trc_basic)
#define fb_trc_quick 	PREFIX(fb_trc_quick)
#define fb_inv_basic 	PREFIX(fb_inv_basic)
#define fb_inv_binar 	PREFIX(fb_inv_binar)
#define fb_inv_exgcd 	PREFIX(fb_inv_exgcd)
#define fb_inv_almos 	PREFIX(fb_inv_almos)
#define fb_inv_itoht 	PREFIX(fb_inv_itoht)
#define fb_inv_bruch 	PREFIX(fb_inv_bruch)
#define fb_inv_lower 	PREFIX(fb_inv_lower)
#define fb_inv_sim 	PREFIX(fb_inv_sim)
#define fb_exp_2b 	PREFIX(fb_exp_2b)
#define fb_exp_basic 	PREFIX(fb_exp_basic)
#define fb_exp_slide 	PREFIX(fb_exp_slide)
#define fb_exp_monty 	PREFIX(fb_exp_monty)
#define fb_slv_basic 	PREFIX(fb_slv_basic)
#define fb_slv_quick 	PREFIX(fb_slv_quick)
#define fb_itr_basic 	PREFIX(fb_itr_basic)
#define fb_itr_pre_quick 	PREFIX(fb_itr_pre_quick)
#define fb_itr_quick 	PREFIX(fb_itr_quick)

#undef fb_add1_low
#undef fb_addn_low
#undef fb_addd_low
#undef fb_cmp1_low
#undef fb_cmpn_low
#undef fb_lsh1_low
#undef fb_lshb_low
#undef fb_lshd_low
#undef fb_rsh1_low
#undef fb_rshb_low
#undef fb_rshd_low
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

#define fb_add1_low 	PREFIX(fb_add1_low)
#define fb_addn_low 	PREFIX(fb_addn_low)
#define fb_addd_low 	PREFIX(fb_addd_low)
#define fb_cmp1_low 	PREFIX(fb_cmp1_low)
#define fb_cmpn_low 	PREFIX(fb_cmpn_low)
#define fb_lsh1_low 	PREFIX(fb_lsh1_low)
#define fb_lshb_low 	PREFIX(fb_lshb_low)
#define fb_lshd_low 	PREFIX(fb_lshd_low)
#define fb_rsh1_low 	PREFIX(fb_rsh1_low)
#define fb_rshb_low 	PREFIX(fb_rshb_low)
#define fb_rshd_low 	PREFIX(fb_rshd_low)
#define fb_lsha_low 	PREFIX(fb_lsha_low)
#define fb_mul1_low 	PREFIX(fb_mul1_low)
#define fb_muln_low 	PREFIX(fb_muln_low)
#define fb_muld_low 	PREFIX(fb_muld_low)
#define fb_mulm_low 	PREFIX(fb_mulm_low)
#define fb_sqrn_low 	PREFIX(fb_sqrn_low)
#define fb_sqrl_low 	PREFIX(fb_sqrl_low)
#define fb_sqrm_low 	PREFIX(fb_sqrm_low)
#define fb_itrn_low 	PREFIX(fb_itrn_low)
#define fb_srtn_low 	PREFIX(fb_srtn_low)
#define fb_slvn_low 	PREFIX(fb_slvn_low)
#define fb_trcn_low 	PREFIX(fb_trcn_low)
#define fb_rdcn_low 	PREFIX(fb_rdcn_low)
#define fb_rdc1_low 	PREFIX(fb_rdc1_low)
#define fb_invn_low 	PREFIX(fb_invn_low)

#undef ep_st
#undef ep_t
#define ep_st	PREFIX(ep_st)
#define ep_t	PREFIX(ep_t)

#undef ep_curve_init
#undef ep_curve_clean
#undef ep_curve_get_a
#undef ep_curve_get_b
#undef ep_curve_get_beta
#undef ep_curve_get_v1
#undef ep_curve_get_v2
#undef ep_curve_opt_a
#undef ep_curve_opt_b
#undef ep_curve_is_endom
#undef ep_curve_is_super
#undef ep_curve_get_gen
#undef ep_curve_get_tab
#undef ep_curve_get_ord
#undef ep_curve_get_cof
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
#undef ep_rhs
#undef ep_is_valid
#undef ep_tab
#undef ep_print
#undef ep_size_bin
#undef ep_read_bin
#undef ep_write_bin
#undef ep_neg_basic
#undef ep_neg_projc
#undef ep_add_basic
#undef ep_add_slp_basic
#undef ep_add_projc
#undef ep_sub_basic
#undef ep_sub_projc
#undef ep_dbl_basic
#undef ep_dbl_slp_basic
#undef ep_dbl_projc
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
#undef ep_mul_sim_gen
#undef ep_norm
#undef ep_norm_sim
#undef ep_map
#undef ep_pck
#undef ep_upk

#define ep_curve_init 	PREFIX(ep_curve_init)
#define ep_curve_clean 	PREFIX(ep_curve_clean)
#define ep_curve_get_a 	PREFIX(ep_curve_get_a)
#define ep_curve_get_b 	PREFIX(ep_curve_get_b)
#define ep_curve_get_beta 	PREFIX(ep_curve_get_beta)
#define ep_curve_get_v1 	PREFIX(ep_curve_get_v1)
#define ep_curve_get_v2 	PREFIX(ep_curve_get_v2)
#define ep_curve_opt_a 	PREFIX(ep_curve_opt_a)
#define ep_curve_opt_b 	PREFIX(ep_curve_opt_b)
#define ep_curve_is_endom 	PREFIX(ep_curve_is_endom)
#define ep_curve_is_super 	PREFIX(ep_curve_is_super)
#define ep_curve_get_gen 	PREFIX(ep_curve_get_gen)
#define ep_curve_get_tab 	PREFIX(ep_curve_get_tab)
#define ep_curve_get_ord 	PREFIX(ep_curve_get_ord)
#define ep_curve_get_cof 	PREFIX(ep_curve_get_cof)
#define ep_curve_set_plain 	PREFIX(ep_curve_set_plain)
#define ep_curve_set_super 	PREFIX(ep_curve_set_super)
#define ep_curve_set_endom 	PREFIX(ep_curve_set_endom)
#define ep_param_set 	PREFIX(ep_param_set)
#define ep_param_set_any 	PREFIX(ep_param_set_any)
#define ep_param_set_any_plain 	PREFIX(ep_param_set_any_plain)
#define ep_param_set_any_endom 	PREFIX(ep_param_set_any_endom)
#define ep_param_set_any_super 	PREFIX(ep_param_set_any_super)
#define ep_param_set_any_pairf 	PREFIX(ep_param_set_any_pairf)
#define ep_param_get 	PREFIX(ep_param_get)
#define ep_param_print 	PREFIX(ep_param_print)
#define ep_param_level 	PREFIX(ep_param_level)
#define ep_param_embed 	PREFIX(ep_param_embed)
#define ep_is_infty 	PREFIX(ep_is_infty)
#define ep_set_infty 	PREFIX(ep_set_infty)
#define ep_copy 	PREFIX(ep_copy)
#define ep_cmp 	PREFIX(ep_cmp)
#define ep_rand 	PREFIX(ep_rand)
#define ep_rhs 	PREFIX(ep_rhs)
#define ep_is_valid 	PREFIX(ep_is_valid)
#define ep_tab 	PREFIX(ep_tab)
#define ep_print 	PREFIX(ep_print)
#define ep_size_bin 	PREFIX(ep_size_bin)
#define ep_read_bin 	PREFIX(ep_read_bin)
#define ep_write_bin 	PREFIX(ep_write_bin)
#define ep_neg_basic 	PREFIX(ep_neg_basic)
#define ep_neg_projc 	PREFIX(ep_neg_projc)
#define ep_add_basic 	PREFIX(ep_add_basic)
#define ep_add_slp_basic 	PREFIX(ep_add_slp_basic)
#define ep_add_projc 	PREFIX(ep_add_projc)
#define ep_sub_basic 	PREFIX(ep_sub_basic)
#define ep_sub_projc 	PREFIX(ep_sub_projc)
#define ep_dbl_basic 	PREFIX(ep_dbl_basic)
#define ep_dbl_slp_basic 	PREFIX(ep_dbl_slp_basic)
#define ep_dbl_projc 	PREFIX(ep_dbl_projc)
#define ep_mul_basic 	PREFIX(ep_mul_basic)
#define ep_mul_slide 	PREFIX(ep_mul_slide)
#define ep_mul_monty 	PREFIX(ep_mul_monty)
#define ep_mul_lwnaf 	PREFIX(ep_mul_lwnaf)
#define ep_mul_lwreg 	PREFIX(ep_mul_lwreg)
#define ep_mul_gen 	PREFIX(ep_mul_gen)
#define ep_mul_dig 	PREFIX(ep_mul_dig)
#define ep_mul_pre_basic 	PREFIX(ep_mul_pre_basic)
#define ep_mul_pre_yaowi 	PREFIX(ep_mul_pre_yaowi)
#define ep_mul_pre_nafwi 	PREFIX(ep_mul_pre_nafwi)
#define ep_mul_pre_combs 	PREFIX(ep_mul_pre_combs)
#define ep_mul_pre_combd 	PREFIX(ep_mul_pre_combd)
#define ep_mul_pre_lwnaf 	PREFIX(ep_mul_pre_lwnaf)
#define ep_mul_fix_basic 	PREFIX(ep_mul_fix_basic)
#define ep_mul_fix_yaowi 	PREFIX(ep_mul_fix_yaowi)
#define ep_mul_fix_nafwi 	PREFIX(ep_mul_fix_nafwi)
#define ep_mul_fix_combs 	PREFIX(ep_mul_fix_combs)
#define ep_mul_fix_combd 	PREFIX(ep_mul_fix_combd)
#define ep_mul_fix_lwnaf 	PREFIX(ep_mul_fix_lwnaf)
#define ep_mul_sim_basic 	PREFIX(ep_mul_sim_basic)
#define ep_mul_sim_trick 	PREFIX(ep_mul_sim_trick)
#define ep_mul_sim_inter 	PREFIX(ep_mul_sim_inter)
#define ep_mul_sim_joint 	PREFIX(ep_mul_sim_joint)
#define ep_mul_sim_gen 	PREFIX(ep_mul_sim_gen)
#define ep_norm 	PREFIX(ep_norm)
#define ep_norm_sim 	PREFIX(ep_norm_sim)
#define ep_map 	PREFIX(ep_map)
#define ep_pck 	PREFIX(ep_pck)
#define ep_upk 	PREFIX(ep_upk)

#undef ed_st
#undef ed_t
#define ed_st	PREFIX(ed_st)
#define ed_t	PREFIX(ed_t)

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
#undef ed_copy
#undef ed_cmp
#undef ed_set_infty
#undef ed_is_infty
#undef ed_neg
#undef ed_add
#undef ed_sub
#undef ed_dbl
#undef ed_dbl_short
#undef ed_norm
#undef ed_norm_sim
#undef ed_map
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
#undef ed_is_valid
#undef ed_size_bin
#undef ed_read_bin
#undef ed_write_bin
#undef ed_mul_basic
#undef ed_mul_slide
#undef ed_mul_monty
#undef ed_mul_fixed
#undef ed_mul_lwnaf
#undef ed_mul_lwnaf_mixed
#undef ed_mul_lwreg
#undef ed_pck
#undef ed_upk

#define ed_param_set 	PREFIX(ed_param_set)
#define ed_param_set_any 	PREFIX(ed_param_set_any)
#define ed_param_get 	PREFIX(ed_param_get)
#define ed_curve_get_ord 	PREFIX(ed_curve_get_ord)
#define ed_curve_get_gen 	PREFIX(ed_curve_get_gen)
#define ed_curve_get_tab 	PREFIX(ed_curve_get_tab)
#define ed_curve_get_cof 	PREFIX(ed_curve_get_cof)
#define ed_param_print 	PREFIX(ed_param_print)
#define ed_param_level 	PREFIX(ed_param_level)
#define ed_projc_to_extnd 	PREFIX(ed_projc_to_extnd)
#define ed_rand 	PREFIX(ed_rand)
#define ed_copy 	PREFIX(ed_copy)
#define ed_cmp 	PREFIX(ed_cmp)
#define ed_set_infty 	PREFIX(ed_set_infty)
#define ed_is_infty 	PREFIX(ed_is_infty)
#define ed_neg 	PREFIX(ed_neg)
#define ed_add 	PREFIX(ed_add)
#define ed_sub 	PREFIX(ed_sub)
#define ed_dbl 	PREFIX(ed_dbl)
#define ed_dbl_short 	PREFIX(ed_dbl_short)
#define ed_norm 	PREFIX(ed_norm)
#define ed_norm_sim 	PREFIX(ed_norm_sim)
#define ed_map 	PREFIX(ed_map)
#define ed_curve_init 	PREFIX(ed_curve_init)
#define ed_curve_clean 	PREFIX(ed_curve_clean)
#define ed_mul_pre_basic 	PREFIX(ed_mul_pre_basic)
#define ed_mul_pre_yaowi 	PREFIX(ed_mul_pre_yaowi)
#define ed_mul_pre_nafwi 	PREFIX(ed_mul_pre_nafwi)
#define ed_mul_pre_combs 	PREFIX(ed_mul_pre_combs)
#define ed_mul_pre_combd 	PREFIX(ed_mul_pre_combd)
#define ed_mul_pre_lwnaf 	PREFIX(ed_mul_pre_lwnaf)
#define ed_mul_fix_basic 	PREFIX(ed_mul_fix_basic)
#define ed_mul_fix_yaowi 	PREFIX(ed_mul_fix_yaowi)
#define ed_mul_fix_nafwi 	PREFIX(ed_mul_fix_nafwi)
#define ed_mul_fix_combs 	PREFIX(ed_mul_fix_combs)
#define ed_mul_fix_combd 	PREFIX(ed_mul_fix_combd)
#define ed_mul_fix_lwnaf 	PREFIX(ed_mul_fix_lwnaf)
#define ed_mul_fix_lwnaf_mixed 	PREFIX(ed_mul_fix_lwnaf_mixed)
#define ed_mul_gen 	PREFIX(ed_mul_gen)
#define ed_mul_dig 	PREFIX(ed_mul_dig)
#define ed_mul_sim_basic 	PREFIX(ed_mul_sim_basic)
#define ed_mul_sim_trick 	PREFIX(ed_mul_sim_trick)
#define ed_mul_sim_inter 	PREFIX(ed_mul_sim_inter)
#define ed_mul_sim_joint 	PREFIX(ed_mul_sim_joint)
#define ed_mul_sim_gen 	PREFIX(ed_mul_sim_gen)
#define ed_tab 	PREFIX(ed_tab)
#define ed_print 	PREFIX(ed_print)
#define ed_is_valid 	PREFIX(ed_is_valid)
#define ed_size_bin 	PREFIX(ed_size_bin)
#define ed_read_bin 	PREFIX(ed_read_bin)
#define ed_write_bin 	PREFIX(ed_write_bin)
#define ed_mul_basic 	PREFIX(ed_mul_basic)
#define ed_mul_slide 	PREFIX(ed_mul_slide)
#define ed_mul_monty 	PREFIX(ed_mul_monty)
#define ed_mul_fixed 	PREFIX(ed_mul_fixed)
#define ed_mul_lwnaf 	PREFIX(ed_mul_lwnaf)
#define ed_mul_lwnaf_mixed 	PREFIX(ed_mul_lwnaf_mixed)
#define ed_mul_lwreg 	PREFIX(ed_mul_lwreg)
#define ed_pck 	PREFIX(ed_pck)
#define ed_upk 	PREFIX(ed_upk)

#undef eb_st
#undef eb_t
#define eb_st	PREFIX(eb_st)
#define eb_t	PREFIX(eb_t)

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
#undef eb_rhs
#undef eb_is_valid
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
#undef eb_frb_basic
#undef eb_frb_projc
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

#define eb_curve_init 	PREFIX(eb_curve_init)
#define eb_curve_clean 	PREFIX(eb_curve_clean)
#define eb_curve_get_a 	PREFIX(eb_curve_get_a)
#define eb_curve_get_b 	PREFIX(eb_curve_get_b)
#define eb_curve_opt_a 	PREFIX(eb_curve_opt_a)
#define eb_curve_opt_b 	PREFIX(eb_curve_opt_b)
#define eb_curve_is_kbltz 	PREFIX(eb_curve_is_kbltz)
#define eb_curve_get_gen 	PREFIX(eb_curve_get_gen)
#define eb_curve_get_tab 	PREFIX(eb_curve_get_tab)
#define eb_curve_get_ord 	PREFIX(eb_curve_get_ord)
#define eb_curve_get_cof 	PREFIX(eb_curve_get_cof)
#define eb_curve_set 	PREFIX(eb_curve_set)
#define eb_param_set 	PREFIX(eb_param_set)
#define eb_param_set_any 	PREFIX(eb_param_set_any)
#define eb_param_set_any_plain 	PREFIX(eb_param_set_any_plain)
#define eb_param_set_any_kbltz 	PREFIX(eb_param_set_any_kbltz)
#define eb_param_get 	PREFIX(eb_param_get)
#define eb_param_print 	PREFIX(eb_param_print)
#define eb_param_level 	PREFIX(eb_param_level)
#define eb_is_infty 	PREFIX(eb_is_infty)
#define eb_set_infty 	PREFIX(eb_set_infty)
#define eb_copy 	PREFIX(eb_copy)
#define eb_cmp 	PREFIX(eb_cmp)
#define eb_rand 	PREFIX(eb_rand)
#define eb_rhs 	PREFIX(eb_rhs)
#define eb_is_valid 	PREFIX(eb_is_valid)
#define eb_tab 	PREFIX(eb_tab)
#define eb_print 	PREFIX(eb_print)
#define eb_size_bin 	PREFIX(eb_size_bin)
#define eb_read_bin 	PREFIX(eb_read_bin)
#define eb_write_bin 	PREFIX(eb_write_bin)
#define eb_neg_basic 	PREFIX(eb_neg_basic)
#define eb_neg_projc 	PREFIX(eb_neg_projc)
#define eb_add_basic 	PREFIX(eb_add_basic)
#define eb_add_projc 	PREFIX(eb_add_projc)
#define eb_sub_basic 	PREFIX(eb_sub_basic)
#define eb_sub_projc 	PREFIX(eb_sub_projc)
#define eb_dbl_basic 	PREFIX(eb_dbl_basic)
#define eb_dbl_projc 	PREFIX(eb_dbl_projc)
#define eb_hlv 	PREFIX(eb_hlv)
#define eb_frb_basic 	PREFIX(eb_frb_basic)
#define eb_frb_projc 	PREFIX(eb_frb_projc)
#define eb_mul_basic 	PREFIX(eb_mul_basic)
#define eb_mul_lodah 	PREFIX(eb_mul_lodah)
#define eb_mul_lwnaf 	PREFIX(eb_mul_lwnaf)
#define eb_mul_rwnaf 	PREFIX(eb_mul_rwnaf)
#define eb_mul_halve 	PREFIX(eb_mul_halve)
#define eb_mul_gen 	PREFIX(eb_mul_gen)
#define eb_mul_dig 	PREFIX(eb_mul_dig)
#define eb_mul_pre_basic 	PREFIX(eb_mul_pre_basic)
#define eb_mul_pre_yaowi 	PREFIX(eb_mul_pre_yaowi)
#define eb_mul_pre_nafwi 	PREFIX(eb_mul_pre_nafwi)
#define eb_mul_pre_combs 	PREFIX(eb_mul_pre_combs)
#define eb_mul_pre_combd 	PREFIX(eb_mul_pre_combd)
#define eb_mul_pre_lwnaf 	PREFIX(eb_mul_pre_lwnaf)
#define eb_mul_fix_basic 	PREFIX(eb_mul_fix_basic)
#define eb_mul_fix_yaowi 	PREFIX(eb_mul_fix_yaowi)
#define eb_mul_fix_nafwi 	PREFIX(eb_mul_fix_nafwi)
#define eb_mul_fix_combs 	PREFIX(eb_mul_fix_combs)
#define eb_mul_fix_combd 	PREFIX(eb_mul_fix_combd)
#define eb_mul_fix_lwnaf 	PREFIX(eb_mul_fix_lwnaf)
#define eb_mul_sim_basic 	PREFIX(eb_mul_sim_basic)
#define eb_mul_sim_trick 	PREFIX(eb_mul_sim_trick)
#define eb_mul_sim_inter 	PREFIX(eb_mul_sim_inter)
#define eb_mul_sim_joint 	PREFIX(eb_mul_sim_joint)
#define eb_mul_sim_gen 	PREFIX(eb_mul_sim_gen)
#define eb_norm 	PREFIX(eb_norm)
#define eb_norm_sim 	PREFIX(eb_norm_sim)
#define eb_map 	PREFIX(eb_map)
#define eb_pck 	PREFIX(eb_pck)
#define eb_upk 	PREFIX(eb_upk)

#undef ep2_st
#undef ep2_t
#define ep2_st	PREFIX(ep2_st)
#define ep2_t		PREFIX(ep2_t)

#undef ep2_curve_init
#undef ep2_curve_clean
#undef ep2_curve_get_a
#undef ep2_curve_get_b
#undef ep2_curve_get_vs
#undef ep2_curve_opt_a
#undef ep2_curve_is_twist
#undef ep2_curve_get_gen
#undef ep2_curve_get_tab
#undef ep2_curve_get_ord
#undef ep2_curve_get_cof
#undef ep2_curve_set
#undef ep2_curve_set_twist
#undef ep2_is_infty
#undef ep2_set_infty
#undef ep2_copy
#undef ep2_cmp
#undef ep2_rand
#undef ep2_rhs
#undef ep2_is_valid
#undef ep2_tab
#undef ep2_print
#undef ep2_size_bin
#undef ep2_read_bin
#undef ep2_write_bin
#undef ep2_neg_basic
#undef ep2_neg_projc
#undef ep2_add_basic
#undef ep2_add_slp_basic
#undef ep2_sub_basic
#undef ep2_add_projc
#undef ep2_sub_projc
#undef ep2_dbl_basic
#undef ep2_dbl_slp_basic
#undef ep2_dbl_projc
#undef ep2_mul_basic
#undef ep2_mul_slide
#undef ep2_mul_monty
#undef ep2_mul_lwnaf
#undef ep2_mul_lwreg
#undef ep2_mul_gen
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
#undef ep2_mul_sim_gen
#undef ep2_mul_dig
#undef ep2_norm
#undef ep2_norm_sim
#undef ep2_map
#undef ep2_frb
#undef ep2_pck
#undef ep2_upk

#define ep2_curve_init 	PREFIX(ep2_curve_init)
#define ep2_curve_clean 	PREFIX(ep2_curve_clean)
#define ep2_curve_get_a 	PREFIX(ep2_curve_get_a)
#define ep2_curve_get_b 	PREFIX(ep2_curve_get_b)
#define ep2_curve_get_vs 	PREFIX(ep2_curve_get_vs)
#define ep2_curve_opt_a 	PREFIX(ep2_curve_opt_a)
#define ep2_curve_is_twist 	PREFIX(ep2_curve_is_twist)
#define ep2_curve_get_gen 	PREFIX(ep2_curve_get_gen)
#define ep2_curve_get_tab 	PREFIX(ep2_curve_get_tab)
#define ep2_curve_get_ord 	PREFIX(ep2_curve_get_ord)
#define ep2_curve_get_cof 	PREFIX(ep2_curve_get_cof)
#define ep2_curve_set 	PREFIX(ep2_curve_set)
#define ep2_curve_set_twist 	PREFIX(ep2_curve_set_twist)
#define ep2_is_infty 	PREFIX(ep2_is_infty)
#define ep2_set_infty 	PREFIX(ep2_set_infty)
#define ep2_copy 	PREFIX(ep2_copy)
#define ep2_cmp 	PREFIX(ep2_cmp)
#define ep2_rand 	PREFIX(ep2_rand)
#define ep2_rhs 	PREFIX(ep2_rhs)
#define ep2_is_valid 	PREFIX(ep2_is_valid)
#define ep2_tab 	PREFIX(ep2_tab)
#define ep2_print 	PREFIX(ep2_print)
#define ep2_size_bin 	PREFIX(ep2_size_bin)
#define ep2_read_bin 	PREFIX(ep2_read_bin)
#define ep2_write_bin 	PREFIX(ep2_write_bin)
#define ep2_neg_basic 	PREFIX(ep2_neg_basic)
#define ep2_neg_projc 	PREFIX(ep2_neg_projc)
#define ep2_add_basic 	PREFIX(ep2_add_basic)
#define ep2_add_slp_basic 	PREFIX(ep2_add_slp_basic)
#define ep2_sub_basic 	PREFIX(ep2_sub_basic)
#define ep2_add_projc 	PREFIX(ep2_add_projc)
#define ep2_sub_projc 	PREFIX(ep2_sub_projc)
#define ep2_dbl_basic 	PREFIX(ep2_dbl_basic)
#define ep2_dbl_slp_basic 	PREFIX(ep2_dbl_slp_basic)
#define ep2_dbl_projc 	PREFIX(ep2_dbl_projc)
#define ep2_mul_basic 	PREFIX(ep2_mul_basic)
#define ep2_mul_slide 	PREFIX(ep2_mul_slide)
#define ep2_mul_monty 	PREFIX(ep2_mul_monty)
#define ep2_mul_lwnaf 	PREFIX(ep2_mul_lwnaf)
#define ep2_mul_lwreg 	PREFIX(ep2_mul_lwreg)
#define ep2_mul_gen 	PREFIX(ep2_mul_gen)
#define ep2_mul_pre_basic 	PREFIX(ep2_mul_pre_basic)
#define ep2_mul_pre_yaowi 	PREFIX(ep2_mul_pre_yaowi)
#define ep2_mul_pre_nafwi 	PREFIX(ep2_mul_pre_nafwi)
#define ep2_mul_pre_combs 	PREFIX(ep2_mul_pre_combs)
#define ep2_mul_pre_combd 	PREFIX(ep2_mul_pre_combd)
#define ep2_mul_pre_lwnaf 	PREFIX(ep2_mul_pre_lwnaf)
#define ep2_mul_fix_basic 	PREFIX(ep2_mul_fix_basic)
#define ep2_mul_fix_yaowi 	PREFIX(ep2_mul_fix_yaowi)
#define ep2_mul_fix_nafwi 	PREFIX(ep2_mul_fix_nafwi)
#define ep2_mul_fix_combs 	PREFIX(ep2_mul_fix_combs)
#define ep2_mul_fix_combd 	PREFIX(ep2_mul_fix_combd)
#define ep2_mul_fix_lwnaf 	PREFIX(ep2_mul_fix_lwnaf)
#define ep2_mul_sim_basic 	PREFIX(ep2_mul_sim_basic)
#define ep2_mul_sim_trick 	PREFIX(ep2_mul_sim_trick)
#define ep2_mul_sim_inter 	PREFIX(ep2_mul_sim_inter)
#define ep2_mul_sim_joint 	PREFIX(ep2_mul_sim_joint)
#define ep2_mul_sim_gen 	PREFIX(ep2_mul_sim_gen)
#define ep2_mul_dig 	PREFIX(ep2_mul_dig)
#define ep2_norm 	PREFIX(ep2_norm)
#define ep2_norm_sim 	PREFIX(ep2_norm_sim)
#define ep2_map 	PREFIX(ep2_map)
#define ep2_frb 	PREFIX(ep2_frb)
#define ep2_pck 	PREFIX(ep2_pck)
#define ep2_upk 	PREFIX(ep2_upk)

#undef fp2_st
#undef fp2_t
#undef dv2_t
#undef fp3_st
#undef fp3_t
#undef dv3_t
#undef fp6_st
#undef fp6_t
#undef dv6_t
#undef fp12_t
#undef fp18_t

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
#undef fp2_sqr_basic
#undef fp2_sqr_integ
#undef fp2_inv
#undef fp2_inv_uni
#undef fp2_inv_sim
#undef fp2_test_uni
#undef fp2_conv_uni
#undef fp2_exp
#undef fp2_exp_uni
#undef fp2_frb
#undef fp2_srt
#undef fp2_pck
#undef fp2_upk

#define fp2_copy 	PREFIX(fp2_copy)
#define fp2_zero 	PREFIX(fp2_zero)
#define fp2_is_zero 	PREFIX(fp2_is_zero)
#define fp2_rand 	PREFIX(fp2_rand)
#define fp2_print 	PREFIX(fp2_print)
#define fp2_size_bin 	PREFIX(fp2_size_bin)
#define fp2_read_bin 	PREFIX(fp2_read_bin)
#define fp2_write_bin 	PREFIX(fp2_write_bin)
#define fp2_cmp 	PREFIX(fp2_cmp)
#define fp2_cmp_dig 	PREFIX(fp2_cmp_dig)
#define fp2_set_dig 	PREFIX(fp2_set_dig)
#define fp2_add_basic 	PREFIX(fp2_add_basic)
#define fp2_add_integ 	PREFIX(fp2_add_integ)
#define fp2_sub_basic 	PREFIX(fp2_sub_basic)
#define fp2_sub_integ 	PREFIX(fp2_sub_integ)
#define fp2_neg 	PREFIX(fp2_neg)
#define fp2_dbl_basic 	PREFIX(fp2_dbl_basic)
#define fp2_dbl_integ 	PREFIX(fp2_dbl_integ)
#define fp2_mul_basic 	PREFIX(fp2_mul_basic)
#define fp2_mul_integ 	PREFIX(fp2_mul_integ)
#define fp2_mul_art 	PREFIX(fp2_mul_art)
#define fp2_mul_nor_basic 	PREFIX(fp2_mul_nor_basic)
#define fp2_mul_nor_integ 	PREFIX(fp2_mul_nor_integ)
#define fp2_mul_frb 	PREFIX(fp2_mul_frb)
#define fp2_sqr_basic 	PREFIX(fp2_sqr_basic)
#define fp2_sqr_integ 	PREFIX(fp2_sqr_integ)
#define fp2_inv 	PREFIX(fp2_inv)
#define fp2_inv_uni 	PREFIX(fp2_inv_uni)
#define fp2_inv_sim 	PREFIX(fp2_inv_sim)
#define fp2_test_uni 	PREFIX(fp2_test_uni)
#define fp2_conv_uni 	PREFIX(fp2_conv_uni)
#define fp2_exp 	PREFIX(fp2_exp)
#define fp2_exp_uni 	PREFIX(fp2_exp_uni)
#define fp2_frb 	PREFIX(fp2_frb)
#define fp2_srt 	PREFIX(fp2_srt)
#define fp2_pck 	PREFIX(fp2_pck)
#define fp2_upk 	PREFIX(fp2_upk)

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

#define fp2_addn_low 	PREFIX(fp2_addn_low)
#define fp2_addm_low 	PREFIX(fp2_addm_low)
#define fp2_addd_low 	PREFIX(fp2_addd_low)
#define fp2_addc_low 	PREFIX(fp2_addc_low)
#define fp2_subn_low 	PREFIX(fp2_subn_low)
#define fp2_subm_low 	PREFIX(fp2_subm_low)
#define fp2_subd_low 	PREFIX(fp2_subd_low)
#define fp2_subc_low 	PREFIX(fp2_subc_low)
#define fp2_dbln_low 	PREFIX(fp2_dbln_low)
#define fp2_dblm_low 	PREFIX(fp2_dblm_low)
#define fp2_norm_low 	PREFIX(fp2_norm_low)
#define fp2_norh_low 	PREFIX(fp2_norh_low)
#define fp2_nord_low 	PREFIX(fp2_nord_low)
#define fp2_muln_low 	PREFIX(fp2_muln_low)
#define fp2_mulc_low 	PREFIX(fp2_mulc_low)
#define fp2_mulm_low 	PREFIX(fp2_mulm_low)
#define fp2_sqrn_low 	PREFIX(fp2_sqrn_low)
#define fp2_sqrm_low 	PREFIX(fp2_sqrm_low)
#define fp2_rdcn_low 	PREFIX(fp2_rdcn_low)

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
#undef fp3_mul_art
#undef fp3_mul_nor_basic
#undef fp3_mul_nor_integ
#undef fp3_mul_frb
#undef fp3_sqr_basic
#undef fp3_sqr_integ
#undef fp3_inv
#undef fp3_inv_sim
#undef fp3_exp
#undef fp3_frb
#undef fp3_srt

#define fp3_copy 	PREFIX(fp3_copy)
#define fp3_zero 	PREFIX(fp3_zero)
#define fp3_is_zero 	PREFIX(fp3_is_zero)
#define fp3_rand 	PREFIX(fp3_rand)
#define fp3_print 	PREFIX(fp3_print)
#define fp3_size_bin 	PREFIX(fp3_size_bin)
#define fp3_read_bin 	PREFIX(fp3_read_bin)
#define fp3_write_bin 	PREFIX(fp3_write_bin)
#define fp3_cmp 	PREFIX(fp3_cmp)
#define fp3_cmp_dig 	PREFIX(fp3_cmp_dig)
#define fp3_set_dig 	PREFIX(fp3_set_dig)
#define fp3_add_basic 	PREFIX(fp3_add_basic)
#define fp3_add_integ 	PREFIX(fp3_add_integ)
#define fp3_sub_basic 	PREFIX(fp3_sub_basic)
#define fp3_sub_integ 	PREFIX(fp3_sub_integ)
#define fp3_neg 	PREFIX(fp3_neg)
#define fp3_dbl_basic 	PREFIX(fp3_dbl_basic)
#define fp3_dbl_integ 	PREFIX(fp3_dbl_integ)
#define fp3_mul_basic 	PREFIX(fp3_mul_basic)
#define fp3_mul_integ 	PREFIX(fp3_mul_integ)
#define fp3_mul_art 	PREFIX(fp3_mul_art)
#define fp3_mul_nor_basic 	PREFIX(fp3_mul_nor_basic)
#define fp3_mul_nor_integ 	PREFIX(fp3_mul_nor_integ)
#define fp3_mul_frb 	PREFIX(fp3_mul_frb)
#define fp3_sqr_basic 	PREFIX(fp3_sqr_basic)
#define fp3_sqr_integ 	PREFIX(fp3_sqr_integ)
#define fp3_inv 	PREFIX(fp3_inv)
#define fp3_inv_sim 	PREFIX(fp3_inv_sim)
#define fp3_exp 	PREFIX(fp3_exp)
#define fp3_frb 	PREFIX(fp3_frb)
#define fp3_srt 	PREFIX(fp3_srt)

#undef fp3_addn_low
#undef fp3_addm_low
#undef fp3_subn_low
#undef fp3_subm_low
#undef fp3_subc_low
#undef fp3_dbln_low
#undef fp3_dblm_low
#undef fp3_muln_low
#undef fp3_mulm_low
#undef fp3_sqrn_low
#undef fp3_sqrm_low
#undef fp3_rdcn_low

#define fp3_addn_low 	PREFIX(fp3_addn_low)
#define fp3_addm_low 	PREFIX(fp3_addm_low)
#define fp3_subn_low 	PREFIX(fp3_subn_low)
#define fp3_subm_low 	PREFIX(fp3_subm_low)
#define fp3_subc_low 	PREFIX(fp3_subc_low)
#define fp3_dbln_low 	PREFIX(fp3_dbln_low)
#define fp3_dblm_low 	PREFIX(fp3_dblm_low)
#define fp3_muln_low 	PREFIX(fp3_muln_low)
#define fp3_mulm_low 	PREFIX(fp3_mulm_low)
#define fp3_sqrn_low 	PREFIX(fp3_sqrn_low)
#define fp3_sqrm_low 	PREFIX(fp3_sqrm_low)
#define fp3_rdcn_low 	PREFIX(fp3_rdcn_low)

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

#define fp6_copy 	PREFIX(fp6_copy)
#define fp6_zero 	PREFIX(fp6_zero)
#define fp6_is_zero 	PREFIX(fp6_is_zero)
#define fp6_rand 	PREFIX(fp6_rand)
#define fp6_print 	PREFIX(fp6_print)
#define fp6_size_bin 	PREFIX(fp6_size_bin)
#define fp6_read_bin 	PREFIX(fp6_read_bin)
#define fp6_write_bin 	PREFIX(fp6_write_bin)
#define fp6_cmp 	PREFIX(fp6_cmp)
#define fp6_cmp_dig 	PREFIX(fp6_cmp_dig)
#define fp6_set_dig 	PREFIX(fp6_set_dig)
#define fp6_add 	PREFIX(fp6_add)
#define fp6_sub 	PREFIX(fp6_sub)
#define fp6_neg 	PREFIX(fp6_neg)
#define fp6_dbl 	PREFIX(fp6_dbl)
#define fp6_mul_unr 	PREFIX(fp6_mul_unr)
#define fp6_mul_basic 	PREFIX(fp6_mul_basic)
#define fp6_mul_lazyr 	PREFIX(fp6_mul_lazyr)
#define fp6_mul_art 	PREFIX(fp6_mul_art)
#define fp6_mul_dxs 	PREFIX(fp6_mul_dxs)
#define fp6_sqr_unr 	PREFIX(fp6_sqr_unr)
#define fp6_sqr_basic 	PREFIX(fp6_sqr_basic)
#define fp6_sqr_lazyr 	PREFIX(fp6_sqr_lazyr)
#define fp6_inv 	PREFIX(fp6_inv)
#define fp6_exp 	PREFIX(fp6_exp)
#define fp6_frb 	PREFIX(fp6_frb)

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
#undef fp12_mul_unr
#undef fp12_mul_basic
#undef fp12_mul_lazyr
#undef fp12_mul_dxs_basic
#undef fp12_mul_dxs_lazyr
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
#undef fp12_inv_uni
#undef fp12_conv_uni
#undef fp12_frb
#undef fp12_exp
#undef fp12_exp_cyc
#undef fp12_exp_cyc_sps
#undef fp12_pck
#undef fp12_upk

#define fp12_copy 	PREFIX(fp12_copy)
#define fp12_zero 	PREFIX(fp12_zero)
#define fp12_is_zero 	PREFIX(fp12_is_zero)
#define fp12_rand 	PREFIX(fp12_rand)
#define fp12_print 	PREFIX(fp12_print)
#define fp12_size_bin 	PREFIX(fp12_size_bin)
#define fp12_read_bin 	PREFIX(fp12_read_bin)
#define fp12_write_bin 	PREFIX(fp12_write_bin)
#define fp12_cmp 	PREFIX(fp12_cmp)
#define fp12_cmp_dig 	PREFIX(fp12_cmp_dig)
#define fp12_set_dig 	PREFIX(fp12_set_dig)
#define fp12_add 	PREFIX(fp12_add)
#define fp12_sub 	PREFIX(fp12_sub)
#define fp12_neg 	PREFIX(fp12_neg)
#define fp12_mul_unr 	PREFIX(fp12_mul_unr)
#define fp12_mul_basic 	PREFIX(fp12_mul_basic)
#define fp12_mul_lazyr 	PREFIX(fp12_mul_lazyr)
#define fp12_mul_dxs_basic 	PREFIX(fp12_mul_dxs_basic)
#define fp12_mul_dxs_lazyr 	PREFIX(fp12_mul_dxs_lazyr)
#define fp12_sqr_basic 	PREFIX(fp12_sqr_basic)
#define fp12_sqr_lazyr 	PREFIX(fp12_sqr_lazyr)
#define fp12_sqr_cyc_basic 	PREFIX(fp12_sqr_cyc_basic)
#define fp12_sqr_cyc_lazyr 	PREFIX(fp12_sqr_cyc_lazyr)
#define fp12_sqr_pck_basic 	PREFIX(fp12_sqr_pck_basic)
#define fp12_sqr_pck_lazyr 	PREFIX(fp12_sqr_pck_lazyr)
#define fp12_test_cyc 	PREFIX(fp12_test_cyc)
#define fp12_conv_cyc 	PREFIX(fp12_conv_cyc)
#define fp12_back_cyc 	PREFIX(fp12_back_cyc)
#define fp12_back_cyc_sim 	PREFIX(fp12_back_cyc_sim)
#define fp12_inv 	PREFIX(fp12_inv)
#define fp12_inv_uni 	PREFIX(fp12_inv_uni)
#define fp12_conv_uni 	PREFIX(fp12_conv_uni)
#define fp12_frb 	PREFIX(fp12_frb)
#define fp12_exp 	PREFIX(fp12_exp)
#define fp12_exp_cyc 	PREFIX(fp12_exp_cyc)
#define fp12_exp_cyc_sps 	PREFIX(fp12_exp_cyc_sps)
#define fp12_pck 	PREFIX(fp12_pck)
#define fp12_upk 	PREFIX(fp12_upk)

#undef fp18_copy
#undef fp18_zero
#undef fp18_is_zero
#undef fp18_rand
#undef fp18_print
#undef fp18_cmp
#undef fp18_cmp_dig
#undef fp18_set_dig
#undef fp18_add
#undef fp18_sub
#undef fp18_neg
#undef fp18_mul_basic
#undef fp18_mul_lazyr
#undef fp18_mul_dxs_basic
#undef fp18_mul_dxs_lazyr
#undef fp18_sqr_basic
#undef fp18_sqr_lazyr
#undef fp18_sqr_cyc
#undef fp18_sqr_pck
#undef fp18_test_cyc
#undef fp18_conv_cyc
#undef fp18_back_cyc
#undef fp18_back_cyc_sim
#undef fp18_inv
#undef fp18_inv_uni
#undef fp18_conv_uni
#undef fp18_frb
#undef fp18_exp
#undef fp18_exp_cyc
#undef fp18_exp_cyc_sps

#define fp18_copy 	PREFIX(fp18_copy)
#define fp18_zero 	PREFIX(fp18_zero)
#define fp18_is_zero 	PREFIX(fp18_is_zero)
#define fp18_rand 	PREFIX(fp18_rand)
#define fp18_print 	PREFIX(fp18_print)
#define fp18_cmp 	PREFIX(fp18_cmp)
#define fp18_cmp_dig 	PREFIX(fp18_cmp_dig)
#define fp18_set_dig 	PREFIX(fp18_set_dig)
#define fp18_add 	PREFIX(fp18_add)
#define fp18_sub 	PREFIX(fp18_sub)
#define fp18_neg 	PREFIX(fp18_neg)
#define fp18_mul_basic 	PREFIX(fp18_mul_basic)
#define fp18_mul_lazyr 	PREFIX(fp18_mul_lazyr)
#define fp18_mul_dxs_basic 	PREFIX(fp18_mul_dxs_basic)
#define fp18_mul_dxs_lazyr 	PREFIX(fp18_mul_dxs_lazyr)
#define fp18_sqr_basic 	PREFIX(fp18_sqr_basic)
#define fp18_sqr_lazyr 	PREFIX(fp18_sqr_lazyr)
#define fp18_sqr_cyc 	PREFIX(fp18_sqr_cyc)
#define fp18_sqr_pck 	PREFIX(fp18_sqr_pck)
#define fp18_test_cyc 	PREFIX(fp18_test_cyc)
#define fp18_conv_cyc 	PREFIX(fp18_conv_cyc)
#define fp18_back_cyc 	PREFIX(fp18_back_cyc)
#define fp18_back_cyc_sim 	PREFIX(fp18_back_cyc_sim)
#define fp18_inv 	PREFIX(fp18_inv)
#define fp18_inv_uni 	PREFIX(fp18_inv_uni)
#define fp18_conv_uni 	PREFIX(fp18_conv_uni)
#define fp18_frb 	PREFIX(fp18_frb)
#define fp18_exp 	PREFIX(fp18_exp)
#define fp18_exp_cyc 	PREFIX(fp18_exp_cyc)
#define fp18_exp_cyc_sps 	PREFIX(fp18_exp_cyc_sps)

#undef fb2_mul
 #undef fb2_mul_nor
#undef fb2_sqr
#undef fb2_slv
#undef fb2_inv

#define fb2_mul 	PREFIX(fb2_mul)
 #define fb2_mul_nor 	PREFIX(fb2_mul_nor)
#define fb2_sqr 	PREFIX(fb2_sqr)
#define fb2_slv 	PREFIX(fb2_slv)
#define fb2_inv 	PREFIX(fb2_inv)



#undef pp_map_init
#undef pp_map_clean
#undef pp_add_k2_basic
#undef pp_add_k2_projc_basic
#undef pp_add_k2_projc_lazyr
#undef pp_add_k12_basic
#undef pp_add_k12_projc_basic
#undef pp_add_k12_projc_lazyr
#undef pp_add_lit_k12
#undef pp_dbl_k2_basic
#undef pp_dbl_k2_projc_basic
#undef pp_dbl_k2_projc_lazyr
#undef pp_dbl_k12_basic
#undef pp_dbl_k12_projc_basic
#undef pp_dbl_k12_projc_lazyr
#undef pp_dbl_lit_k12
#undef pp_exp_k2
#undef pp_exp_k12
#undef pp_norm_k2
#undef pp_norm_k12
#undef pp_map_tatep_k2
#undef pp_map_sim_tatep_k2
#undef pp_map_weilp_k2
#undef pp_map_sim_weilp_k2
#undef pp_map_tatep_k12
#undef pp_map_sim_tatep_k12
#undef pp_map_weilp_k12
#undef pp_map_sim_weilp_k12
#undef pp_map_oatep_k12
#undef pp_map_sim_oatep_k12

#define pp_map_init 	PREFIX(pp_map_init)
#define pp_map_clean 	PREFIX(pp_map_clean)
#define pp_add_k2_basic 	PREFIX(pp_add_k2_basic)
#define pp_add_k2_projc_basic 	PREFIX(pp_add_k2_projc_basic)
#define pp_add_k2_projc_lazyr 	PREFIX(pp_add_k2_projc_lazyr)
#define pp_add_k12_basic 	PREFIX(pp_add_k12_basic)
#define pp_add_k12_projc_basic 	PREFIX(pp_add_k12_projc_basic)
#define pp_add_k12_projc_lazyr 	PREFIX(pp_add_k12_projc_lazyr)
#define pp_add_lit_k12 	PREFIX(pp_add_lit_k12)
#define pp_dbl_k2_basic 	PREFIX(pp_dbl_k2_basic)
#define pp_dbl_k2_projc_basic 	PREFIX(pp_dbl_k2_projc_basic)
#define pp_dbl_k2_projc_lazyr 	PREFIX(pp_dbl_k2_projc_lazyr)
#define pp_dbl_k12_basic 	PREFIX(pp_dbl_k12_basic)
#define pp_dbl_k12_projc_basic 	PREFIX(pp_dbl_k12_projc_basic)
#define pp_dbl_k12_projc_lazyr 	PREFIX(pp_dbl_k12_projc_lazyr)
#define pp_dbl_lit_k12 	PREFIX(pp_dbl_lit_k12)
#define pp_exp_k2 	PREFIX(pp_exp_k2)
#define pp_exp_k12 	PREFIX(pp_exp_k12)
#define pp_norm_k2 	PREFIX(pp_norm_k2)
#define pp_norm_k12 	PREFIX(pp_norm_k12)
#define pp_map_tatep_k2 	PREFIX(pp_map_tatep_k2)
#define pp_map_sim_tatep_k2 	PREFIX(pp_map_sim_tatep_k2)
#define pp_map_weilp_k2 	PREFIX(pp_map_weilp_k2)
#define pp_map_sim_weilp_k2 	PREFIX(pp_map_sim_weilp_k2)
#define pp_map_tatep_k12 	PREFIX(pp_map_tatep_k12)
#define pp_map_sim_tatep_k12 	PREFIX(pp_map_sim_tatep_k12)
#define pp_map_weilp_k12 	PREFIX(pp_map_weilp_k12)
#define pp_map_sim_weilp_k12 	PREFIX(pp_map_sim_weilp_k12)
#define pp_map_oatep_k12 	PREFIX(pp_map_oatep_k12)
#define pp_map_sim_oatep_k12 	PREFIX(pp_map_sim_oatep_k12)

#undef rsa_t
#undef rabin_t
#undef bdpe_t
#undef sokaka_t
#define rsa_t		PREFIX(rsa_t)
#define rabin_t	PREFIX(rabin_t)
#define bdpe_t	PREFIX(bdpe_t)
#define sokaka_t	PREFIX(sokaka_t)

#undef cp_rsa_gen_basic
#undef cp_rsa_gen_quick
#undef cp_rsa_enc
#undef cp_rsa_dec_basic
#undef cp_rsa_dec_quick
#undef cp_rsa_sig_basic
#undef cp_rsa_sig_quick
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
#undef cp_ibe_gen
#undef cp_bgn_gen
#undef cp_bgn_enc1
#undef cp_bgn_dec1
#undef cp_bgn_enc2
#undef cp_bgn_dec2
#undef cp_bgn_add
#undef cp_bgn_mul
#undef cp_bgn_dec
#undef cp_ibe_gen_prv
#undef cp_ibe_enc
#undef cp_ibe_dec
#undef cp_bls_gen
#undef cp_bls_sig
#undef cp_bls_ver
#undef cp_bbs_gen
#undef cp_bbs_sig
#undef cp_bbs_ver
#undef cp_zss_gen
#undef cp_zss_sig
#undef cp_zss_ver
#undef cp_vbnn_ibs_kgc_gen
#undef cp_vbnn_ibs_kgc_extract_key
#undef cp_vbnn_ibs_user_sign
#undef cp_vbnn_ibs_user_verify

#define cp_rsa_gen_basic 	PREFIX(cp_rsa_gen_basic)
#define cp_rsa_gen_quick 	PREFIX(cp_rsa_gen_quick)
#define cp_rsa_enc 	PREFIX(cp_rsa_enc)
#define cp_rsa_dec_basic 	PREFIX(cp_rsa_dec_basic)
#define cp_rsa_dec_quick 	PREFIX(cp_rsa_dec_quick)
#define cp_rsa_sig_basic 	PREFIX(cp_rsa_sig_basic)
#define cp_rsa_sig_quick 	PREFIX(cp_rsa_sig_quick)
#define cp_rsa_ver 	PREFIX(cp_rsa_ver)
#define cp_rabin_gen 	PREFIX(cp_rabin_gen)
#define cp_rabin_enc 	PREFIX(cp_rabin_enc)
#define cp_rabin_dec 	PREFIX(cp_rabin_dec)
#define cp_bdpe_gen 	PREFIX(cp_bdpe_gen)
#define cp_bdpe_enc 	PREFIX(cp_bdpe_enc)
#define cp_bdpe_dec 	PREFIX(cp_bdpe_dec)
#define cp_phpe_gen 	PREFIX(cp_phpe_gen)
#define cp_phpe_enc 	PREFIX(cp_phpe_enc)
#define cp_phpe_dec 	PREFIX(cp_phpe_dec)
#define cp_ecdh_gen 	PREFIX(cp_ecdh_gen)
#define cp_ecdh_key 	PREFIX(cp_ecdh_key)
#define cp_ecmqv_gen 	PREFIX(cp_ecmqv_gen)
#define cp_ecmqv_key 	PREFIX(cp_ecmqv_key)
#define cp_ecies_gen 	PREFIX(cp_ecies_gen)
#define cp_ecies_enc 	PREFIX(cp_ecies_enc)
#define cp_ecies_dec 	PREFIX(cp_ecies_dec)
#define cp_ecdsa_gen 	PREFIX(cp_ecdsa_gen)
#define cp_ecdsa_sig 	PREFIX(cp_ecdsa_sig)
#define cp_ecdsa_ver 	PREFIX(cp_ecdsa_ver)
#define cp_ecss_gen 	PREFIX(cp_ecss_gen)
#define cp_ecss_sig 	PREFIX(cp_ecss_sig)
#define cp_ecss_ver 	PREFIX(cp_ecss_ver)
#define cp_sokaka_gen 	PREFIX(cp_sokaka_gen)
#define cp_sokaka_gen_prv 	PREFIX(cp_sokaka_gen_prv)
#define cp_sokaka_key 	PREFIX(cp_sokaka_key)
#define cp_ibe_gen 	PREFIX(cp_ibe_gen)
#define cp_bgn_gen 	PREFIX(cp_bgn_gen)
#define cp_bgn_enc1 	PREFIX(cp_bgn_enc1)
#define cp_bgn_dec1 	PREFIX(cp_bgn_dec1)
#define cp_bgn_enc2 	PREFIX(cp_bgn_enc2)
#define cp_bgn_dec2 	PREFIX(cp_bgn_dec2)
#define cp_bgn_add 	PREFIX(cp_bgn_add)
#define cp_bgn_mul 	PREFIX(cp_bgn_mul)
#define cp_bgn_dec 	PREFIX(cp_bgn_dec)
#define cp_ibe_gen_prv 	PREFIX(cp_ibe_gen_prv)
#define cp_ibe_enc 	PREFIX(cp_ibe_enc)
#define cp_ibe_dec 	PREFIX(cp_ibe_dec)
#define cp_bls_gen 	PREFIX(cp_bls_gen)
#define cp_bls_sig 	PREFIX(cp_bls_sig)
#define cp_bls_ver 	PREFIX(cp_bls_ver)
#define cp_bbs_gen 	PREFIX(cp_bbs_gen)
#define cp_bbs_sig 	PREFIX(cp_bbs_sig)
#define cp_bbs_ver 	PREFIX(cp_bbs_ver)
#define cp_zss_gen 	PREFIX(cp_zss_gen)
#define cp_zss_sig 	PREFIX(cp_zss_sig)
#define cp_zss_ver 	PREFIX(cp_zss_ver)
#define cp_vbnn_ibs_kgc_gen 	PREFIX(cp_vbnn_ibs_kgc_gen)
#define cp_vbnn_ibs_kgc_extract_key 	PREFIX(cp_vbnn_ibs_kgc_extract_key)
#define cp_vbnn_ibs_user_sign 	PREFIX(cp_vbnn_ibs_user_sign)
#define cp_vbnn_ibs_user_verify 	PREFIX(cp_vbnn_ibs_user_verify)

#endif /* LABEL */

#endif /* !RELIC_LABEL_H */
