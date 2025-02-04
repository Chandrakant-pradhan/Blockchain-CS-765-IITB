#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include "headers.hpp"  

class Blockchain {
public:
    static Block* globalGenBlk;
    Block* genBlk;
    Block* currentBlk;
    void addBlk(Block* b);
    void getBalance(vector<int>&balance);
    void getCompletedTxns(vector<Transaction*>&txns);
    
};

#endif // BLOCKCHAIN_HPP
