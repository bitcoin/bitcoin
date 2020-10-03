message(STATUS "Prime field arithmetic configuration (FP module):\n")

message("   ** Arithmetic precision of the prime field module (default = 256,0,off,off):\n")

message("      FP_PRIME=n        The prime modulus size in bits.")
message("      FP_KARAT=n        The number of Karatsuba levels.")
message("      FP_PMERS=[off|on] Prefer Pseudo-Mersenne primes over random primes.")
message("      FP_QNRES=[off|on] Use -1 as quadratic non-residue (make sure that p = 3 mod 8).")
message("      FP_WIDTH=w        Width w in [2,6] of window processing for exponentiation methods.\n")

message("   ** Available prime field arithmetic methods (default = BASIC;COMBA;COMBA;MONTY;MONTY;SLIDE):")

message("      Field addition")
message("      FP_METHD=BASIC    Schoolbook addition.")
message("      FP_METHD=INTEG    Integrated modular addition.\n")

message("      Field multiplication")
message("      FP_METHD=BASIC    Schoolbook multiplication.")
message("      FP_METHD=INTEG    Integrated modular multiplication.")
message("      FP_METHD=COMBA    Comba multiplication.\n")

message("      Field squaring")
message("      FP_METHD=BASIC    Schoolbook squaring.")
message("      FP_METHD=INTEG    Integrated modular squaring.")
message("      FP_METHD=COMBA    Comba squaring.")
message("      FP_METHD=MULTP    Reuse multiplication for squaring.\n")

message("      Modular reduction")
message("      FP_METHD=BASIC    Division-based reduction.")
message("      FP_METHD=QUICK    Fast reduction modulo special form prime (2^t - c, c > 0).")
message("      FP_METHD=MONTY    Montgomery modular reduction.\n")

message("      Field inversion")
message("      FP_METHD=BASIC    Inversion by Fermat's Little Theorem.")
message("      FP_METHD=BINAR    Binary Inversion algorithm.")
message("      FP_METHD=MONTY    Montgomery inversion.")
message("      FP_METHD=EXGCD    Inversion by the Extended Euclidean algorithm.")
message("      FP_METHD=LOWER    Pass inversion to the lower level.\n")

message("      Field exponentiation")
message("      FP_METHD=BASIC    Binary exponentiation.")
message("      FP_METHD=SLIDE    Sliding window exponentiation.")
message("      FP_METHD=MONTY    Constant-time Montgomery powering ladder.\n")

# Choose the prime field size.
if (NOT FP_PRIME)
	set(FP_PRIME 256)
endif(NOT FP_PRIME)
set(FP_PRIME ${FP_PRIME} CACHE INTEGER "Prime modulus size")

# Fix the number of Karatsuba instances
if (NOT FP_KARAT)
	set(FP_KARAT 0)
endif(NOT FP_KARAT)
set(FP_KARAT ${FP_KARAT} CACHE INTEGER "Number of Karatsuba levels.")

if (NOT FP_WIDTH)
	set(FP_WIDTH 4)
endif(NOT FP_WIDTH)
set(FP_WIDTH ${FP_WIDTH} CACHE INTEGER "Width of window processing for exponentiation methods.")

option(FP_PMERS "Prefer special form primes over random primes." off)
option(FP_QNRES "Use -1 as quadratic non-residue." off)

# Choose the arithmetic methods.
if (NOT FP_METHD)
	set(FP_METHD "BASIC;COMBA;COMBA;MONTY;MONTY;SLIDE")
endif(NOT FP_METHD)
list(LENGTH FP_METHD FP_LEN)
if (FP_LEN LESS 6)
	message(FATAL_ERROR "Incomplete FP_METHD specification: ${FP_METHD}")
endif(FP_LEN LESS 6)

list(GET FP_METHD 0 FP_ADD)
list(GET FP_METHD 1 FP_MUL)
list(GET FP_METHD 2 FP_SQR)
list(GET FP_METHD 3 FP_RDC)
list(GET FP_METHD 4 FP_INV)
list(GET FP_METHD 5 FP_EXP)
set(FP_METHD ${FP_METHD} CACHE STRING "Method for prime field arithmetic.")
