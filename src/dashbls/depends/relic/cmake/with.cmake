# Default modules.
set(WITH "ALL" CACHE STRING "Selected modules")
LIST(FIND WITH "ALL" TEMP)
if(TEMP GREATER -1)
	set(WITH_BN 1)
	set(WITH_DV 1)
	set(WITH_FP 1)
	set(WITH_FPX 1)
	set(WITH_FB 1)
	set(WITH_FBX 1)
	set(WITH_EP 1)
	set(WITH_EPX 1)
	set(WITH_EB 1)
	set(WITH_ED 1)
	set(WITH_EC 1)
	set(WITH_PP 1)
	set(WITH_PC 1)
	set(WITH_BC 1)
	set(WITH_MD 1)
	set(WITH_CP 1)
	set(WITH_MPC 1)
endif(TEMP GREATER -1)

# Check if multiple precision integer arithmetic is required.
list(FIND WITH "BN" TEMP)
if(TEMP GREATER -1)
	set(WITH_BN 1)
endif(TEMP GREATER -1)

# Check if temporary vectors are required.
list(FIND WITH "DV" TEMP)
if(TEMP GREATER -1)
	set(WITH_DV 1)
endif(TEMP GREATER -1)

# Check if prime field arithmetic is required.
list(FIND WITH "FP" TEMP)
if(TEMP GREATER -1)
	set(WITH_FP 1)
endif(TEMP GREATER -1)

# Check if prime extension field arithmetic is required.
list(FIND WITH "FPX" TEMP)
if(TEMP GREATER -1)
	set(WITH_FPX 1)
endif(TEMP GREATER -1)

# Check if binary field arithmetic is required.
list(FIND WITH "FB" TEMP)
if(TEMP GREATER -1)
	set(WITH_FB 1)
endif(TEMP GREATER -1)

# Check if binary extension field arithmetic is required.
list(FIND WITH "FBX" TEMP)
if(TEMP GREATER -1)
	set(WITH_FBX 1)
endif(TEMP GREATER -1)

# Check if prime elliptic curve support is required.
list(FIND WITH "EP" TEMP)
if(TEMP GREATER -1)
	set(WITH_EP 1)
endif(TEMP GREATER -1)

#Check if support for elliptic curves defined over prime field extensions is required.
list(FIND WITH "EPX" TEMP)
if (TEMP GREATER -1)
	set(WITH_EPX 1)
endif(TEMP GREATER -1)

# Check if binary elliptic curve support is required.
list(FIND WITH "EB" TEMP)
if(TEMP GREATER -1)
	set(WITH_EB 1)
endif(TEMP GREATER -1)

# Check if binary elliptic curve support is required.
list(FIND WITH "ED" TEMP)
if(TEMP GREATER -1)
	set(WITH_ED 1)
endif(TEMP GREATER -1)

# Check if elliptic curve cryptography support is required.
list(FIND WITH "EC" TEMP)
if(TEMP GREATER -1)
	set(WITH_EC 1)
endif(TEMP GREATER -1)

# Check if support for pairings over prime curves is required.
list(FIND WITH "PP" TEMP)
if(TEMP GREATER -1)
	set(WITH_PP 1)
endif(TEMP GREATER -1)

# Check if support for pairings over binary curves is required.
list(FIND WITH "PB" TEMP)
if(TEMP GREATER -1)
	set(WITH_PB 1)
endif(TEMP GREATER -1)

# Check if elliptic curve cryptography support is required.
list(FIND WITH "PC" TEMP)
if(TEMP GREATER -1)
	set(WITH_PC 1)
endif(TEMP GREATER -1)

# Check if support for block ciphers is required.
list(FIND WITH "BC" TEMP)
if(TEMP GREATER -1)
	set(WITH_BC 1)
endif(TEMP GREATER -1)

# Check if support for hash functions is required.
list(FIND WITH "MD" TEMP)
if(TEMP GREATER -1)
	set(WITH_MD 1)
endif(TEMP GREATER -1)

# Check if support for cryptographic protocols is required.
list(FIND WITH "CP" TEMP)
if(TEMP GREATER -1)
	set(WITH_CP 1)
endif(TEMP GREATER -1)

# Check if support for cryptographic protocols is required.
list(FIND WITH "MPC" TEMP)
if(TEMP GREATER -1)
	set(WITH_MPC 1)
endif(TEMP GREATER -1)
