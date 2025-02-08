#include "defs.hpp"

int Block::counter=0;

//vector<Transaction*>  txns is best as it uses the & which is the same reference
Block::Block(ld timestamp, Block* parentBlock, Peer* miner, vector<Transaction*> &txns) 
    : timestamp(timestamp), parentBlock(parentBlock), miner(miner), txns(txns) {
    ID = counter++;
    size = computeSize();
    depth = computeDepth();
    if (parentBlock) {
        (parentBlock->childBlocks).push_back(this);
    }
}

int Block::computeSize() { 
    int totalSize = 0;
    for (Transaction* txn : txns) {
        totalSize += txn->size;
    }
    return totalSize;
}

int Block::computeDepth() {  
    return (parentBlock == nullptr) ? 0 : parentBlock->depth + 1;
}

Block* Block::make_copy_for_peer(Peer* other) {
    Block* newBlock = new Block(timestamp, nullptr, miner, txns);
    Block* parent_block_in_other = nullptr; 

    if (parentBlock != nullptr) {
        parent_block_in_other = other->block_map[parentBlock->ID];  
    }

    newBlock->ID = ID;
    newBlock->size = size;
    newBlock->parentBlock = parent_block_in_other;
    newBlock->depth = newBlock->computeDepth();
    
    return newBlock;
}
