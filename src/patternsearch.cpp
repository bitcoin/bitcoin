#include <boost/unordered_map.hpp>
#include <iostream>
#include <openssl/sha.h>
#include "patternsearch.h"
#include <openssl/aes.h>
#include "main.h"
#include <openssl/evp.h>
#include <boost/thread.hpp>
#include "util.h"

namespace patternsearch
{
	#define PSUEDORANDOM_DATA_SIZE 30 //2^30 = 1GB
    #define PSUEDORANDOM_DATA_CHUNK_SIZE 6 //2^6 = 64 bytes //must be same as SHA512_DIGEST_LENGTH 64
    #define L2CACHE_TARGET 12 // 2^12 = 4096 bytes
    #define AES_ITERATIONS 15

	// useful constants
    uint32_t psuedoRandomDataSize=(1<<PSUEDORANDOM_DATA_SIZE); //2^30 = 1GB
    uint32_t cacheMemorySize = (1<<L2CACHE_TARGET); //2^12 = 4096 bytes
    uint32_t chunks=(1<<(PSUEDORANDOM_DATA_SIZE-PSUEDORANDOM_DATA_CHUNK_SIZE)); //2^(30-6) = 16 mil
    uint32_t chunkSize=(1<<(PSUEDORANDOM_DATA_CHUNK_SIZE)); //2^6 = 64 bytes
    uint32_t comparisonSize=(1<<(PSUEDORANDOM_DATA_SIZE-L2CACHE_TARGET)); //2^(30-12) = 256K
	
	void static SHA512Filler(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads,uint256 midHash){
		//Generate psuedo random data to store in main memory
		unsigned char hash_tmp[sizeof(midHash)];
		memcpy((char*)&hash_tmp[0], (char*)&midHash, sizeof(midHash) );
		uint32_t* index = (uint32_t*)hash_tmp;
		uint32_t chunksToProcess=chunks/totalThreads;
		uint32_t startChunk=threadNumber*chunksToProcess;
		for( uint32_t i = startChunk; i < startChunk+chunksToProcess;  i++){
            //This changes the first character of hash_tmp
			*index = i;
            SHA512((unsigned char*)hash_tmp, sizeof(hash_tmp), (unsigned char*)&(mainMemoryPsuedoRandomData[i*chunkSize]));
		}
	}
	
	void static aesSearch(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads, std::vector< std::pair<uint32_t,uint32_t> > *results, boost::mutex *mtx){
		//Allocate temporary memory
		unsigned char *cacheMemoryOperatingData;
		unsigned char *cacheMemoryOperatingData2;	
		cacheMemoryOperatingData=new unsigned char[cacheMemorySize+16];
		cacheMemoryOperatingData2=new unsigned char[cacheMemorySize];
	
		//Create references to data as 32 bit arrays
		uint32_t* cacheMemoryOperatingData32 = (uint32_t*)cacheMemoryOperatingData;
		uint32_t* cacheMemoryOperatingData322 = (uint32_t*)cacheMemoryOperatingData2;
        //uint32_t* mainMemoryPsuedoRandomData32 = (uint32_t*)mainMemoryPsuedoRandomData;
		
		//Search for pattern in psuedorandom data
		
		unsigned char key[32] = {0};
		unsigned char iv[AES_BLOCK_SIZE];
		int outlen1, outlen2;
        unsigned int useEVP = GetArg("-useevp", 1);
		
		//Iterate over the data
		int searchNumber=comparisonSize/totalThreads;
		int startLoc=threadNumber*searchNumber;
		for(uint32_t k=startLoc;k<startLoc+searchNumber;k++){
			
            //copy data to first l2 cache
			memcpy((char*)&cacheMemoryOperatingData[0], (char*)&mainMemoryPsuedoRandomData[k*cacheMemorySize], cacheMemorySize);
			
			for(int j=0;j<AES_ITERATIONS;j++){

                //use last 4 bytes of first cache as next location
				uint32_t nextLocation = cacheMemoryOperatingData32[(cacheMemorySize/4)-1]%comparisonSize;

				//Copy data from indicated location to second l2 cache -
				memcpy((char*)&cacheMemoryOperatingData2[0], (char*)&mainMemoryPsuedoRandomData[nextLocation*cacheMemorySize], cacheMemorySize);

				//XOR location data into second cache
				for(uint32_t i = 0; i < cacheMemorySize/4; i++){
					cacheMemoryOperatingData322[i] = cacheMemoryOperatingData32[i] ^ cacheMemoryOperatingData322[i];
				}

				//AES Encrypt using last 256bits of Xorred value as key
				//AES_set_encrypt_key((unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-32], 256, &AESkey);
				
				//Use last X bits as initial vector
				
				//AES CBC encrypt data in cache 2, place it into cache 1, ready for the next round
				//AES_cbc_encrypt((unsigned char*)&cacheMemoryOperatingData2[0], (unsigned char*)&cacheMemoryOperatingData[0], cacheMemorySize, &AESkey, iv, AES_ENCRYPT);
				if(useEVP){
					EVP_CIPHER_CTX ctx;
					memcpy(key,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-32],32);
					memcpy(iv,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-AES_BLOCK_SIZE],AES_BLOCK_SIZE);
					EVP_EncryptInit(&ctx, EVP_aes_256_cbc(), key, iv);
					EVP_EncryptUpdate(&ctx, cacheMemoryOperatingData, &outlen1, cacheMemoryOperatingData2, cacheMemorySize);
					EVP_EncryptFinal(&ctx, cacheMemoryOperatingData + outlen1, &outlen2);
					EVP_CIPHER_CTX_cleanup(&ctx);
				}else{
					AES_KEY AESkey;
					AES_set_encrypt_key((unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-32], 256, &AESkey);			
					memcpy(iv,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-AES_BLOCK_SIZE],AES_BLOCK_SIZE);
					AES_cbc_encrypt((unsigned char*)&cacheMemoryOperatingData2[0], (unsigned char*)&cacheMemoryOperatingData[0], cacheMemorySize, &AESkey, iv, AES_ENCRYPT);
				}
			}
			
			//use last X bits as solution
			uint32_t solution=cacheMemoryOperatingData32[(cacheMemorySize/4)-1]%comparisonSize;
            if(solution<1000){
				uint32_t proofOfCalculation=cacheMemoryOperatingData32[(cacheMemorySize/4)-2];
                //LogPrintf("found solution - %d / %u / %u\n",k,solution,proofOfCalculation);
				boost::mutex::scoped_lock lck(*mtx);
				(*results).push_back( std::make_pair( k, proofOfCalculation ) );
			}
		}
		
		//free memory
		delete [] cacheMemoryOperatingData;
		delete [] cacheMemoryOperatingData2;
	}
	
    std::vector< std::pair<uint32_t,uint32_t> > pattern_search( uint256 midHash, char *mainMemoryPsuedoRandomData, int totalThreads){
		
        //LogPrintf("Start Search\n");
		
		std::vector< std::pair<uint32_t,uint32_t> > results;
		
        //clock_t t1 = clock();
		boost::thread_group* sha512Threads = new boost::thread_group();
		char *threadsComplete;
		threadsComplete=new char[totalThreads];
		for (int i = 0; i < totalThreads; i++){
			sha512Threads->create_thread(boost::bind(&SHA512Filler, mainMemoryPsuedoRandomData, i,totalThreads,midHash));
		}
		//Wait for all threads to complete
		sha512Threads->join_all();
		
        //clock_t t2 = clock();
        //LogPrintf("create sha512 data %d\n",((double)t2-(double)t1)/CLOCKS_PER_SEC);

		boost::mutex mtx;
		boost::thread_group* aesThreads = new boost::thread_group();
		threadsComplete=new char[totalThreads];
		for (int i = 0; i < totalThreads; i++){
			aesThreads->create_thread(boost::bind(&aesSearch, mainMemoryPsuedoRandomData, i,totalThreads,&results, &mtx));
		}
		//Wait for all threads to complete
		aesThreads->join_all();

        //clock_t t3 = clock();
        //LogPrintf("aes search %d\n",((double)t3-(double)t2)/CLOCKS_PER_SEC);

		delete aesThreads;
		delete sha512Threads;
		return results;
	}
	
	
	
    bool pattern_verify( uint256 midHash, uint32_t a, uint32_t b ){
		//return false;
		
		clock_t t1 = clock();
		
		//Basic check
        if( a >= comparisonSize ) return false;
		
		//Allocate memory required
		unsigned char *cacheMemoryOperatingData;
		unsigned char *cacheMemoryOperatingData2;	
		cacheMemoryOperatingData=new unsigned char[cacheMemorySize+16];
		cacheMemoryOperatingData2=new unsigned char[cacheMemorySize];
		uint32_t* cacheMemoryOperatingData32 = (uint32_t*)cacheMemoryOperatingData;
		uint32_t* cacheMemoryOperatingData322 = (uint32_t*)cacheMemoryOperatingData2;
		
		unsigned char  hash_tmp[sizeof(midHash)];
		memcpy((char*)&hash_tmp[0], (char*)&midHash, sizeof(midHash) );
		uint32_t* index = (uint32_t*)hash_tmp;
		
		uint32_t startLocation=a*cacheMemorySize/chunkSize;
		uint32_t finishLocation=startLocation+(cacheMemorySize/chunkSize);
			
        //copy data to first l2 cache
		for( uint32_t i = startLocation; i <  finishLocation;  i++){
			*index = i;
			SHA512((unsigned char*)hash_tmp, sizeof(hash_tmp), (unsigned char*)&(cacheMemoryOperatingData[(i-startLocation)*chunkSize]));
		}
		
        unsigned int useEVP = GetArg("-useevp", 1);

        //allow override for AESNI testing
        /*if(midHash==0){
            useEVP=0;
        }else if(midHash==1){
            useEVP=1;
        }*/

        unsigned char key[32] = {0};
		unsigned char iv[AES_BLOCK_SIZE];
		int outlen1, outlen2;
		
		//memset(cacheMemoryOperatingData2,0,cacheMemorySize);
		for(int j=0;j<AES_ITERATIONS;j++){
			
			//use last 4 bits as next location
			startLocation = (cacheMemoryOperatingData32[(cacheMemorySize/4)-1]%comparisonSize)*cacheMemorySize/chunkSize;
			finishLocation=startLocation+(cacheMemorySize/chunkSize);
			for( uint32_t i = startLocation; i <  finishLocation;  i++){
				*index = i;
				SHA512((unsigned char*)hash_tmp, sizeof(hash_tmp), (unsigned char*)&(cacheMemoryOperatingData2[(i-startLocation)*chunkSize]));
			}

			//XOR location data into second cache
			for(uint32_t i = 0; i < cacheMemorySize/4; i++){
				cacheMemoryOperatingData322[i] = cacheMemoryOperatingData32[i] ^ cacheMemoryOperatingData322[i];
			}
				
			//AES Encrypt using last 256bits as key
			
			if(useEVP){
				EVP_CIPHER_CTX ctx;
				memcpy(key,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-32],32);
				memcpy(iv,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-AES_BLOCK_SIZE],AES_BLOCK_SIZE);
				EVP_EncryptInit(&ctx, EVP_aes_256_cbc(), key, iv);
				EVP_EncryptUpdate(&ctx, cacheMemoryOperatingData, &outlen1, cacheMemoryOperatingData2, cacheMemorySize);
				EVP_EncryptFinal(&ctx, cacheMemoryOperatingData + outlen1, &outlen2);
				EVP_CIPHER_CTX_cleanup(&ctx);
			}else{
				AES_KEY AESkey;
				AES_set_encrypt_key((unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-32], 256, &AESkey);			
				memcpy(iv,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-AES_BLOCK_SIZE],AES_BLOCK_SIZE);
				AES_cbc_encrypt((unsigned char*)&cacheMemoryOperatingData2[0], (unsigned char*)&cacheMemoryOperatingData[0], cacheMemorySize, &AESkey, iv, AES_ENCRYPT);
			}
			
		}
			
		//use last X bits as solution
		uint32_t solution=cacheMemoryOperatingData32[(cacheMemorySize/4)-1]%comparisonSize;
		uint32_t proofOfCalculation=cacheMemoryOperatingData32[(cacheMemorySize/4)-2];
        //LogPrintf("verify solution - %d / %u / %u\n",a,solution,proofOfCalculation);
		
		//free memory
		delete [] cacheMemoryOperatingData;
		delete [] cacheMemoryOperatingData2;		

        clock_t t2 = clock();
        //LogPrintf("verify %d\n",((double)t2-(double)t1)/CLOCKS_PER_SEC);

        if(solution<1000 && proofOfCalculation==b){
			return true;
		}
		
		return false;

	}

    // http://blog.paphus.com/blog/2012/07/24/runtime-cpu-feature-checking/
    void cpuid(unsigned info, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
      {
        *eax = info;
        __asm volatile
          ("mov %%ebx, %%edi;" /* 32bit PIC: don't clobber ebx */
           "cpuid;"
           "mov %%ebx, %%esi;"
           "mov %%edi, %%ebx;"
           :"+a" (*eax), "=S" (*ebx), "=c" (*ecx), "=d" (*edx)
           : :"edi");
    }

    bool hasAESNIInstructions(){
        unsigned int eax, ebx, ecx, edx;
        cpuid(1, &eax, &ebx, &ecx, &edx);
        return ((edx & 0x2000000) != 0);
    }

}
