// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <clocale>
#include <cstdint>
#include <locale>
#include <string>
#include <vector>

namespace {
const std::string locale_identifiers[] = {
    "C", "C.UTF-8", "aa_DJ", "aa_DJ.ISO-8859-1", "aa_DJ.UTF-8", "aa_ER", "aa_ER.UTF-8", "aa_ET", "aa_ET.UTF-8", "af_ZA", "af_ZA.ISO-8859-1", "af_ZA.UTF-8", "agr_PE", "agr_PE.UTF-8", "ak_GH", "ak_GH.UTF-8", "am_ET", "am_ET.UTF-8", "an_ES", "an_ES.ISO-8859-15", "an_ES.UTF-8", "anp_IN", "anp_IN.UTF-8", "ar_AE", "ar_AE.ISO-8859-6", "ar_AE.UTF-8", "ar_BH", "ar_BH.ISO-8859-6", "ar_BH.UTF-8", "ar_DZ", "ar_DZ.ISO-8859-6", "ar_DZ.UTF-8", "ar_EG", "ar_EG.ISO-8859-6", "ar_EG.UTF-8", "ar_IN", "ar_IN.UTF-8", "ar_IQ", "ar_IQ.ISO-8859-6", "ar_IQ.UTF-8", "ar_JO", "ar_JO.ISO-8859-6", "ar_JO.UTF-8", "ar_KW", "ar_KW.ISO-8859-6", "ar_KW.UTF-8", "ar_LB", "ar_LB.ISO-8859-6", "ar_LB.UTF-8", "ar_LY", "ar_LY.ISO-8859-6", "ar_LY.UTF-8", "ar_MA", "ar_MA.ISO-8859-6", "ar_MA.UTF-8", "ar_OM", "ar_OM.ISO-8859-6", "ar_OM.UTF-8", "ar_QA", "ar_QA.ISO-8859-6", "ar_QA.UTF-8", "ar_SA", "ar_SA.ISO-8859-6", "ar_SA.UTF-8", "ar_SD", "ar_SD.ISO-8859-6", "ar_SD.UTF-8", "ar_SS", "ar_SS.UTF-8", "ar_SY", "ar_SY.ISO-8859-6", "ar_SY.UTF-8", "ar_TN", "ar_TN.ISO-8859-6", "ar_TN.UTF-8", "ar_YE", "ar_YE.ISO-8859-6", "ar_YE.UTF-8", "as_IN", "as_IN.UTF-8", "ast_ES", "ast_ES.ISO-8859-15", "ast_ES.UTF-8", "ayc_PE", "ayc_PE.UTF-8", "az_AZ", "az_AZ.UTF-8", "az_IR", "az_IR.UTF-8", "be_BY", "be_BY.CP1251", "be_BY.UTF-8", "bem_ZM", "bem_ZM.UTF-8", "ber_DZ", "ber_DZ.UTF-8", "ber_MA", "ber_MA.UTF-8", "bg_BG", "bg_BG.CP1251", "bg_BG.UTF-8", "bho_IN", "bho_IN.UTF-8", "bho_NP", "bho_NP.UTF-8", "bi_VU", "bi_VU.UTF-8", "bn_BD", "bn_BD.UTF-8", "bn_IN", "bn_IN.UTF-8", "bo_CN", "bo_CN.UTF-8", "bo_IN", "bo_IN.UTF-8", "br_FR", "br_FR.ISO-8859-1", "br_FR.UTF-8", "brx_IN", "brx_IN.UTF-8", "bs_BA", "bs_BA.ISO-8859-2", "bs_BA.UTF-8", "byn_ER", "byn_ER.UTF-8", "ca_AD", "ca_AD.ISO-8859-15", "ca_AD.UTF-8", "ca_ES", "ca_ES.ISO-8859-1", "ca_ES.UTF-8", "ca_FR", "ca_FR.ISO-8859-15", "ca_FR.UTF-8", "ca_IT", "ca_IT.ISO-8859-15", "ca_IT.UTF-8", "ce_RU", "ce_RU.UTF-8", "chr_US", "chr_US.UTF-8", "ckb_IQ", "ckb_IQ.UTF-8", "cmn_TW", "cmn_TW.UTF-8", "crh_UA", "crh_UA.UTF-8", "csb_PL", "csb_PL.UTF-8", "cs_CZ", "cs_CZ.ISO-8859-2", "cs_CZ.UTF-8", "cv_RU", "cv_RU.UTF-8", "cy_GB", "cy_GB.ISO-8859-14", "cy_GB.UTF-8", "da_DK", "da_DK.ISO-8859-1", "da_DK.UTF-8", "de_AT", "de_AT.ISO-8859-1", "de_AT.UTF-8", "de_BE", "de_BE.ISO-8859-1", "de_BE.UTF-8", "de_CH", "de_CH.ISO-8859-1", "de_CH.UTF-8", "de_DE", "de_DE.ISO-8859-1", "de_DE.UTF-8", "de_IT", "de_IT.ISO-8859-1", "de_IT.UTF-8", "de_LU", "de_LU.ISO-8859-1", "de_LU.UTF-8", "doi_IN", "doi_IN.UTF-8", "dv_MV", "dv_MV.UTF-8", "dz_BT", "dz_BT.UTF-8", "el_CY", "el_CY.ISO-8859-7", "el_CY.UTF-8", "el_GR", "el_GR.ISO-8859-7", "el_GR.UTF-8", "en_AG", "en_AG.UTF-8", "en_AU", "en_AU.ISO-8859-1", "en_AU.UTF-8", "en_BW", "en_BW.ISO-8859-1", "en_BW.UTF-8", "en_CA", "en_CA.ISO-8859-1", "en_CA.UTF-8", "en_DK", "en_DK.ISO-8859-1", "en_DK.ISO-8859-15", "en_DK.UTF-8", "en_GB", "en_GB.ISO-8859-1", "en_GB.ISO-8859-15", "en_GB.UTF-8", "en_HK", "en_HK.ISO-8859-1", "en_HK.UTF-8", "en_IE", "en_IE.ISO-8859-1", "en_IE.UTF-8", "en_IL", "en_IL.UTF-8", "en_IN", "en_IN.UTF-8", "en_NG", "en_NG.UTF-8", "en_NZ", "en_NZ.ISO-8859-1", "en_NZ.UTF-8", "en_PH", "en_PH.ISO-8859-1", "en_PH.UTF-8", "en_SG", "en_SG.ISO-8859-1", "en_SG.UTF-8", "en_US", "en_US.ISO-8859-1", "en_US.ISO-8859-15", "en_US.UTF-8", "en_ZA", "en_ZA.ISO-8859-1", "en_ZA.UTF-8", "en_ZM", "en_ZM.UTF-8", "en_ZW", "en_ZW.ISO-8859-1", "en_ZW.UTF-8", "es_AR", "es_AR.ISO-8859-1", "es_AR.UTF-8", "es_BO", "es_BO.ISO-8859-1", "es_BO.UTF-8", "es_CL", "es_CL.ISO-8859-1", "es_CL.UTF-8", "es_CO", "es_CO.ISO-8859-1", "es_CO.UTF-8", "es_CR", "es_CR.ISO-8859-1", "es_CR.UTF-8", "es_CU", "es_CU.UTF-8", "es_DO", "es_DO.ISO-8859-1", "es_DO.UTF-8", "es_EC", "es_EC.ISO-8859-1", "es_EC.UTF-8", "es_ES", "es_ES.ISO-8859-1", "es_ES.UTF-8", "es_GT", "es_GT.ISO-8859-1", "es_GT.UTF-8", "es_HN", "es_HN.ISO-8859-1", "es_HN.UTF-8", "es_MX", "es_MX.ISO-8859-1", "es_MX.UTF-8", "es_NI", "es_NI.ISO-8859-1", "es_NI.UTF-8", "es_PA", "es_PA.ISO-8859-1", "es_PA.UTF-8", "es_PE", "es_PE.ISO-8859-1", "es_PE.UTF-8", "es_PR", "es_PR.ISO-8859-1", "es_PR.UTF-8", "es_PY", "es_PY.ISO-8859-1", "es_PY.UTF-8", "es_SV", "es_SV.ISO-8859-1", "es_SV.UTF-8", "es_US", "es_US.ISO-8859-1", "es_US.UTF-8", "es_UY", "es_UY.ISO-8859-1", "es_UY.UTF-8", "es_VE", "es_VE.ISO-8859-1", "es_VE.UTF-8", "et_EE", "et_EE.ISO-8859-1", "et_EE.ISO-8859-15", "et_EE.UTF-8", "eu_ES", "eu_ES.ISO-8859-1", "eu_ES.UTF-8", "eu_FR", "eu_FR.ISO-8859-1", "eu_FR.UTF-8", "fa_IR", "fa_IR.UTF-8", "ff_SN", "ff_SN.UTF-8", "fi_FI", "fi_FI.ISO-8859-1", "fi_FI.UTF-8", "fil_PH", "fil_PH.UTF-8", "fo_FO", "fo_FO.ISO-8859-1", "fo_FO.UTF-8", "fr_BE", "fr_BE.ISO-8859-1", "fr_BE.UTF-8", "fr_CA", "fr_CA.ISO-8859-1", "fr_CA.UTF-8", "fr_CH", "fr_CH.ISO-8859-1", "fr_CH.UTF-8", "fr_FR", "fr_FR.ISO-8859-1", "fr_FR.UTF-8", "fr_LU", "fr_LU.ISO-8859-1", "fr_LU.UTF-8", "fur_IT", "fur_IT.UTF-8", "fy_DE", "fy_DE.UTF-8", "fy_NL", "fy_NL.UTF-8", "ga_IE", "ga_IE.ISO-8859-1", "ga_IE.UTF-8", "gd_GB", "gd_GB.ISO-8859-15", "gd_GB.UTF-8", "gez_ER", "gez_ER.UTF-8", "gez_ET", "gez_ET.UTF-8", "gl_ES", "gl_ES.ISO-8859-1", "gl_ES.UTF-8", "gu_IN", "gu_IN.UTF-8", "gv_GB", "gv_GB.ISO-8859-1", "gv_GB.UTF-8", "hak_TW", "hak_TW.UTF-8", "ha_NG", "ha_NG.UTF-8", "he_IL", "he_IL.ISO-8859-8", "he_IL.UTF-8", "hif_FJ", "hif_FJ.UTF-8", "hi_IN", "hi_IN.UTF-8", "hne_IN", "hne_IN.UTF-8", "hr_HR", "hr_HR.ISO-8859-2", "hr_HR.UTF-8", "hsb_DE", "hsb_DE.ISO-8859-2", "hsb_DE.UTF-8", "ht_HT", "ht_HT.UTF-8", "hu_HU", "hu_HU.ISO-8859-2", "hu_HU.UTF-8", "hy_AM", "hy_AM.ARMSCII-8", "hy_AM.UTF-8", "ia_FR", "ia_FR.UTF-8", "id_ID", "id_ID.ISO-8859-1", "id_ID.UTF-8", "ig_NG", "ig_NG.UTF-8", "ik_CA", "ik_CA.UTF-8", "is_IS", "is_IS.ISO-8859-1", "is_IS.UTF-8", "it_CH", "it_CH.ISO-8859-1", "it_CH.UTF-8", "it_IT", "it_IT.ISO-8859-1", "it_IT.UTF-8", "iu_CA", "iu_CA.UTF-8", "kab_DZ", "kab_DZ.UTF-8", "ka_GE", "ka_GE.GEORGIAN-PS", "ka_GE.UTF-8", "kk_KZ", "kk_KZ.PT154", "kk_KZ.RK1048", "kk_KZ.UTF-8", "kl_GL", "kl_GL.ISO-8859-1", "kl_GL.UTF-8", "km_KH", "km_KH.UTF-8", "kn_IN", "kn_IN.UTF-8", "kok_IN", "kok_IN.UTF-8", "ks_IN", "ks_IN.UTF-8", "ku_TR", "ku_TR.ISO-8859-9", "ku_TR.UTF-8", "kw_GB", "kw_GB.ISO-8859-1", "kw_GB.UTF-8", "ky_KG", "ky_KG.UTF-8", "lb_LU", "lb_LU.UTF-8", "lg_UG", "lg_UG.ISO-8859-10", "lg_UG.UTF-8", "li_BE", "li_BE.UTF-8", "lij_IT", "lij_IT.UTF-8", "li_NL", "li_NL.UTF-8", "ln_CD", "ln_CD.UTF-8", "lo_LA", "lo_LA.UTF-8", "lt_LT", "lt_LT.ISO-8859-13", "lt_LT.UTF-8", "lv_LV", "lv_LV.ISO-8859-13", "lv_LV.UTF-8", "lzh_TW", "lzh_TW.UTF-8", "mag_IN", "mag_IN.UTF-8", "mai_IN", "mai_IN.UTF-8", "mai_NP", "mai_NP.UTF-8", "mfe_MU", "mfe_MU.UTF-8", "mg_MG", "mg_MG.ISO-8859-15", "mg_MG.UTF-8", "mhr_RU", "mhr_RU.UTF-8", "mi_NZ", "mi_NZ.ISO-8859-13", "mi_NZ.UTF-8", "miq_NI", "miq_NI.UTF-8", "mjw_IN", "mjw_IN.UTF-8", "mk_MK", "mk_MK.ISO-8859-5", "mk_MK.UTF-8", "ml_IN", "ml_IN.UTF-8", "mni_IN", "mni_IN.UTF-8", "mn_MN", "mn_MN.UTF-8", "mr_IN", "mr_IN.UTF-8", "ms_MY", "ms_MY.ISO-8859-1", "ms_MY.UTF-8", "mt_MT", "mt_MT.ISO-8859-3", "mt_MT.UTF-8", "my_MM", "my_MM.UTF-8", "nan_TW", "nan_TW.UTF-8", "nb_NO", "nb_NO.ISO-8859-1", "nb_NO.UTF-8", "nds_DE", "nds_DE.UTF-8", "nds_NL", "nds_NL.UTF-8", "ne_NP", "ne_NP.UTF-8", "nhn_MX", "nhn_MX.UTF-8", "niu_NU", "niu_NU.UTF-8", "niu_NZ", "niu_NZ.UTF-8", "nl_AW", "nl_AW.UTF-8", "nl_BE", "nl_BE.ISO-8859-1", "nl_BE.UTF-8", "nl_NL", "nl_NL.ISO-8859-1", "nl_NL.UTF-8", "nn_NO", "nn_NO.ISO-8859-1", "nn_NO.UTF-8", "nr_ZA", "nr_ZA.UTF-8", "nso_ZA", "nso_ZA.UTF-8", "oc_FR", "oc_FR.ISO-8859-1", "oc_FR.UTF-8", "om_ET", "om_ET.UTF-8", "om_KE", "om_KE.ISO-8859-1", "om_KE.UTF-8", "or_IN", "or_IN.UTF-8", "os_RU", "os_RU.UTF-8", "pa_IN", "pa_IN.UTF-8", "pap_AW", "pap_AW.UTF-8", "pap_CW", "pap_CW.UTF-8", "pa_PK", "pa_PK.UTF-8", "pl_PL", "pl_PL.ISO-8859-2", "pl_PL.UTF-8", "ps_AF", "ps_AF.UTF-8", "pt_BR", "pt_BR.ISO-8859-1", "pt_BR.UTF-8", "pt_PT", "pt_PT.ISO-8859-1", "pt_PT.UTF-8", "quz_PE", "quz_PE.UTF-8", "raj_IN", "raj_IN.UTF-8", "ro_RO", "ro_RO.ISO-8859-2", "ro_RO.UTF-8", "ru_RU", "ru_RU.CP1251", "ru_RU.ISO-8859-5", "ru_RU.KOI8-R", "ru_RU.UTF-8", "ru_UA", "ru_UA.KOI8-U", "ru_UA.UTF-8", "rw_RW", "rw_RW.UTF-8", "sa_IN", "sa_IN.UTF-8", "sat_IN", "sat_IN.UTF-8", "sc_IT", "sc_IT.UTF-8", "sd_IN", "sd_IN.UTF-8", "sd_PK", "sd_PK.UTF-8", "se_NO", "se_NO.UTF-8", "sgs_LT", "sgs_LT.UTF-8", "shn_MM", "shn_MM.UTF-8", "shs_CA", "shs_CA.UTF-8", "sid_ET", "sid_ET.UTF-8", "si_LK", "si_LK.UTF-8", "sk_SK", "sk_SK.ISO-8859-2", "sk_SK.UTF-8", "sl_SI", "sl_SI.ISO-8859-2", "sl_SI.UTF-8", "sm_WS", "sm_WS.UTF-8", "so_DJ", "so_DJ.ISO-8859-1", "so_DJ.UTF-8", "so_ET", "so_ET.UTF-8", "so_KE", "so_KE.ISO-8859-1", "so_KE.UTF-8", "so_SO", "so_SO.ISO-8859-1", "so_SO.UTF-8", "sq_AL", "sq_AL.ISO-8859-1", "sq_AL.UTF-8", "sq_MK", "sq_MK.UTF-8", "sr_ME", "sr_ME.UTF-8", "sr_RS", "sr_RS.UTF-8", "ss_ZA", "ss_ZA.UTF-8", "st_ZA", "st_ZA.ISO-8859-1", "st_ZA.UTF-8", "sv_FI", "sv_FI.ISO-8859-1", "sv_FI.UTF-8", "sv_SE", "sv_SE.ISO-8859-1", "sv_SE.ISO-8859-15", "sv_SE.UTF-8", "sw_KE", "sw_KE.UTF-8", "sw_TZ", "sw_TZ.UTF-8", "szl_PL", "szl_PL.UTF-8", "ta_IN", "ta_IN.UTF-8", "ta_LK", "ta_LK.UTF-8", "te_IN", "te_IN.UTF-8", "tg_TJ", "tg_TJ.KOI8-T", "tg_TJ.UTF-8", "the_NP", "the_NP.UTF-8", "th_TH", "th_TH.TIS-620", "th_TH.UTF-8", "ti_ER", "ti_ER.UTF-8", "ti_ET", "ti_ET.UTF-8", "tig_ER", "tig_ER.UTF-8", "tk_TM", "tk_TM.UTF-8", "tl_PH", "tl_PH.ISO-8859-1", "tl_PH.UTF-8", "tn_ZA", "tn_ZA.UTF-8", "to_TO", "to_TO.UTF-8", "tpi_PG", "tpi_PG.UTF-8", "tr_CY", "tr_CY.ISO-8859-9", "tr_CY.UTF-8", "tr_TR", "tr_TR.ISO-8859-9", "tr_TR.UTF-8", "ts_ZA", "ts_ZA.UTF-8", "tt_RU", "tt_RU.UTF-8", "ug_CN", "ug_CN.UTF-8", "uk_UA", "uk_UA.KOI8-U", "uk_UA.UTF-8", "unm_US", "unm_US.UTF-8", "ur_IN", "ur_IN.UTF-8", "ur_PK", "ur_PK.UTF-8", "uz_UZ", "uz_UZ.ISO-8859-1", "uz_UZ.UTF-8", "ve_ZA", "ve_ZA.UTF-8", "vi_VN", "vi_VN.UTF-8", "wa_BE", "wa_BE.ISO-8859-1", "wa_BE.UTF-8", "wae_CH", "wae_CH.UTF-8", "wal_ET", "wal_ET.UTF-8", "wo_SN", "wo_SN.UTF-8", "xh_ZA", "xh_ZA.ISO-8859-1", "xh_ZA.UTF-8", "yi_US", "yi_US.CP1255", "yi_US.UTF-8", "yo_NG", "yo_NG.UTF-8", "yue_HK", "yue_HK.UTF-8", "yuw_PG", "yuw_PG.UTF-8", "zh_CN", "zh_CN.GB18030", "zh_CN.GB2312", "zh_CN.GBK", "zh_CN.UTF-8", "zh_HK", "zh_HK.BIG5-HKSCS", "zh_HK.UTF-8", "zh_SG", "zh_SG.GB2312", "zh_SG.GBK", "zh_SG.UTF-8", "zh_TW", "zh_TW.BIG5", "zh_TW.EUC-TW", "zh_TW.UTF-8", "zu_ZA", "zu_ZA.ISO-8859-1", "zu_ZA.UTF-8"};

std::string ConsumeLocaleIdentifier(FuzzedDataProvider& fuzzed_data_provider)
{
    return fuzzed_data_provider.PickValueInArray<std::string>(locale_identifiers);
}

bool IsAvailableLocale(const std::string& locale_identifier)
{
    try {
        (void)std::locale(locale_identifier);
    } catch (const std::runtime_error&) {
        return false;
    }
    return true;
}
} // namespace

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string locale_identifier = ConsumeLocaleIdentifier(fuzzed_data_provider);
    if (!IsAvailableLocale(locale_identifier)) {
        return;
    }
    const char* c_locale = std::setlocale(LC_ALL, "C");
    assert(c_locale != nullptr);

    const std::string random_string = fuzzed_data_provider.ConsumeRandomLengthString(5);
    int32_t parseint32_out_without_locale;
    const bool parseint32_without_locale = ParseInt32(random_string, &parseint32_out_without_locale);
    int64_t parseint64_out_without_locale;
    const bool parseint64_without_locale = ParseInt64(random_string, &parseint64_out_without_locale);
    const int64_t atoi64_without_locale = atoi64(random_string);
    const int atoi_without_locale = atoi(random_string);
    const int64_t random_int64 = fuzzed_data_provider.ConsumeIntegral<int64_t>();
    const std::string tostring_without_locale = ToString(random_int64);
    // The variable `random_int32` is no longer used, but the harness still needs to
    // consume the same data that it did previously to not invalidate existing seeds.
    const int32_t random_int32 = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    (void)random_int32;
    const std::string strprintf_int_without_locale = strprintf("%d", random_int64);
    const double random_double = fuzzed_data_provider.ConsumeFloatingPoint<double>();
    const std::string strprintf_double_without_locale = strprintf("%f", random_double);

    const char* new_locale = std::setlocale(LC_ALL, locale_identifier.c_str());
    assert(new_locale != nullptr);

    int32_t parseint32_out_with_locale;
    const bool parseint32_with_locale = ParseInt32(random_string, &parseint32_out_with_locale);
    assert(parseint32_without_locale == parseint32_with_locale);
    if (parseint32_without_locale) {
        assert(parseint32_out_without_locale == parseint32_out_with_locale);
    }
    int64_t parseint64_out_with_locale;
    const bool parseint64_with_locale = ParseInt64(random_string, &parseint64_out_with_locale);
    assert(parseint64_without_locale == parseint64_with_locale);
    if (parseint64_without_locale) {
        assert(parseint64_out_without_locale == parseint64_out_with_locale);
    }
    const int64_t atoi64_with_locale = atoi64(random_string);
    assert(atoi64_without_locale == atoi64_with_locale);
    const int atoi_with_locale = atoi(random_string);
    assert(atoi_without_locale == atoi_with_locale);
    const std::string tostring_with_locale = ToString(random_int64);
    assert(tostring_without_locale == tostring_with_locale);
    const std::string strprintf_int_with_locale = strprintf("%d", random_int64);
    assert(strprintf_int_without_locale == strprintf_int_with_locale);
    const std::string strprintf_double_with_locale = strprintf("%f", random_double);
    assert(strprintf_double_without_locale == strprintf_double_with_locale);

    const std::locale current_cpp_locale;
    assert(current_cpp_locale == std::locale::classic());
}
