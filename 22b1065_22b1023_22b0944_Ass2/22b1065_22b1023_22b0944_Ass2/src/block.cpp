#include "defs.hpp"

int Block::counter=0;

//instantiating class
Block::Block(ld timestamp, Block* parentBlock, Peer* miner, vector<Transaction*> &txns) 
    : timestamp(timestamp), parentBlock(parentBlock), miner(miner), txns(txns) {
    ID = counter++;
    size = computeSize();
    depth = computeDepth();
    if (parentBlock) {
        (parentBlock->childBlocks).push_back(this);
    }
}

ld Block::computeSize() { 
    ld totalSize = 0;
    for (Transaction* txn : txns) {
        totalSize += txn->size;
    }
    ld min_val= 1.024;
    return max(min_val, totalSize);
}

int Block::computeDepth() {  
    return (parentBlock == nullptr) ? 0 : parentBlock->depth + 1;
}

Block* Block::make_copy_for_peer(Peer* other) {
    counter--;
    Block* newBlock = new Block(timestamp, nullptr, miner, txns);
    Block* parent_block_in_other = nullptr; 

    if (parentBlock != nullptr) {
        parent_block_in_other = other->block_map[parentBlock->ID];  
    }
    newBlock->timestamp = timestamp;
    newBlock->ID = ID;
    newBlock->size = size;
    newBlock->parentBlock = parent_block_in_other;
    newBlock->depth = newBlock->computeDepth();
    return newBlock;
}
