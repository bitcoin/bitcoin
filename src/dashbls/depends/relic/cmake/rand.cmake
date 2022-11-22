message(STATUS "Available pseudo-random number generators (default = HASHD):\n")

message("   RAND=HASHD     Use the HASH-DRBG generator. (recommended)")
message("   RAND=RDRND     Use Intel RdRand instruction directly.")
message("   RAND=UDEV      Use the operating system underlying generator.")
message("   RAND=CALL      Override the generator with a callback.\n")

message(STATUS "Available random number generator seeders (default = UDEV):\n")

message("   SEED=          Use a zero seed. (horribly insecure!)")
message("   SEED=LIBC      Use rand()/random() functions. (insecure!)")
message("   SEED=RDRND     Use Intel RdRand instruction directly.")
message("   SEED=UDEV      Use non-blocking /dev/urandom. (recommended)")
message("   SEED=WCGR      Use Windows' CryptGenRandom. (recommended)\n")

# Choose the pseudo-random number generator.
set(RAND "HASHD" CACHE STRING "Pseudo-random number generator")

if(MSVC)

    # Choose the pseudo-random number generator.
    set(SEED "WCGR" CACHE STRING "Random number generator seeder")

else()

    # Choose the pseudo-random number generator.
    set(SEED "UDEV" CACHE STRING "Random number generator seeder")

endif()
