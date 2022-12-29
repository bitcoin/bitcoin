message(STATUS "Elliptic Edwards curve over prime fields arithmetic configuration (ED module):\n")

message("   ** Options for the prime elliptic Edwards curve module (default = all on):")
message("      ED_PRECO=[off|on] Build precomputation table for generator.")
message("      ED_DEPTH=w        Width w in [2,6] of precomputation table for fixed point methods.")
message("      ED_WIDTH=w        Width w in [2,6] of window processing for unknown point methods.\n")

message("   ** Available prime elliptic Edwards curve methods (default = PROJC;LWNAF;COMBS;INTER):")
message("      ED_METHD=BASIC    Affine coordinates.")
message("      EP_METHD=PROJC  	 Simple projective twisted Edwards coordinates.")
message("      EP_METHD=EXTND 	 Extended projective twisted Edwards coordinates.\n")

message("      *** variable-base multiplication method ***")
message("      ED_METHD=BASIC    Binary method.")
message("      ED_METHD=SLIDE    Sliding window method.")
message("      ED_METHD=MONTY    Montgomery ladder method.")
message("      ED_METHD=LWNAF    Left-to-right window NAF method.")
message("      EP_METHD=LWREG    Left-to-right regular recoding method (GLV for curves with endomorphisms).\n")

message("      *** fixed-base multiplication method ***")
message("      ED_METHD=BASIC    Binary method for fixed point multiplication.")
message("      ED_METHD=COMBS    Single-table Comb method for fixed point multiplication.")
message("      ED_METHD=COMBD    Double-table Comb method for fixed point multiplication.")
message("      ED_METHD=LWNAF    Left-to-right window NAF method.\n")

message("      *** variable-base simultaneous multiplication method ***")
message("      ED_METHD=BASIC    Multiplication-and-addition simultaneous multiplication.")
message("      ED_METHD=TRICK    Shamir's trick for simultaneous multiplication.")
message("      ED_METHD=INTER    Interleaving of window NAFs (GLV for Koblitz curves).")
message("      ED_METHD=JOINT    Joint sparse form.\n")

message("      Note: these methods must be given in order. Ex: ED_METHD=\"EXTND;LWNAF;COMBD;TRICK\"\n")

if (NOT ED_DEPTH)
	set(ED_DEPTH 4)
endif(NOT ED_DEPTH)
if (NOT ED_WIDTH)
	set(ED_WIDTH 4)
endif(NOT ED_WIDTH)
set(ED_DEPTH "${ED_DEPTH}" CACHE STRING "Width of precomputation table for fixed point methods.")
set(ED_WIDTH "${ED_WIDTH}" CACHE STRING "Width of window processing for unknown point methods.")

option(ED_PRECO "Build precomputation table for generator" on)

# Choose the arithmetic methods.
if (NOT ED_METHD)
	set(ED_METHD "PROJC;LWNAF;COMBS;INTER")
endif(NOT ED_METHD)
list(LENGTH ED_METHD ED_LEN)
if (ED_LEN LESS 4)
	message(FATAL_ERROR "Incomplete ED_METHD specification: ${ED_METHD}")
endif(ED_LEN LESS 4)

list(GET ED_METHD 0 ED_ADD)
list(GET ED_METHD 1 ED_MUL)
list(GET ED_METHD 2 ED_FIX)
list(GET ED_METHD 3 ED_SIM)
set(ED_METHD ${ED_METHD} CACHE STRING "Method for prime elliptic Edwards curve arithmetic.")
