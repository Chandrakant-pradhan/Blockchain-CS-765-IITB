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
    Block (ld ts , Block* parent , Peer* miner)
       : timestamp(ts) , parentBlock(parent) , miner(miner){
       counter++;
       ID = counter;
    }

    void addTxn(Transaction* txn);
    void computeSize();
    //depth??
};

#endif // BLOCK_HPP
