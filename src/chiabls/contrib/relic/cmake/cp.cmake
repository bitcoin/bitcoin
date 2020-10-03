message(STATUS "Cryptographic protocols configuration (CP module):\n")

message("   ** Options for the cryptographic protocols module (default = PKCS1):\n")

message("      CP_RSAPD=BASIC    RSA with basic padding.")
message("      CP_RSAPD=PKCS1    RSA with PKCS#1 v1.5 padding.")
message("      CP_RSAPD=PKCS2    RSA with PKCS#1 v2.1 padding.\n")

message("   ** Available cryptographic protocols methods (default = QUICK;BASIC):\n")

message("      CP_METHD=BASIC    Slow RSA decryption/signature.")
message("      CP_METHD=QUICK    Fast RSA decryption/signature using CRT.\n")

if (NOT CP_RSAPD)
	set(CP_RSAPD "PKCS1")
endif(NOT CP_RSAPD)
set(CP_RSAPD ${CP_RSAPD} CACHE STRING "RSA padding")	

# Choose the methods.
if (NOT CP_METHD)
	set(CP_METHD "QUICK")
endif(NOT CP_METHD)
list(LENGTH CP_METHD CP_LEN)
if (CP_LEN LESS 1)
	message(FATAL_ERROR "Incomplete CP_METHD specification: ${CP_METHD}")
endif(CP_LEN LESS 1)

list(GET CP_METHD 0 CP_RSA)
set(CP_METHD ${CP_METHD} CACHE STRING "Method for cryptographic protocols.")
