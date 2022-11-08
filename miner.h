#ifndef __MINER_H_
#define __MINER_H_
#include <omp.h>
#include <openssl/evp.h>
#include "block.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

std::string sha256(const std::string& unhashed)
{
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if(context == NULL)
    {
        std::cerr<<"Error on EVP_MD_CTX_new"<<std::endl;
        exit(0);
    }

    if(EVP_DigestInit_ex(context, EVP_sha256(), NULL) != 1)
    {
        std::cerr<<"Error on EVP_DigestInit_ex"<<std::endl;
        exit(0);
    }
    if(EVP_DigestUpdate(context, unhashed.c_str(), unhashed.length()) != 1)
    {
        std::cerr<<"Error on EVP_DigestUpdate"<<std::endl;
        exit(0);
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if(EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1)
    {
        std::cerr<<"Error on EVP_DigestFinal_ex"<<std::endl;
        exit(0);
    }
    std::stringstream ss;
    for(unsigned int i = 0; i < lengthOfHash; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    EVP_MD_CTX_free(context);

    return ss.str();
}

class Miner
{
    public:
        Miner(int difficulty)
        {
            std::stringstream ss;
            for(int i=0;i<difficulty;i++)
                ss<<'0';
            this->zeros = ss.str();
        }
        
        Block* mine(Block* block) const
        {
            // create a copy of the block
            Block mined = Block(block->serialize());
            mined.nonce = 0;
            std::string hash = this->calculateHash(&mined);
            while(!this->verify(hash))
            {
                mined.nonce++;
                hash = this->calculateHash(&mined);
            };
            
            // update block with mined hash
            block->nonce = mined.nonce;
            block->hash = hash;
            
            return block;
        }
        
        Block* parallel_mine(Block* block) const
        {
            // create a copy of the block
            int sstop=0;
            int tn;
            Block mined = Block(block->serialize());
            Block mined_private = mined;
            std::cout<<mined_private.toString()<<std::endl;
            mined.nonce = 0;
            std::string hash = this->calculateHash(&mined);
            std::string hash_private = hash;
            omp_set_num_threads(8);
            #pragma omp parallel private(tn, hash_private, mined_private)
            {
                
                tn=omp_get_thread_num();
                printf("Thread %d, sstop=%d\n",tn,sstop);
                while(!sstop && !this->verify(hash)) {
                    #pragma omp critical
                    {
                        mined.nonce++;
                        mined_private.nonce=mined.nonce;
                    }
                    
                    if (mined_private.nonce==3042240){
                        std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
                        std::cout<<mined_private.toString()<<std::endl;
                        mined_private.timestamp=0;
                        std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
                    }
                    hash_private = this->calculateHash(&mined_private);
                    if (this->verify(hash_private)) {
                        sstop=1;
                        hash = hash_private;
                        printf("Thread %d, sstop=%d\n",tn,sstop);
                    }
                };
                

            }
            
            // update block with mined hash
            block->nonce = mined.nonce;
            block->hash = hash;
            
            return block;
        }
        
        bool verify(const Block& block) const
        {
            return this->verify(block.hash);
        }
        
        bool verify(const std::string &hash) const
        {
            return hash.substr(0, this->zeros.length()).compare(this->zeros) == 0;
        }
        
    private:
        std::string zeros;
        
        std::string calculateHash(Block* block) const
        {
            
            std::stringstream ss;
            ss<<block->index<<block->timestamp<<block->previousHash<<block->nonce;
            std::string hash=sha256(ss.str());
            if (block->nonce==3042240) {
                std::cout<<block->toString()<<std::endl;
                std::cout<<hash<<std::endl;
            }
            return hash;
        }
};

#endif
