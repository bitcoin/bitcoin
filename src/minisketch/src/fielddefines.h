/**********************************************************************
 * Copyright (c) 2021 Cory Fields                                     *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _MINISKETCH_FIELDDEFINES_H_
#define _MINISKETCH_FIELDDEFINES_H_

/*

This file translates external defines ENABLE_FIELD_FOO, DISABLE_FIELD_FOO, and
DISABLE_DEFAULT_FIELDS to internal ones: ENABLE_FIELD_INT_FOO. Only the
resulting internal includes should be used.

Default: All fields enabled
-DDISABLE_FIELD_3: All fields except 3 are enabled
-DENABLE_FIELD_3: All fields enabled
-DDISABLE_DEFAULT_FIELDS: Error, no fields enabled
-DDISABLE_DEFAULT_FIELDS -DENABLE_FIELD_3: Only field 3 enabled
-DDISABLE_DEFAULT_FIELDS -DENABLE_FIELD_2 -DENABLE_FIELD_3: Only fields 2 and 3 are enabled
-DDISABLE_DEFAULT_FIELDS -DENABLE_FIELD_2 -DENABLE_FIELD_3 -DDISABLE_FIELD_3: Only field 2 enabled

*/

#ifdef DISABLE_DEFAULT_FIELDS
#if defined(ENABLE_FIELD_2) && !defined(DISABLE_FIELD_2)
#define ENABLE_FIELD_INT_2
#endif
#if defined(ENABLE_FIELD_3) && !defined(DISABLE_FIELD_3)
#define ENABLE_FIELD_INT_3
#endif
#if defined(ENABLE_FIELD_4) && !defined(DISABLE_FIELD_4)
#define ENABLE_FIELD_INT_4
#endif
#if defined(ENABLE_FIELD_5) && !defined(DISABLE_FIELD_5)
#define ENABLE_FIELD_INT_5
#endif
#if defined(ENABLE_FIELD_6) && !defined(DISABLE_FIELD_6)
#define ENABLE_FIELD_INT_6
#endif
#if defined(ENABLE_FIELD_7) && !defined(DISABLE_FIELD_7)
#define ENABLE_FIELD_INT_7
#endif
#if defined(ENABLE_FIELD_8) && !defined(DISABLE_FIELD_8)
#define ENABLE_FIELD_INT_8
#endif
#if defined(ENABLE_FIELD_9) && !defined(DISABLE_FIELD_9)
#define ENABLE_FIELD_INT_9
#endif
#if defined(ENABLE_FIELD_10) && !defined(DISABLE_FIELD_10)
#define ENABLE_FIELD_INT_10
#endif
#if defined(ENABLE_FIELD_11) && !defined(DISABLE_FIELD_11)
#define ENABLE_FIELD_INT_11
#endif
#if defined(ENABLE_FIELD_12) && !defined(DISABLE_FIELD_12)
#define ENABLE_FIELD_INT_12
#endif
#if defined(ENABLE_FIELD_13) && !defined(DISABLE_FIELD_13)
#define ENABLE_FIELD_INT_13
#endif
#if defined(ENABLE_FIELD_14) && !defined(DISABLE_FIELD_14)
#define ENABLE_FIELD_INT_14
#endif
#if defined(ENABLE_FIELD_15) && !defined(DISABLE_FIELD_15)
#define ENABLE_FIELD_INT_15
#endif
#if defined(ENABLE_FIELD_16) && !defined(DISABLE_FIELD_16)
#define ENABLE_FIELD_INT_16
#endif
#if defined(ENABLE_FIELD_17) && !defined(DISABLE_FIELD_17)
#define ENABLE_FIELD_INT_17
#endif
#if defined(ENABLE_FIELD_18) && !defined(DISABLE_FIELD_18)
#define ENABLE_FIELD_INT_18
#endif
#if defined(ENABLE_FIELD_19) && !defined(DISABLE_FIELD_19)
#define ENABLE_FIELD_INT_19
#endif
#if defined(ENABLE_FIELD_20) && !defined(DISABLE_FIELD_20)
#define ENABLE_FIELD_INT_20
#endif
#if defined(ENABLE_FIELD_21) && !defined(DISABLE_FIELD_21)
#define ENABLE_FIELD_INT_21
#endif
#if defined(ENABLE_FIELD_22) && !defined(DISABLE_FIELD_22)
#define ENABLE_FIELD_INT_22
#endif
#if defined(ENABLE_FIELD_23) && !defined(DISABLE_FIELD_23)
#define ENABLE_FIELD_INT_23
#endif
#if defined(ENABLE_FIELD_24) && !defined(DISABLE_FIELD_24)
#define ENABLE_FIELD_INT_24
#endif
#if defined(ENABLE_FIELD_25) && !defined(DISABLE_FIELD_25)
#define ENABLE_FIELD_INT_25
#endif
#if defined(ENABLE_FIELD_26) && !defined(DISABLE_FIELD_26)
#define ENABLE_FIELD_INT_26
#endif
#if defined(ENABLE_FIELD_27) && !defined(DISABLE_FIELD_27)
#define ENABLE_FIELD_INT_27
#endif
#if defined(ENABLE_FIELD_28) && !defined(DISABLE_FIELD_28)
#define ENABLE_FIELD_INT_28
#endif
#if defined(ENABLE_FIELD_29) && !defined(DISABLE_FIELD_29)
#define ENABLE_FIELD_INT_29
#endif
#if defined(ENABLE_FIELD_30) && !defined(DISABLE_FIELD_30)
#define ENABLE_FIELD_INT_30
#endif
#if defined(ENABLE_FIELD_31) && !defined(DISABLE_FIELD_31)
#define ENABLE_FIELD_INT_31
#endif
#if defined(ENABLE_FIELD_32) && !defined(DISABLE_FIELD_32)
#define ENABLE_FIELD_INT_32
#endif
#if defined(ENABLE_FIELD_33) && !defined(DISABLE_FIELD_33)
#define ENABLE_FIELD_INT_33
#endif
#if defined(ENABLE_FIELD_34) && !defined(DISABLE_FIELD_34)
#define ENABLE_FIELD_INT_34
#endif
#if defined(ENABLE_FIELD_35) && !defined(DISABLE_FIELD_35)
#define ENABLE_FIELD_INT_35
#endif
#if defined(ENABLE_FIELD_36) && !defined(DISABLE_FIELD_36)
#define ENABLE_FIELD_INT_36
#endif
#if defined(ENABLE_FIELD_37) && !defined(DISABLE_FIELD_37)
#define ENABLE_FIELD_INT_37
#endif
#if defined(ENABLE_FIELD_38) && !defined(DISABLE_FIELD_38)
#define ENABLE_FIELD_INT_38
#endif
#if defined(ENABLE_FIELD_39) && !defined(DISABLE_FIELD_39)
#define ENABLE_FIELD_INT_39
#endif
#if defined(ENABLE_FIELD_40) && !defined(DISABLE_FIELD_40)
#define ENABLE_FIELD_INT_40
#endif
#if defined(ENABLE_FIELD_41) && !defined(DISABLE_FIELD_41)
#define ENABLE_FIELD_INT_41
#endif
#if defined(ENABLE_FIELD_42) && !defined(DISABLE_FIELD_42)
#define ENABLE_FIELD_INT_42
#endif
#if defined(ENABLE_FIELD_43) && !defined(DISABLE_FIELD_43)
#define ENABLE_FIELD_INT_43
#endif
#if defined(ENABLE_FIELD_44) && !defined(DISABLE_FIELD_44)
#define ENABLE_FIELD_INT_44
#endif
#if defined(ENABLE_FIELD_45) && !defined(DISABLE_FIELD_45)
#define ENABLE_FIELD_INT_45
#endif
#if defined(ENABLE_FIELD_46) && !defined(DISABLE_FIELD_46)
#define ENABLE_FIELD_INT_46
#endif
#if defined(ENABLE_FIELD_47) && !defined(DISABLE_FIELD_47)
#define ENABLE_FIELD_INT_47
#endif
#if defined(ENABLE_FIELD_48) && !defined(DISABLE_FIELD_48)
#define ENABLE_FIELD_INT_48
#endif
#if defined(ENABLE_FIELD_49) && !defined(DISABLE_FIELD_49)
#define ENABLE_FIELD_INT_49
#endif
#if defined(ENABLE_FIELD_50) && !defined(DISABLE_FIELD_50)
#define ENABLE_FIELD_INT_50
#endif
#if defined(ENABLE_FIELD_51) && !defined(DISABLE_FIELD_51)
#define ENABLE_FIELD_INT_51
#endif
#if defined(ENABLE_FIELD_52) && !defined(DISABLE_FIELD_52)
#define ENABLE_FIELD_INT_52
#endif
#if defined(ENABLE_FIELD_53) && !defined(DISABLE_FIELD_53)
#define ENABLE_FIELD_INT_53
#endif
#if defined(ENABLE_FIELD_54) && !defined(DISABLE_FIELD_54)
#define ENABLE_FIELD_INT_54
#endif
#if defined(ENABLE_FIELD_55) && !defined(DISABLE_FIELD_55)
#define ENABLE_FIELD_INT_55
#endif
#if defined(ENABLE_FIELD_56) && !defined(DISABLE_FIELD_56)
#define ENABLE_FIELD_INT_56
#endif
#if defined(ENABLE_FIELD_57) && !defined(DISABLE_FIELD_57)
#define ENABLE_FIELD_INT_57
#endif
#if defined(ENABLE_FIELD_58) && !defined(DISABLE_FIELD_58)
#define ENABLE_FIELD_INT_58
#endif
#if defined(ENABLE_FIELD_59) && !defined(DISABLE_FIELD_59)
#define ENABLE_FIELD_INT_59
#endif
#if defined(ENABLE_FIELD_60) && !defined(DISABLE_FIELD_60)
#define ENABLE_FIELD_INT_60
#endif
#if defined(ENABLE_FIELD_61) && !defined(DISABLE_FIELD_61)
#define ENABLE_FIELD_INT_61
#endif
#if defined(ENABLE_FIELD_62) && !defined(DISABLE_FIELD_62)
#define ENABLE_FIELD_INT_62
#endif
#if defined(ENABLE_FIELD_63) && !defined(DISABLE_FIELD_63)
#define ENABLE_FIELD_INT_63
#endif
#if defined(ENABLE_FIELD_64) && !defined(DISABLE_FIELD_64)
#define ENABLE_FIELD_INT_64
#endif
#else
#if !defined(DISABLE_FIELD_2)
#define ENABLE_FIELD_INT_2
#endif
#if !defined(DISABLE_FIELD_3)
#define ENABLE_FIELD_INT_3
#endif
#if !defined(DISABLE_FIELD_4)
#define ENABLE_FIELD_INT_4
#endif
#if !defined(DISABLE_FIELD_5)
#define ENABLE_FIELD_INT_5
#endif
#if !defined(DISABLE_FIELD_6)
#define ENABLE_FIELD_INT_6
#endif
#if !defined(DISABLE_FIELD_7)
#define ENABLE_FIELD_INT_7
#endif
#if !defined(DISABLE_FIELD_8)
#define ENABLE_FIELD_INT_8
#endif
#if !defined(DISABLE_FIELD_9)
#define ENABLE_FIELD_INT_9
#endif
#if !defined(DISABLE_FIELD_10)
#define ENABLE_FIELD_INT_10
#endif
#if !defined(DISABLE_FIELD_11)
#define ENABLE_FIELD_INT_11
#endif
#if !defined(DISABLE_FIELD_12)
#define ENABLE_FIELD_INT_12
#endif
#if !defined(DISABLE_FIELD_13)
#define ENABLE_FIELD_INT_13
#endif
#if !defined(DISABLE_FIELD_14)
#define ENABLE_FIELD_INT_14
#endif
#if !defined(DISABLE_FIELD_15)
#define ENABLE_FIELD_INT_15
#endif
#if !defined(DISABLE_FIELD_16)
#define ENABLE_FIELD_INT_16
#endif
#if !defined(DISABLE_FIELD_17)
#define ENABLE_FIELD_INT_17
#endif
#if !defined(DISABLE_FIELD_18)
#define ENABLE_FIELD_INT_18
#endif
#if !defined(DISABLE_FIELD_19)
#define ENABLE_FIELD_INT_19
#endif
#if !defined(DISABLE_FIELD_20)
#define ENABLE_FIELD_INT_20
#endif
#if !defined(DISABLE_FIELD_21)
#define ENABLE_FIELD_INT_21
#endif
#if !defined(DISABLE_FIELD_22)
#define ENABLE_FIELD_INT_22
#endif
#if !defined(DISABLE_FIELD_23)
#define ENABLE_FIELD_INT_23
#endif
#if !defined(DISABLE_FIELD_24)
#define ENABLE_FIELD_INT_24
#endif
#if !defined(DISABLE_FIELD_25)
#define ENABLE_FIELD_INT_25
#endif
#if !defined(DISABLE_FIELD_26)
#define ENABLE_FIELD_INT_26
#endif
#if !defined(DISABLE_FIELD_27)
#define ENABLE_FIELD_INT_27
#endif
#if !defined(DISABLE_FIELD_28)
#define ENABLE_FIELD_INT_28
#endif
#if !defined(DISABLE_FIELD_29)
#define ENABLE_FIELD_INT_29
#endif
#if !defined(DISABLE_FIELD_30)
#define ENABLE_FIELD_INT_30
#endif
#if !defined(DISABLE_FIELD_31)
#define ENABLE_FIELD_INT_31
#endif
#if !defined(DISABLE_FIELD_32)
#define ENABLE_FIELD_INT_32
#endif
#if !defined(DISABLE_FIELD_33)
#define ENABLE_FIELD_INT_33
#endif
#if !defined(DISABLE_FIELD_34)
#define ENABLE_FIELD_INT_34
#endif
#if !defined(DISABLE_FIELD_35)
#define ENABLE_FIELD_INT_35
#endif
#if !defined(DISABLE_FIELD_36)
#define ENABLE_FIELD_INT_36
#endif
#if !defined(DISABLE_FIELD_37)
#define ENABLE_FIELD_INT_37
#endif
#if !defined(DISABLE_FIELD_38)
#define ENABLE_FIELD_INT_38
#endif
#if !defined(DISABLE_FIELD_39)
#define ENABLE_FIELD_INT_39
#endif
#if !defined(DISABLE_FIELD_40)
#define ENABLE_FIELD_INT_40
#endif
#if !defined(DISABLE_FIELD_41)
#define ENABLE_FIELD_INT_41
#endif
#if !defined(DISABLE_FIELD_42)
#define ENABLE_FIELD_INT_42
#endif
#if !defined(DISABLE_FIELD_43)
#define ENABLE_FIELD_INT_43
#endif
#if !defined(DISABLE_FIELD_44)
#define ENABLE_FIELD_INT_44
#endif
#if !defined(DISABLE_FIELD_45)
#define ENABLE_FIELD_INT_45
#endif
#if !defined(DISABLE_FIELD_46)
#define ENABLE_FIELD_INT_46
#endif
#if !defined(DISABLE_FIELD_47)
#define ENABLE_FIELD_INT_47
#endif
#if !defined(DISABLE_FIELD_48)
#define ENABLE_FIELD_INT_48
#endif
#if !defined(DISABLE_FIELD_49)
#define ENABLE_FIELD_INT_49
#endif
#if !defined(DISABLE_FIELD_50)
#define ENABLE_FIELD_INT_50
#endif
#if !defined(DISABLE_FIELD_51)
#define ENABLE_FIELD_INT_51
#endif
#if !defined(DISABLE_FIELD_52)
#define ENABLE_FIELD_INT_52
#endif
#if !defined(DISABLE_FIELD_53)
#define ENABLE_FIELD_INT_53
#endif
#if !defined(DISABLE_FIELD_54)
#define ENABLE_FIELD_INT_54
#endif
#if !defined(DISABLE_FIELD_55)
#define ENABLE_FIELD_INT_55
#endif
#if !defined(DISABLE_FIELD_56)
#define ENABLE_FIELD_INT_56
#endif
#if !defined(DISABLE_FIELD_57)
#define ENABLE_FIELD_INT_57
#endif
#if !defined(DISABLE_FIELD_58)
#define ENABLE_FIELD_INT_58
#endif
#if !defined(DISABLE_FIELD_59)
#define ENABLE_FIELD_INT_59
#endif
#if !defined(DISABLE_FIELD_60)
#define ENABLE_FIELD_INT_60
#endif
#if !defined(DISABLE_FIELD_61)
#define ENABLE_FIELD_INT_61
#endif
#if !defined(DISABLE_FIELD_62)
#define ENABLE_FIELD_INT_62
#endif
#if !defined(DISABLE_FIELD_63)
#define ENABLE_FIELD_INT_63
#endif
#if !defined(DISABLE_FIELD_64)
#define ENABLE_FIELD_INT_64
#endif
#endif

#if !defined(ENABLE_FIELD_INT_2) && \
    !defined(ENABLE_FIELD_INT_3) && \
    !defined(ENABLE_FIELD_INT_4) && \
    !defined(ENABLE_FIELD_INT_5) && \
    !defined(ENABLE_FIELD_INT_6) && \
    !defined(ENABLE_FIELD_INT_7) && \
    !defined(ENABLE_FIELD_INT_8) && \
    !defined(ENABLE_FIELD_INT_9) && \
    !defined(ENABLE_FIELD_INT_10) && \
    !defined(ENABLE_FIELD_INT_11) && \
    !defined(ENABLE_FIELD_INT_12) && \
    !defined(ENABLE_FIELD_INT_13) && \
    !defined(ENABLE_FIELD_INT_14) && \
    !defined(ENABLE_FIELD_INT_15) && \
    !defined(ENABLE_FIELD_INT_16) && \
    !defined(ENABLE_FIELD_INT_17) && \
    !defined(ENABLE_FIELD_INT_18) && \
    !defined(ENABLE_FIELD_INT_19) && \
    !defined(ENABLE_FIELD_INT_20) && \
    !defined(ENABLE_FIELD_INT_21) && \
    !defined(ENABLE_FIELD_INT_22) && \
    !defined(ENABLE_FIELD_INT_23) && \
    !defined(ENABLE_FIELD_INT_24) && \
    !defined(ENABLE_FIELD_INT_25) && \
    !defined(ENABLE_FIELD_INT_26) && \
    !defined(ENABLE_FIELD_INT_27) && \
    !defined(ENABLE_FIELD_INT_28) && \
    !defined(ENABLE_FIELD_INT_29) && \
    !defined(ENABLE_FIELD_INT_30) && \
    !defined(ENABLE_FIELD_INT_31) && \
    !defined(ENABLE_FIELD_INT_32) && \
    !defined(ENABLE_FIELD_INT_33) && \
    !defined(ENABLE_FIELD_INT_34) && \
    !defined(ENABLE_FIELD_INT_35) && \
    !defined(ENABLE_FIELD_INT_36) && \
    !defined(ENABLE_FIELD_INT_37) && \
    !defined(ENABLE_FIELD_INT_38) && \
    !defined(ENABLE_FIELD_INT_39) && \
    !defined(ENABLE_FIELD_INT_40) && \
    !defined(ENABLE_FIELD_INT_41) && \
    !defined(ENABLE_FIELD_INT_42) && \
    !defined(ENABLE_FIELD_INT_43) && \
    !defined(ENABLE_FIELD_INT_44) && \
    !defined(ENABLE_FIELD_INT_45) && \
    !defined(ENABLE_FIELD_INT_46) && \
    !defined(ENABLE_FIELD_INT_47) && \
    !defined(ENABLE_FIELD_INT_48) && \
    !defined(ENABLE_FIELD_INT_49) && \
    !defined(ENABLE_FIELD_INT_50) && \
    !defined(ENABLE_FIELD_INT_51) && \
    !defined(ENABLE_FIELD_INT_52) && \
    !defined(ENABLE_FIELD_INT_53) && \
    !defined(ENABLE_FIELD_INT_54) && \
    !defined(ENABLE_FIELD_INT_55) && \
    !defined(ENABLE_FIELD_INT_56) && \
    !defined(ENABLE_FIELD_INT_57) && \
    !defined(ENABLE_FIELD_INT_58) && \
    !defined(ENABLE_FIELD_INT_59) && \
    !defined(ENABLE_FIELD_INT_60) && \
    !defined(ENABLE_FIELD_INT_61) && \
    !defined(ENABLE_FIELD_INT_62) && \
    !defined(ENABLE_FIELD_INT_63) && \
    !defined(ENABLE_FIELD_INT_64)
#error No fields enabled
#endif

#if defined(ENABLE_FIELD_INT_2) || \
    defined(ENABLE_FIELD_INT_3) || \
    defined(ENABLE_FIELD_INT_4) || \
    defined(ENABLE_FIELD_INT_5) || \
    defined(ENABLE_FIELD_INT_6) || \
    defined(ENABLE_FIELD_INT_7) || \
    defined(ENABLE_FIELD_INT_8)
#define ENABLE_FIELD_BYTES_INT_1
#endif

#if defined(ENABLE_FIELD_INT_9) || \
    defined(ENABLE_FIELD_INT_10) || \
    defined(ENABLE_FIELD_INT_11) || \
    defined(ENABLE_FIELD_INT_12) || \
    defined(ENABLE_FIELD_INT_13) || \
    defined(ENABLE_FIELD_INT_14) || \
    defined(ENABLE_FIELD_INT_15) || \
    defined(ENABLE_FIELD_INT_16)
#define ENABLE_FIELD_BYTES_INT_2
#endif

#if defined(ENABLE_FIELD_INT_17) || \
    defined(ENABLE_FIELD_INT_18) || \
    defined(ENABLE_FIELD_INT_19) || \
    defined(ENABLE_FIELD_INT_20) || \
    defined(ENABLE_FIELD_INT_21) || \
    defined(ENABLE_FIELD_INT_22) || \
    defined(ENABLE_FIELD_INT_23) || \
    defined(ENABLE_FIELD_INT_24)
#define ENABLE_FIELD_BYTES_INT_3
#endif

#if defined(ENABLE_FIELD_INT_25) || \
    defined(ENABLE_FIELD_INT_26) || \
    defined(ENABLE_FIELD_INT_27) || \
    defined(ENABLE_FIELD_INT_28) || \
    defined(ENABLE_FIELD_INT_29) || \
    defined(ENABLE_FIELD_INT_30) || \
    defined(ENABLE_FIELD_INT_31) || \
    defined(ENABLE_FIELD_INT_32)
#define ENABLE_FIELD_BYTES_INT_4
#endif

#if defined(ENABLE_FIELD_INT_33) || \
    defined(ENABLE_FIELD_INT_34) || \
    defined(ENABLE_FIELD_INT_35) || \
    defined(ENABLE_FIELD_INT_36) || \
    defined(ENABLE_FIELD_INT_37) || \
    defined(ENABLE_FIELD_INT_38) || \
    defined(ENABLE_FIELD_INT_39) || \
    defined(ENABLE_FIELD_INT_40)
#define ENABLE_FIELD_BYTES_INT_5
#endif

#if defined(ENABLE_FIELD_INT_41) || \
    defined(ENABLE_FIELD_INT_42) || \
    defined(ENABLE_FIELD_INT_43) || \
    defined(ENABLE_FIELD_INT_44) || \
    defined(ENABLE_FIELD_INT_45) || \
    defined(ENABLE_FIELD_INT_46) || \
    defined(ENABLE_FIELD_INT_47) || \
    defined(ENABLE_FIELD_INT_48)
#define ENABLE_FIELD_BYTES_INT_6
#endif

#if defined(ENABLE_FIELD_INT_49) || \
    defined(ENABLE_FIELD_INT_50) || \
    defined(ENABLE_FIELD_INT_51) || \
    defined(ENABLE_FIELD_INT_52) || \
    defined(ENABLE_FIELD_INT_53) || \
    defined(ENABLE_FIELD_INT_54) || \
    defined(ENABLE_FIELD_INT_55) || \
    defined(ENABLE_FIELD_INT_56)
#define ENABLE_FIELD_BYTES_INT_7
#endif

#if defined(ENABLE_FIELD_INT_57) || \
    defined(ENABLE_FIELD_INT_58) || \
    defined(ENABLE_FIELD_INT_59) || \
    defined(ENABLE_FIELD_INT_60) || \
    defined(ENABLE_FIELD_INT_61) || \
    defined(ENABLE_FIELD_INT_62) || \
    defined(ENABLE_FIELD_INT_63) || \
    defined(ENABLE_FIELD_INT_64)
#define ENABLE_FIELD_BYTES_INT_8
#endif
#endif // _MINISKETCH_FIELDDEFINES_H_
