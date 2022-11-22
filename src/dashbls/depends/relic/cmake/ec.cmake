message(STATUS "Elliptic curve cryptography configuration (EC module):\n")

message("   ** Options for the binary elliptic curve module (default = on):\n")
message("      EC_ENDOM=[off|on] Prefer (prime or binary) curves with endomorphisms.\n")

message("   ** Available elliptic curve methods (default = PRIME):\n")
message("      EC_METHD=PRIME    Use prime curves.")
message("      EC_METHD=CHAR2    Use binary curves.")
message("      EC_METHD=EDDIE    Use prime Edwards curves.\n")

option(EC_ENDOM "Prefer (prime or binary) curves with endomorphisms" off)

# Choose the arithmetic methods.
if (NOT EC_METHD)
	set(EC_METHD "PRIME")
endif(NOT EC_METHD)
list(LENGTH EC_METHD EC_LEN)
if (EC_LEN LESS 1)
	message(FATAL_ERROR "Incomplete EC_METHD specification: ${EC_METHD}")
endif(EC_LEN LESS 1)

list(GET EC_METHD 0 EC_CUR)
set(EC_METHD ${EC_METHD} CACHE STRING "Method for Elliptic Curve Cryptography.")
