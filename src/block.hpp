#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "headers.hpp"  

class Block {
public:
    static int counter;
    int ID;
    ld timestamp;
    
    int depth;
    int size;
    vector<Transaction*> txns;
    Block* parentBlock;
    Peer* miner;
    
    Block (ld ts , Block* parent , Peer* miner , vector<Transaction*>txns)
       : timestamp(ts) , parentBlock(parent) , miner(miner) , txns(txns) {
       counter++;
       ID = counter;
    }

    void computeSize();
    void computeDepth();
 
};

#endif // BLOCK_HPP
