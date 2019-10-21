#ifndef BITCOINTALKCOIN_SEMIORDEREDMAP_H
#define BITCOINTALKCOIN_SEMIORDEREDMAP_H

#include <stdint.h>
#include <math.h>

class semiOrderedMap
{
    private:

        uint64_t *indexOfBirthdayHashes;
        uint32_t *indexOfBirthdays;
        int bucketSizeExponent;
        int bucketSize;

    public:

        ~semiOrderedMap()
        {
            delete [] indexOfBirthdayHashes;
            delete [] indexOfBirthdays;
        }

        void allocate(int bSE)
        {
            bucketSizeExponent=bSE;
            bucketSize=pow(2.0,bSE);
            indexOfBirthdayHashes=new uint64_t[67108864];
            indexOfBirthdays=new uint32_t[67108864];
           
        }
        
        uint32_t checkAdd(uint64_t birthdayHash, uint32_t nonce)
        {
            uint64_t bucketStart = (birthdayHash >> (24+bucketSizeExponent))*bucketSize;
            for(int i=0;i<bucketSize;i++) 
            {
                uint64_t bucketValue=indexOfBirthdayHashes[bucketStart+i];
                if(bucketValue==birthdayHash)
                {
                    return indexOfBirthdays[bucketStart+i];
                }
                else if(bucketValue==0)
                {
                    indexOfBirthdayHashes[bucketStart+i]=birthdayHash;
                    indexOfBirthdays[bucketStart+i]=nonce;
                    return 0;
                }
            }
            return 0;
        }
};

#endif
