include(CheckCSourceCompiles)

function(check_64bit_assembly)
  check_c_source_compiles("
    #include <stdint.h>

    int main()
    {
      uint64_t a = 11, tmp;
      __asm__ __volatile__(\"movq $0x100000000,%1; mulq %%rsi\" : \"+a\"(a) : \"S\"(tmp) : \"cc\", \"%rdx\");
    }
  " HAS_64BIT_ASM)
  set(HAS_64BIT_ASM ${HAS_64BIT_ASM} PARENT_SCOPE)
endfunction()
