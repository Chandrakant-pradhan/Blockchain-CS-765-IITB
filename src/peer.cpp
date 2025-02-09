#include "defs.hpp"

Peer::Peer(Simulation * sim, int ID, bool slow, bool low) : sim(sim), ID(ID), slow(slow), low(low) {
    Block* genesisLocal = (sim->global_genesis_block)->make_copy_for_peer(this);
    longest_chain_tips.push_back(genesisLocal);
    block_map[genesisLocal->ID] = genesisLocal;
    miningBlock = genesisLocal;
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
}

Block* Peer::on_order_to_create_block(ld timestamp) {
    vector<Transaction*> txns_to_include;
    auto txn = txn_pool.begin();
    vector<int>temp_balance = balance;
    
    while (txn != txn_pool.end() && txns_to_include.size() <= ((ld)sim->max_block_size / sim->transaction_size - 1)) {
        int sender = (*txn)->senderID;
        int amount = (*txn)->amount;
        if (temp_balance[sender] <= 0 || temp_balance[sender] < amount) {
            txn++;
            continue;
        }
        txns_to_include.push_back(*txn);
        temp_balance[sender] -= amount;
        txn = txn_pool.erase(txn);  // Erase returns the next valid iterator.
    }
    Block* parent = miningBlock;
    Block* block = new Block(timestamp, parent, this, txns_to_include);
    return block;
}

void Peer::change_branch(Block* newBlock){
    // revert current chain's effect
    Block* oldBranch= miningBlock;
    Block* newBranch = newBlock;
    
    newBranch  = newBlock -> parentBlock;
    while(newBranch != oldBranch){
        newBranch = newBranch -> parentBlock;
        oldBranch = oldBranch -> parentBlock;
    }

    // here oldBranch == newBranch -------!!
    // Found LCA now just revert the Txns of new branch
    while(miningBlock != oldBranch){
        //all the txn in miningBlock->txns
        //for each each update the txns
        for(auto txn : miningBlock->txns){
            balance[txn->senderID] += txn->amount;
            balance[txn->receiverID] -= txn->amount;
            txn_pool.insert(txn);
        }
        balance[(miningBlock->miner)->ID] -= 5; // Mining Fee
        miningBlock = miningBlock->parentBlock;
    }

    // Add the txns of new branch
    miningBlock = newBlock; // Change current mining Block
    while(newBlock != oldBranch){
        for(auto txn : newBlock->txns){
            balance[txn->senderID] -= txn->amount;
            balance[txn->receiverID] += txn->amount;
            txn_pool.erase(txn);
        }
        balance[(newBlock->miner)->ID] += 5; // Mining Fee
        newBlock = newBlock->parentBlock;
    }
}

void Peer::add_block_to_chain(Block* block) {
    // but this makes sense when replicating it to other nodes blockchains and not this
    Block* newBlock = block->make_copy_for_peer(this);

    block_map[newBlock->ID] = newBlock;
    block_chain.push_back(newBlock);
    // till here all the forked will also be coming
    if(newBlock->parentBlock->ID == miningBlock->ID){
        longest_chain_tips.clear();
        longest_chain_tips.push_back(newBlock);
        miningBlock = newBlock;
        balance[newBlock->miner->ID] += 5; // Mining Fee

        // initiate a new block creation event
        Tell_node_to_create_block * event = new Tell_node_to_create_block(this,sim->global_time);
        sim->event_queue.push(event);
        // invalidate the ongoing creation process
        if(createProc) createProc->isStillValid = false;
        createProc = event;
    }
    else if(newBlock->depth > miningBlock->depth && newBlock->parentBlock != miningBlock){
        longest_chain_tips.clear();
        longest_chain_tips.push_back(newBlock);

        // Now handle the branch change
        change_branch(newBlock);

        // initiate a new block creation event
        Tell_node_to_create_block * event = new Tell_node_to_create_block(this,sim->global_time);
        sim->event_queue.push(event);
        // invalidate the ongoing creation process
        if(createProc) createProc->isStillValid = false;
        createProc = event;
    }
    else if(newBlock->depth == miningBlock->depth && newBlock->parentBlock != miningBlock){
        longest_chain_tips.push_back(newBlock);
    }


    std::ofstream file("peer_" + std::to_string(ID) + "_tree.txt", std::ios::app);
    if (file.is_open()) {
        file << "Block ID: " << newBlock->ID 
             << ", Parent ID: " << (newBlock->parentBlock ? std::to_string(newBlock->parentBlock->ID) : "None")
             << ", Timestamp: " << newBlock->timestamp << "\n";
        file.close();
    } else {
        std::cerr << "Error: Unable to open file for Peer " << ID << "\n";
    }

}

void Peer::on_receive_block(Block* block) {
    if (block_map.find(block->ID) != block_map.end()){
        return;
    }
    int parentID = block->parentBlock->ID;

    if (block_map.find(parentID) == block_map.end()) {
        block_pool.insert(block);
        return;
    }
    if (parentID == miningBlock->ID) {
        for (auto txn : block->txns) {
            if (balance[txn->senderID] < txn->amount){
                return;
            }
        }
        for (auto txn : block->txns) {
            txn_pool.erase(txn);
        }
        for (auto txn : block->txns) {
            balance[txn->senderID] -= txn->amount;
            balance[txn->receiverID] += txn->amount;
        }
        add_block_to_chain(block);
    } else {
        Block* parent = block_map[parentID];
        vector<int> temp_balance = this->balance;
        Block* actual_branch = miningBlock;
        int act_depth = actual_branch->depth;
        int curr_branch_depth = parent->depth;

        while (act_depth != curr_branch_depth) {
            for (auto txn : actual_branch->txns) {
                temp_balance[txn->senderID] += txn->amount;
                temp_balance[txn->receiverID] -= txn->amount;
            }
            temp_balance[(actual_branch->miner)->ID] -= 5;
            actual_branch = actual_branch->parentBlock;
            act_depth--;
        }
        
        while (parent != actual_branch) {
            for (auto txn : actual_branch->txns) {
                temp_balance[txn->senderID] += txn->amount;
                temp_balance[txn->receiverID] -= txn->amount;
            }
            temp_balance[(actual_branch->miner)->ID] -= 5;
            for (auto txn : parent->txns) {
                temp_balance[txn->senderID] -= txn->amount;
                temp_balance[txn->receiverID] += txn->amount;
            }
            temp_balance[(parent->miner)->ID] += 5;
            parent = parent->parentBlock;
            actual_branch = actual_branch->parentBlock;
        }

        for (auto txn : block->txns) {
            if (temp_balance[txn->senderID] < txn->amount) // Amount exceeds balance
            {
                cout<<"fishy balance"<<endl;
                return;
            }
        }
        add_block_to_chain(block);
    }

    vector<Block*> toProcess;
    for (auto orphan : block_pool) {
        if (orphan->parentBlock->ID == block->ID) {
            toProcess.push_back(orphan);
        }
    }
    for (auto orphan : toProcess) {
        block_pool.erase(orphan); // Remove from block_pool
        on_receive_block(orphan); // Re-process the orphan block
    }
}