#include <iostream>
#include <openssl/sha.h>
#include "patternsearch.h"
#include <openssl/aes.h>
#include "main.h"
#include <openssl/evp.h>
#include <boost/thread.hpp>
#include "util.h"

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace patternsearch
{
	#define PSUEDORANDOM_DATA_SIZE 30 //2^30 = 1GB
    #define PSUEDORANDOM_DATA_CHUNK_SIZE 6 //2^6 = 64 bytes //must be same as SHA512_DIGEST_LENGTH 64
    #define L2CACHE_TARGET 12 // 2^12 = 4096 bytes
    #define AES_ITERATIONS 15

	// useful constants
    #define psuedoRandomDataSize (1<<PSUEDORANDOM_DATA_SIZE) //2^30 = 1GB
    #define cacheMemorySize (1<<L2CACHE_TARGET) //2^12 = 4096 bytes
    #define chunks (1<<(PSUEDORANDOM_DATA_SIZE-PSUEDORANDOM_DATA_CHUNK_SIZE)) //2^(30-6) = 16 mil
    #define chunkSize (1<<(PSUEDORANDOM_DATA_CHUNK_SIZE)) //2^6 = 64 bytes
    #define comparisonSize (1<<(PSUEDORANDOM_DATA_SIZE-L2CACHE_TARGET)) //2^(30-12) = 256K


	void static SHA512Filler(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads, uint256 midHash, int* minerStopFlag) {
		uint32_t chunksToProcess = chunks / totalThreads;
		uint32_t startChunk = threadNumber * chunksToProcess;
		uint32_t* midHash32 = (uint32_t*)&midHash;
		unsigned char *data = (unsigned char*) mainMemoryPsuedoRandomData;
		EVP_MD_CTX ctx;
		EVP_MD_CTX_init(&ctx);
		
		//for (uint32_t i = threadNumber; likely(i < chunks - totalThreads); i += totalThreads) {
		for (uint32_t i = startChunk; likely(i < startChunk + chunksToProcess); i++) {		// && *minerStopFlag == 0
			*midHash32 = i;
      //SHA512((unsigned char*)&midHash, sizeof(midHash), (unsigned char*)&(mainMemoryPsuedoRandomData[i * chunkSize]));
			EVP_DigestInit_ex(&ctx, EVP_sha512(), NULL);
			EVP_DigestUpdate(&ctx, &midHash, sizeof(midHash));
			EVP_DigestFinal_ex(&ctx, data + i * chunkSize, NULL);
		}
		EVP_MD_CTX_cleanup(&ctx);
	}


	void static aesSearch(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads, std::vector< std::pair<uint32_t,uint32_t> > *results, boost::mutex *mtx, int* minerStopFlag){
		unsigned char cache[cacheMemorySize + 16] __attribute__ ((aligned (16)));
		uint32_t* cache32 = (uint32_t*)cache;
		uint64_t* cache64 = (uint64_t*)cache;
		uint64_t* data64 = (uint64_t*)mainMemoryPsuedoRandomData;
		int outlen;
		uint32_t searchNumber = comparisonSize / totalThreads;
		uint32_t startLoc = threadNumber * searchNumber;
		uint32_t nextLocation;
		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);

		//for (uint32_t k = threadNumber; likely(k < comparisonSize - totalThreads); k += totalThreads) {
		for (uint32_t k = startLoc; likely(k < startLoc + searchNumber && *minerStopFlag == 0); k++) {
			memcpy((char*)cache, mainMemoryPsuedoRandomData + k * cacheMemorySize, cacheMemorySize);

			for(unsigned char j = 0; j < AES_ITERATIONS; j++) {
				nextLocation = cache32[(cacheMemorySize / 4) - 1] % comparisonSize;
				for(uint32_t i = 0; i < cacheMemorySize / 8; i++) cache64[i] ^= data64[nextLocation * cacheMemorySize / 8 + i];

				EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, cache + cacheMemorySize - 32, cache + cacheMemorySize - AES_BLOCK_SIZE);
				EVP_EncryptUpdate(&ctx, cache, &outlen, cache, cacheMemorySize);
				//EVP_EncryptFinal_ex(&ctx, cache + outlen, &outlen);
			}

      if (unlikely(cache32[cacheMemorySize / 4 - 1] % comparisonSize < 1000)) {		// check solution
				boost::mutex::scoped_lock lck(*mtx);
				(*results).push_back(std::make_pair(k, cache32[cacheMemorySize / 4 - 2]));	// set proof of calculation
			}
		}
		EVP_CIPHER_CTX_cleanup(&ctx);
	}


  std::vector< std::pair<uint32_t,uint32_t> > pattern_search(uint256 midHash, char *mainMemoryPsuedoRandomData, int totalThreads, int* minerStopFlag) {
    boost::this_thread::disable_interruption di;
		std::vector< std::pair<uint32_t,uint32_t> > results;

    clock_t t1 = clock();
		boost::thread_group* sha512Threads = new boost::thread_group();
		for (int i = 0; i < totalThreads; i++) {
			sha512Threads->create_thread(boost::bind(&SHA512Filler, mainMemoryPsuedoRandomData, i, totalThreads, midHash, minerStopFlag));
		}
		sha512Threads->join_all();
		delete sha512Threads;

    clock_t t2 = clock();
    LogPrintf("create sha512 data %d\n", ((double)t2 - (double)t1) / CLOCKS_PER_SEC);

		if (*minerStopFlag == 0) {
			boost::mutex mtx;
			boost::thread_group* aesThreads = new boost::thread_group();
			for (int i = 0; i < totalThreads; i++) {
				aesThreads->create_thread(boost::bind(&aesSearch, mainMemoryPsuedoRandomData, i, totalThreads, &results, &mtx, minerStopFlag));
			}
			aesThreads->join_all();
			delete aesThreads;

			clock_t t3 = clock();
			LogPrintf("aes search %d\n", ((double)t3 - (double)t2) / CLOCKS_PER_SEC);
		}

    boost::this_thread::restore_interruption ri(di);
		return results;
	}



  bool pattern_verify( uint256 midHash, uint32_t a, uint32_t b ){
		//return false;

		//clock_t t1 = clock();

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

        //clock_t t2 = clock();
        //LogPrintf("verify %d\n",((double)t2-(double)t1)/CLOCKS_PER_SEC);

        if(solution<1000 && proofOfCalculation==b){
			return true;
		}

		return false;

	}
}
