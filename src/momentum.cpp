#include <momentum.h>
#include <semiOrderedMap.h>
#include <uint256.h>

#include <iostream>
//#include <openssl/sha.h>
#include <crypto/sha512.h>
#include <boost/thread.hpp>


#define MAX_MOMENTUM_NONCE  (1<<26)
#define SEARCH_SPACE_BITS 50
#define BIRTHDAYS_PER_HASH 8

std::vector< std::pair<uint32_t,uint32_t> > momentum_search( uint256 midHash )
{
   semiOrderedMap somap;
   somap.allocate(4);
   std::vector< std::pair<uint32_t,uint32_t> > results;
   char  hash_tmp[sizeof(midHash)+4];
   memcpy((char*)&hash_tmp[4], (char*)&midHash, sizeof(midHash) );
   uint32_t* index = (uint32_t*)hash_tmp;

   for( uint32_t i = 0; i < MAX_MOMENTUM_NONCE;  )
   {
	 if(i%1048576==0)
	 {
		boost::this_thread::interruption_point();
	 }
	 
	 *index = i;
	 //uint64_t  result_hash[8];
	 
	 //SHA512((unsigned char*)hash_tmp, sizeof(hash_tmp), (unsigned char*)&result_hash);

    unsigned char result_hash[CSHA512::OUTPUT_SIZE];
    CSHA512 di;
    di.Write((unsigned char*)hash_tmp, sizeof(hash_tmp));
    di.Finalize(result_hash);
    di.~CSHA512();
	 
	 for( uint32_t x = 0; x < BIRTHDAYS_PER_HASH; ++x )
	 { 
		uint64_t birthday = result_hash[x] >> (64-SEARCH_SPACE_BITS);           
		uint32_t nonce = i+x;   
		uint64_t foundMatch=somap.checkAdd( birthday, nonce );
		  if( foundMatch != 0 )
		  {
			   results.push_back( std::make_pair( foundMatch, nonce ) );
		  }
	 }
	 i += BIRTHDAYS_PER_HASH;
   }        
	   return results;
}   
 
uint64_t getBirthdayHash(const uint256& midHash, uint32_t a)
{
   uint32_t index = a - (a%8);
   char  hash_tmp[sizeof(midHash)+4];
   memcpy(&hash_tmp[4], (char*)&midHash, sizeof(midHash) );
   memcpy(&hash_tmp[0], (char*)&index, sizeof(index) ); 
   //uint64_t  result_hash[8];
   //SHA512((unsigned char*)hash_tmp, sizeof(hash_tmp), (unsigned char*)&result_hash);

    unsigned char result_hash[CSHA512::OUTPUT_SIZE];
    CSHA512 di;
    di.Write((unsigned char*)hash_tmp, sizeof(hash_tmp));
    di.Finalize(result_hash);
    di.~CSHA512();

   uint64_t r = result_hash[a%BIRTHDAYS_PER_HASH]>>(64-SEARCH_SPACE_BITS);
   return r;
}

bool momentum_verify( uint256 head, uint32_t a, uint32_t b )
{
   if( a == b ) return false;
   if( a > MAX_MOMENTUM_NONCE ) return false;
   if( b > MAX_MOMENTUM_NONCE ) return false;      

   bool r = (getBirthdayHash(head,a) == getBirthdayHash(head,b));

   return r;
}

