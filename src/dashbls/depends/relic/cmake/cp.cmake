message(STATUS "Cryptographic protocols configuration (CP module):\n")

message("   ** Options for the cryptographic protocols module (default = on, PKCS2):\n")

message("      CP_CRT=[off|on] Support for faster CRT-based exponentiation in factoring-based cryptosystems.\n")

message("      CP_RSAPD=BASIC    RSA with basic padding.")
message("      CP_RSAPD=PKCS1    RSA with PKCS#1 v1.5 padding.")
message("      CP_RSAPD=PKCS2    RSA with PKCS#1 v2.1 padding.\n")

if (NOT CP_RSAPD)
	set(CP_RSAPD "PKCS2")
endif(NOT CP_RSAPD)
set(CP_RSAPD ${CP_RSAPD} CACHE STRING "RSA padding")

option(CP_CRT "Support for faster CRT-based exponentiation in factoring-based cryptosystems." on)
