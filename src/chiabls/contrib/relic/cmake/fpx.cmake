message(STATUS "Prime extension field arithmetic configuration (FPX module):\n")

message("   ** Available bilinear pairing methods (default = BASIC;BASIC;BASIC):")

message("      Quadratic extension arithmetic:")
message("      FPX_METHD=BASIC    Basic quadratic extension field arithmetic.")    
message("      FPX_METHD=INTEG    Quadratic extension field arithmetic with embedded modular reduction.\n")

message("      Cubic extension arithmetic:")
message("      FPX_METHD=BASIC    Basic cubic extension field arithmetic.")    
message("      FPX_METHD=INTEG    Cubic extension field arithmetic with embedded modular reduction.\n")

message("      Extension field arithmetic:")
message("      FPX_METHD=BASIC    Basic extension field arithmetic.")    
message("      FPX_METHD=LAZYR    Lazy-reduced extension field arithmetic.\n")

# Choose the arithmetic methods.
if (NOT FPX_METHD)
	set(FPX_METHD "BASIC;BASIC;BASIC")
endif(NOT FPX_METHD)
list(LENGTH FPX_METHD FPX_LEN)
if (FPX_LEN LESS 3)
	message(FATAL_ERROR "Incomplete FPX_METHD specification: ${FPX_METHD}")
endif(FPX_LEN LESS 3)

list(GET FPX_METHD 0 FPX_QDR)
list(GET FPX_METHD 1 FPX_CBC)
list(GET FPX_METHD 2 FPX_RDC)
set(FPX_METHD ${FPX_METHD} CACHE STRING "Method for prime extension field arithmetic.")
