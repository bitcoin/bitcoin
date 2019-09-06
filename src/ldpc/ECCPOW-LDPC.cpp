// ECCPOW-LDPC.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "LDPC.h"
#include <string>
#include <iostream>

using namespace std;

int main()
{
  char phv[32] = "abc255dff722ddddffff3ffff1";
  unsigned int nonce = 0;
  string current_block_header = "1fdf22ffc2233ff";

  LDPC *ldpc = new LDPC;
  int level = 103;
  ldpc->set_difficulty(level);
  printf("level : %d, difficulty : %lf\n", level, ptr->get_ldpc_difficulty(level));
  printf("n : %d\t wc : %d\t wr : %d\n", ldpc_level_table[level].n, ldpc_level_table[level].wc, ldpc_level_table[level].wr);

  ldpc->initialization();
  std::cout << "SEED:" << phv << std::endl;

  ldpc->generate_seeds(3423423423423424237);
  ldpc->generate_H();
  ldpc->generate_Q();


  string current_block_header_with_nonce;
  while (1)
  {
    // std::cout << nonce << std::endl;
    current_block_header_with_nonce.assign(current_block_header);
    current_block_header_with_nonce += to_string(nonce);

    ldpc->generate_hv((unsigned char*)current_block_header_with_nonce.c_str());
    ldpc->decoding();
    if (ldpc->decision())
    {
      printf("codeword is founded with nonce = %d\n", nonce);
      break;
    }

    nonce++;
  }
  delete ldpc;
  return 0;
}

