#include "block.hpp"

void Block::computeSize() {
    size = 0;
    for (Transaction* txn : txns) {
        size += txn->size;
    }
}

void Block::computeDepth() {
    if (parentBlock == nullptr) {
        depth = 0;
    } else {
        depth = parentBlock->depth + 1;
    }
}