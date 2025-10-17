include(CheckCSourceCompiles)

function(check_x86_64_assembly)
  check_c_source_compiles("
    #include <stdint.h>

    int main(void)
    {
      uint64_t a = 11, tmp = 0;
      __asm__ __volatile__(\"movq $0x100000000,%1; mulq %%rsi\" : \"+a\"(a) : \"S\"(tmp) : \"cc\", \"%rdx\");
      return 0;
    }
  " HAVE_X86_64_ASM)
  set(HAVE_X86_64_ASM ${HAVE_X86_64_ASM} PARENT_SCOPE)
endfunction()
