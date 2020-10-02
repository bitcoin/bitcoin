message(STATUS "Message digest configuration (MD module):\n")

message("   ** Available hash functions (default = SH256):\n")
message("      MD_METHD=SHONE        SHA-1 hash function.")
message("      MD_METHD=SH224        SHA-224 hash function.")
message("      MD_METHD=SH256        SHA-256 hash function.")
message("      MD_METHD=SH384        SHA-384 hash function.")
message("      MD_METHD=SH512        SHA-512 hash function.")
message("      MD_METHD=B2S160       BLAKE2s-160 hash function.")
message("      MD_METHD=B2S256       BLAKE2s-256 hash function.\n")

# Choose the arithmetic methods.
if (NOT MD_METHD)
	set(MD_METHD "SH256")
endif(NOT MD_METHD)
list(LENGTH MD_METHD MD_LEN)
if (MD_LEN LESS 1)
	message(FATAL_ERROR "Incomplete MD_METHD specification: ${MD_METHD}")
endif(MD_LEN LESS 1)

list(GET MD_METHD 0 MD_MAP)
set(MD_METHD ${MD_METHD} CACHE STRING "Method for hash functions.")
