#include "blockchain.hpp"

void Blockchain::addBlk(Block* b) {
    currentBlk->next = b;
    currentBlk = b;
}

void Blockchain::getBalance(std::vector<int>& balance){
    Block* curr = currentBlk;
    while(curr != genBlk){
        for(Transaction* txn : curr->txns){
            balance[txn->sender] -= txn->amount;
            balance[txn->receiver] += txn->amount;
        }
        curr = curr->parentBlock;
    }
}

void Blockchain::getCompletedTxns(std::vector<Transaction*>& txns){
    Block* curr = currentBlk;
    while(curr != genBlk){
        for(Transaction* txn : curr->txns){
            txns.push_back(txn);
        }
        curr = curr->parentBlock;
    }
}

