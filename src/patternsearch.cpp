#include <iostream>
#include <openssl/sha.h>
#include "patternsearch.h"
#include <openssl/aes.h>
#include "main.h"
#include <openssl/evp.h>
#include <boost/thread.hpp>
#if (__x86_64__)
#include <cpuid.h>
#endif
#include "aligned_malloc.h"
#include "util.h"
#include "sha512.h"
#include "aes.h"

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace patternsearch
{
	#define PSUEDORANDOM_DATA_SIZE 30 //2^30 = 1GB
    #define PSUEDORANDOM_DATA_CHUNK_SIZE 6 //2^6 = 64 bytes //must be same as SHA512_DIGEST_LENGTH 64
    #define L2CACHE_TARGET 12 // 2^12 = 4096 bytes
    #define AES_ITERATIONS 15
	#define GARBAGE_SIZE        (1 << 30)
	#define GARBAGE_CHUNK_SIZE  (1 << 6)
	#define GARBAGE_SLICE_SIZE  (1 << 12)
	#define TOTAL_CHUNKS        (1 << 24)       // GARBAGE_SIZE / GARBAGE_CHUNK_SIZE
	#define COMPARE_SIZE        (1 << 18)       // GARBAGE_SIZE / GARBAGE_SLICE_SIZE

	// useful constants
    #define psuedoRandomDataSize (1<<PSUEDORANDOM_DATA_SIZE) //2^30 = 1GB
    #define cacheMemorySize (1<<L2CACHE_TARGET) //2^12 = 4096 bytes
    #define chunks (1<<(PSUEDORANDOM_DATA_SIZE-PSUEDORANDOM_DATA_CHUNK_SIZE)) //2^(30-6) = 16 mil
    #define chunkSize (1<<(PSUEDORANDOM_DATA_CHUNK_SIZE)) //2^6 = 64 bytes
    #define comparisonSize (1<<(PSUEDORANDOM_DATA_SIZE-L2CACHE_TARGET)) //2^(30-12) = 256K

	typedef union _CacheEntry
	{
		uint32_t dwords[GARBAGE_SLICE_SIZE >> 2] __attribute__((aligned(16)));
	} CacheEntry;

	void static SHA512Filler(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads, uint256 midHash, int* minerStopFlag) {
		printf("using generic sha512\n");
		uint32_t chunksToProcess = chunks / totalThreads;
		uint32_t startChunk = threadNumber * chunksToProcess;
		uint32_t* midHash32 = (uint32_t*)&midHash;
		unsigned char *data = (unsigned char*) mainMemoryPsuedoRandomData;
		EVP_MD_CTX ctx;
		EVP_MD_CTX_init(&ctx);

        int remainder=0;
        if(threadNumber+1==totalThreads){
        //The last thread will also do the remainder
            remainder=chunks%totalThreads;
        }

		//for (uint32_t i = threadNumber; likely(i < chunks - totalThreads); i += totalThreads) {
        for (uint32_t i = startChunk; likely(i < startChunk + chunksToProcess+remainder); i++) {		// && *minerStopFlag == 0
			*midHash32 = i;
			//SHA512((unsigned char*)&midHash, sizeof(midHash), (unsigned char*)&(mainMemoryPsuedoRandomData[i * chunkSize]));
			EVP_DigestInit_ex(&ctx, EVP_sha512(), NULL);
			EVP_DigestUpdate(&ctx, &midHash, sizeof(midHash));
			EVP_DigestFinal_ex(&ctx, data + i * chunkSize, NULL);
		}
		EVP_MD_CTX_cleanup(&ctx);
	}

	void static SHA512FillerAVX2(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads, uint256 midHash, int* minerStopFlag) {
        CacheEntry *Garbage = (CacheEntry*)mainMemoryPsuedoRandomData;
        uint64_t* TempBufs[SHA512_PARALLEL_N];
        uint64_t* desination[SHA512_PARALLEL_N];

        for (int i=0; i<SHA512_PARALLEL_N; ++i) {
            TempBufs[i] = (uint64_t*)aligned_malloc(32);
            memcpy(TempBufs[i], (char*)&midHash, 32);
        }

        uint32_t StartChunk = threadNumber * (TOTAL_CHUNKS / totalThreads);
        for(uint32_t i = StartChunk; i < StartChunk + (TOTAL_CHUNKS / totalThreads); i+= SHA512_PARALLEL_N)
        {
            for(int j=0; j<SHA512_PARALLEL_N; ++j) {
                ((uint32_t*)TempBufs[j])[0] = i + j;
                desination[j] = (uint64_t*)((uint8_t *)Garbage + ((i+j) * GARBAGE_CHUNK_SIZE));
            }
            sha512Compute32b_parallel(TempBufs, desination);
        }

        for (int i=0; i<SHA512_PARALLEL_N; ++i) {
            aligned_free(TempBufs[i]);
        }
	}

	void static aesSearch(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads, std::vector< std::pair<uint32_t,uint32_t> > *results, boost::mutex *mtx, int* minerStopFlag){
		unsigned char* cache = (unsigned char*)aligned_malloc(sizeof(char)* (cacheMemorySize + 16));
		uint32_t* cache32 = (uint32_t*)cache;
		uint64_t* cache64 = (uint64_t*)cache;
		uint64_t* data64 = (uint64_t*)mainMemoryPsuedoRandomData;
		int outlen;
		uint32_t searchNumber = comparisonSize / totalThreads;
		uint32_t startLoc = threadNumber * searchNumber;
		uint32_t nextLocation;
		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);

        int remainder=0;
        if(threadNumber+1==totalThreads){
            //The last thread will also do the remainder
            remainder=chunks%totalThreads;
        }
        for (uint32_t k = startLoc; likely(k < startLoc + searchNumber + remainder && *minerStopFlag == 0); k++) {
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
		aligned_free(cache);
	}
	
	void static aesSearchAESNI(char *mainMemoryPsuedoRandomData, int threadNumber, int totalThreads, std::vector< std::pair<uint32_t,uint32_t> > *results, boost::mutex *mtx, int* minerStopFlag){
	    // start
	    CacheEntry* Garbage = (CacheEntry*)mainMemoryPsuedoRandomData;
	    CacheEntry* Cache = (CacheEntry*)aligned_malloc(sizeof(CacheEntry) * AES_PARALLEL_N);

	    uint32_t* data[AES_PARALLEL_N];
	    const uint32_t* next[AES_PARALLEL_N];

	    for(int n=0; n<AES_PARALLEL_N; ++n) {
	        data[n] = Cache[n].dwords;
	    }
	    uint32_t* ExpKey = (uint32_t*)aligned_malloc(sizeof(uint32_t) * AES_PARALLEL_N * 16 * 4);
	    uint32_t* ivs = (uint32_t*)aligned_malloc(sizeof(uint32_t) * AES_PARALLEL_N * 4);
	    uint32_t* last = (uint32_t*)aligned_malloc(sizeof(uint32_t) * 2 * 4);

	    // Search for pattern in pseudo random data
	    int searchNumber = COMPARE_SIZE / totalThreads;
	    int startLoc = threadNumber * searchNumber;

	    for(int32_t k = startLoc; k < startLoc + searchNumber; k+=AES_PARALLEL_N)
	    {
	        // copy data to first l2 cache
	        for (int n=0; n<AES_PARALLEL_N; ++n)
	            memcpy(Cache[n].dwords, Garbage + k + n, GARBAGE_SLICE_SIZE);

	        for(int j = 0; j < AES_ITERATIONS; ++j)
	        {

	            // use last 4 bytes of first cache as next location
	            for(int n=0; n<AES_PARALLEL_N; ++n) {
	                uint32_t nextLocation = Cache[n].dwords[(GARBAGE_SLICE_SIZE >> 2) - 1] & (COMPARE_SIZE - 1); //% COMPARE_SIZE;
	                next[n] = Garbage[nextLocation].dwords;

	                for(uint32_t i=0; i<8; ++i) {
	                	last[i] = Cache[n].dwords[254*4+i] ^ next[n][254*4+i];
	                }

	                // Key is last 32b of Cache
	                // IV is last 16b of Cache
	                ExpandAESKey256_int(&ExpKey[16*4*n], last);
	                for(int i=0; i<4; ++i) {
	                  ivs[n*4+i] = last[4+i];
	                }
	            }
	            AES256CBC_int(data, next, ExpKey, ivs);
	        }

	        // use last X bits as solution
	        for(int n=0; n<AES_PARALLEL_N; ++n)
	        if((Cache[n].dwords[(GARBAGE_SLICE_SIZE >> 2) - 1] & (COMPARE_SIZE - 1)) < 1000)
	        {
              uint32_t proofOfCalculation=Cache[n].dwords[(GARBAGE_SLICE_SIZE >> 2) - 2];
              boost::mutex::scoped_lock lck(*mtx);
              (*results).push_back( std::make_pair( k + n, proofOfCalculation ) );
	        }
	    }

	    aligned_free(last);
	    aligned_free(ivs);
	    aligned_free(ExpKey);
	    aligned_free(Cache);
	}


	std::vector< std::pair<uint32_t,uint32_t> > pattern_search(uint256 midHash, char *mainMemoryPsuedoRandomData, int totalThreads, int* minerStopFlag) {
		boost::this_thread::disable_interruption di;
		std::vector< std::pair<uint32_t,uint32_t> > results;

        bool aes_ni_supported = GetArg("-optimineraes", false);
        bool avx2_supported = GetArg("-optimineravx2", false);
        LogPrintf("Optiminer Flags: AES-NI = %b, AVX2 = %b\n", aes_ni_supported, avx2_supported);

#if (__x86_64__)
		uint32_t eax, ebx, ecx, edx;
		eax = ebx = ecx = edx = 0;
		__get_cpuid(1, &eax, &ebx, &ecx, &edx);
        aes_ni_supported = aes_ni_supported && (ecx & (1 << 25)) /* bit_AES */;
		if (__get_cpuid_max(0, NULL) >= 7) {
			__cpuid_count (7, 0, eax, ebx, ecx, edx);
            avx2_supported = avx2_supported && (ebx & (1 << 5)) /* bit_AVX2 */;
		}
#else
        aes_ni_supported=false;
        avx2_supported=false;
        LogPrintf("32 bit compile, neither optimineraes nor optimineravx2 can be used\n");
#endif
        LogPrintf("Optiminer Flags and CPU support: AES-NI = %b, AVX2 = %b\n", aes_ni_supported, avx2_supported);

        clock_t t1 = clock();
		boost::thread_group* sha512Threads = new boost::thread_group();
		for (int i = 0; i < totalThreads; i++) {
			if (avx2_supported) {
				sha512Threads->create_thread(boost::bind(&SHA512FillerAVX2, mainMemoryPsuedoRandomData, i, totalThreads, midHash, minerStopFlag));
			} else {
				sha512Threads->create_thread(boost::bind(&SHA512Filler, mainMemoryPsuedoRandomData, i, totalThreads, midHash, minerStopFlag));
			}
		}
		sha512Threads->join_all();
		delete sha512Threads;

		clock_t t2 = clock();
		LogPrintf("create sha512 data %d\n", ((double)t2 - (double)t1) / CLOCKS_PER_SEC);

		if (*minerStopFlag == 0) {
			boost::mutex mtx;
			boost::thread_group* aesThreads = new boost::thread_group();
			for (int i = 0; i < totalThreads; i++) {
				if (aes_ni_supported) {
					aesThreads->create_thread(boost::bind(&aesSearchAESNI, mainMemoryPsuedoRandomData, i, totalThreads, &results, &mtx, minerStopFlag));
				} else {
					aesThreads->create_thread(boost::bind(&aesSearch, mainMemoryPsuedoRandomData, i, totalThreads, &results, &mtx, minerStopFlag));
				}
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
