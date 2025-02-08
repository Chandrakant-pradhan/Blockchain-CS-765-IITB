#include "defs.hpp"

Peer::Peer(Simulation * sim, int ID, bool slow, bool low) : sim(sim), ID(ID), slow(slow), low(low) {
    geneisLocal = sim->global_genesis_block->make_copy_for_peer(this);
    longest_chain_tips.push_back(sim->global_genesis_block);
    block_map[sim->global_genesis_block->ID] = sim->global_genesis_block;
}

Peer* Peer::get_random_peer() {
    vector<Peer*> eligible_peers;
    for (const auto& entry : sim->peer_map) {
        if (entry.second != this) { 
            eligible_peers.push_back(entry.second);
        }
    }
    if (eligible_peers.empty()) return nullptr;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0, eligible_peers.size() - 1);
    return eligible_peers[dist(gen)];
}

int Peer::get_random_money() {
    // balance in the self ID
    int max_money = balance[ID];
    if (max_money <= 0) return 0;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(1, max_money);
    return dist(gen);
}

Transaction* Peer::on_order_to_create_txn(ld timestamp) {
    Peer* receiver = get_random_peer();
    if (!receiver) return nullptr;
    int amount = get_random_money();
    if (amount == 0) return nullptr;
    Transaction* txn = new Transaction(timestamp, this->ID, receiver->ID, amount);
    txn_map[txn->ID] = txn;  // Store in map for reference
    return txn;
}

void Peer::on_receive_txn(Transaction* txn) {
    txn_pool.insert(txn);
    // TODO: add the logic here instead of in the event execute
}

void Peer::on_order_to_create_block(ld timestamp) {
    vector<Transaction*> txns_to_include;
    auto txn = txn_pool.begin();

    // Find txns to include in block;
    while (txn != txn_pool.end() && txns_to_include.size() <= (sim->max_block_size / sim->transaction_size - 1)) {
        if(balance[(*txn)->senderID] > (*txn)->amount) // Amount exceeds balance
                {   txn++;
                    continue;
                }
        txns_to_include.push_back(*txn);
    }

    // Update the balances and delete the txns from pool
    for(auto txn : txns_to_include){
        balance[txn->senderID] -= txn->amount;
        balance[txn->receiverID] += txn->amount;
        txn_pool.erase(txn);
    }


    Block* parent = miningBlock;
    // next it has pointer to parent and current block of miner and all the transactions to include
    // there is in blockchain that
    Block* block = new Block(timestamp, parent, this, txns_to_include);
    miningBlock = block;
    balance[ID] += 50;

    Tell_node_to_create_block* event = new Tell_node_to_create_block(this,sim->global_time);
    sim->event_queue.push(event);

    // Now send to all the neighbours
    for(auto n : neigh){
        int delay = link_map[n]->getTotalDelay(block->size);
        Tell_node_they_rcvd_block * event = new Tell_node_they_rcvd_block(n,block,sim->global_time + delay);
        sim->event_queue.push(event);
        sent_map_block[block->ID].insert(n);
    }

}
void Peer::change_branch(Block* newBlock){
    // revert current chain's effect
    Block* oldBranch= miningBlock;
    Block* newBranch = newBlock;
    // Now the newBranch is ahead by one so just 
    // move it back by one to reach LCA

    while(newBranch != oldBranch){
        newBranch = newBranch -> parentBlock;
        oldBranch = newBranch -> parentBlock;
    }
    // Found LCA now just revert the Txns of new branch
    while(miningBlock != oldBranch){
        for(auto txn : miningBlock->txns){
            balance[txn->senderID] += txn->amount;
            balance[txn->receiverID] -= txn->amount;
            txn_pool.insert(txn);
        }
        balance[newBlock -> miner -> ID] -= 50; // Mining Fee
        miningBlock = miningBlock->parentBlock;
    }


    // Add the txns of new branch
    miningBlock = newBlock; // Change current mining Block
    while(oldBranch != newBlock){
        for(auto txn : newBlock->txns){
            balance[txn->senderID] -= txn->amount;
            balance[txn->receiverID] += txn->amount;
            txn_pool.erase(txn);
        }
        balance[newBlock -> miner -> ID] += 50; // Mining Fee
        newBlock = newBlock->parentBlock;
    }
}

void Peer::add_block_to_chain(Block* block) {
    Block* newBlock = block->make_copy_for_peer(this);
    
    block_map[newBlock->ID] = newBlock;
    block_chain.push_back(newBlock);

    // If in original chain => add it as it is
    if(block->parentBlock == miningBlock){
        miningBlock = block;
        balance[block->miner->ID] += 50; // Mining Fee

        // initiate a new block creation event
        Tell_node_to_create_block * event = new Tell_node_to_create_block(this,sim->global_time);
        sim->event_queue.push(event);
        // invalidate the ongoing creation process
        currBlockProc->isStillValid = false;
        currBlockProc = event;
    }
    else if(block->depth > miningBlock->depth){
        longest_chain_tips.clear();
        longest_chain_tips.push_back(block);

        // Now handle the branch change
        change_branch(block);

        // initiate a new block creation event
        Tell_node_to_create_block * event = new Tell_node_to_create_block(this,sim->global_time);
        sim->event_queue.push(event);
        // invalidate the ongoing creation process
        currBlockProc->isStillValid = false;
        currBlockProc = event;
    }
    else if(block->depth == miningBlock->depth){
        longest_chain_tips.push_back(block);
    }

}

void Peer::on_receive_block(Block* block) {
    if (block_map.find(block->ID) != block_map.end()) return; // Block already known

    int parentID = block->parentBlock->ID;

    if(block_map.find(parentID) == block_map.end()){
        // Parent not arrived just put it in pool
        block_pool.insert(block);
    }
    else if(parentID == miningBlock->ID){
        // In the main chain
        for(auto txn : block->txns){
            if(balance[txn->senderID] > txn->amount) // Amount exceeds balance
                return;
            // Actual change in balance happens when block gets added in the chain
        }
        for(auto txn : block->txns){
            txn_pool.erase(txn);
        }
        for(auto txn : block->txns){
            balance[txn->senderID] -= txn->amount;
            balance[txn->receiverID] += txn->amount;
        }
        add_block_to_chain(block);

        // Check if some orphaned block now has a parent.
        for(auto orphan : block_pool){
            if(orphan->parentBlock->ID == block->ID){
                add_block_to_chain(orphan);
            }
        }
    }
    else{
        // Creating or adding to fork
        for(auto txn : block->txns){
            if(balance[txn->senderID] > txn->amount) // Amount exceeds balance
                return;
        }
        // No Txn removal from pool as it is not in the main chain
        add_block_to_chain(block);

        // Check if some orphaned block now has a parent.
        for(auto orphan : block_pool){
            if(orphan->parentBlock->ID == block->ID){
                add_block_to_chain(orphan);
            }
        }
    }

    Block* parent = block_map[parentID];
    
    for(auto n : neigh){
        if((n->sent_map_block[block->ID].find(this) == n->sent_map_block[block->ID].end())
        && (sent_map_block[block->ID].find(n) == sent_map_block[block->ID].end()))
        {
            // Send to neighbours and Add to sent map
            int delay = link_map[n]->getTotalDelay(block->size);
            Tell_node_they_rcvd_block * event = new Tell_node_they_rcvd_block(n,block,sim->global_time + delay);
            sim->event_queue.push(event);
            sent_map_block[block->ID].insert(n);
        }
    }
}
