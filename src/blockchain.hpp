#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include "headers.hpp"    

class Blockchain {
public:
    static Block* globalGenBlk;  

    Block* genBlk;
    Block* currentBlk;
    
    Blockchain(Block* genBlk) : genBlk(genBlk) {
        currentBlk = genBlk;
    }

    void addBlk(Block* b);
    void getBalance(std::vector<int>& balance);  
    void getCompletedTxns(std::vector<Transaction*>& txns); 
};

#endif // BLOCKCHAIN_HPP

