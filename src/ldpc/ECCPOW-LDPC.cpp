// ECCPOW-LDPC.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "LDPC.h"
#include <string>
using namespace std;

int main()
{
  char phv[32] = "abc255dff722ddddffff3ffff1";
  unsigned int nonce = 0;
  string current_block_header = "1fdf22ffc2233ff";

  LDPC *ptr = new LDPC;

  ptr->set_difficulty(24,3,6);        //2 => n = 64, wc = 3, wr = 6,
  if (!ptr->initialization())
  {
    printf("error for calling the initialization function");
    return 0;
  }

  ptr->generate_seed(phv);
  ptr->generate_H();
  ptr->generate_Q();

  // ptr->print_H("H2.txt");
  ptr->print_Q(NULL, 1);
  ptr->print_Q(NULL, 2);


  while (1)
  {
    string current_block_header_with_nonce;
    current_block_header_with_nonce.assign(current_block_header);
    current_block_header_with_nonce += to_string(nonce);

    ptr->generate_hv((unsigned char*)current_block_header_with_nonce.c_str());
    ptr->decoding();
    if (ptr->decision())
    {
      printf("codeword is founded with nonce = %d\n", nonce);
      break;
    }

    nonce++;
  }


  ptr->print_word(NULL, 1);
  ptr->print_word(NULL, 2);
  delete ptr;
  return 0;
}

