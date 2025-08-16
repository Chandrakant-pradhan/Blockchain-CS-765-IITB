#include "defs.hpp"

//initializing class
extern std::chrono::steady_clock::time_point start_time;
Peer::Peer(Simulation * sim, int ID, bool slow, bool low) : sim(sim), ID(ID), slow(slow), low(low) {
    genesisLocal = (sim->global_genesis_block)->make_copy_for_peer(this);
    numBlocks = 0;
    longest_chain_tips.push_back(genesisLocal);
    block_map[genesisLocal->ID] = genesisLocal;
    miningBlock = genesisLocal;
}

//get a random peer to send money
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

//get random money based on your balance 
int Peer::get_random_money() {
    int max_money = balance[ID];
    if (max_money <= 0) return 0;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(1, max_money);
    return dist(gen);
}

//creating transaction from random peer and random node
Transaction* Peer::on_order_to_create_txn(ld timestamp) {
    Peer* receiver = get_random_peer();
    if (!receiver) return nullptr;
    int amount = get_random_money();
    if (amount == 0) return nullptr;
    Transaction* txn = new Transaction(timestamp, this->ID, receiver->ID, amount);
    txn_map[txn->ID] = txn;
    return txn;
}

//adding txn received to txn_pool
void Peer::on_receive_txn(Transaction* txn) {
    txn_pool.insert(txn);
}

//function to mine a new block
Block* Peer::on_order_to_create_block(ld timestamp) {
    vector<Transaction*> txns_to_include;
    auto txn = txn_pool.begin();
    //new vector created to check time based txn validity
    vector<int>temp_balance = balance;
    //including txn
    while (txn != txn_pool.end() && txns_to_include.size() <= ((ld)sim->max_block_size / sim->transaction_size - 1)) {
        int sender = (*txn)->senderID;
        int amount = (*txn)->amount;
        if (temp_balance[sender] <= 0 || temp_balance[sender] < amount) {
            txn++;
            continue;
        }
        txns_to_include.push_back(*txn);
        temp_balance[sender] -= amount;
        txn = txn_pool.erase(txn);  
    }
    //returning the new block
    Block* parent = miningBlock;
    Block* block = new Block(timestamp, parent, this, txns_to_include);
    numBlocks++;
    return block;
}

//function to update state of peer when longest branch changes
void Peer::change_branch(Block* newBlock){
    Block* oldBranch= miningBlock;
    Block* newBranch = newBlock;
    
    //finding the forking point via LCA
    newBranch  = newBlock -> parentBlock;
    while(newBranch != oldBranch){
        newBranch = newBranch -> parentBlock;
        oldBranch = oldBranch -> parentBlock;
    }
    
    while(miningBlock != oldBranch){
        
        // cout<<miningBlock->ID<<" "<<oldBranch->ID<<endl;
        for(auto txn : miningBlock->txns){
            balance[txn->senderID] += txn->amount;
            balance[txn->receiverID] -= txn->amount;
            txn_pool.insert(txn);
        }
        balance[(miningBlock->miner)->ID] -= sim->miner_reward; 
        miningBlock = miningBlock->parentBlock;
    }
    
    //updating the pointer to the longest tip
    miningBlock = newBlock;
    while(newBlock != oldBranch){
        for(auto txn : newBlock->txns){
            balance[txn->senderID] -= txn->amount;
            balance[txn->receiverID] += txn->amount;
            txn_pool.erase(txn);
        }
        balance[(newBlock->miner)->ID] += sim->miner_reward; 
        newBlock = newBlock->parentBlock;
    }
}

void Peer::add_block_to_chain(Block* block) {
    Block* newBlock = block->make_copy_for_peer(this);
    block_map[newBlock->ID] = newBlock;
    block_chain.push_back(newBlock);
    
    //when block comes on the longest chain 
    if(newBlock->parentBlock->ID == miningBlock->ID){
        
        longest_chain_tips.clear();
        longest_chain_tips.push_back(newBlock);
        miningBlock = newBlock;
        balance[newBlock->miner->ID] += sim->miner_reward;
        
        // initiate a new block creation event
        Tell_node_to_create_block * event = new Tell_node_to_create_block(this,sim->global_time);
        sim->event_queue.push(event);
        // invalidate the ongoing creation process
        if(createProc) createProc->isStillValid = false;
        createProc = event;
        
    }
    //when there is a possiblity of longer chain than the current longest chain
    else if(newBlock->depth > miningBlock->depth && newBlock->parentBlock != miningBlock){
        
        longest_chain_tips.clear();
        longest_chain_tips.push_back(newBlock);
        
        change_branch(newBlock);
        
        // initiate a new block creation event
        Tell_node_to_create_block * event = new Tell_node_to_create_block(this,sim->global_time);
        sim->event_queue.push(event);
        // invalidate the ongoing creation process
        if(createProc) createProc->isStillValid = false;
        createProc = event;
        
    }
    // 
    //if depth same as current longest chain then add to the longest tips
    else if(newBlock->depth == miningBlock->depth && newBlock->parentBlock != miningBlock){
        longest_chain_tips.push_back(newBlock);
    }
    
    auto current_time = std::chrono::steady_clock::now(); // Current time
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
    //writing data to the peer file
    std::ofstream file("peer_" + std::to_string(ID) + "_tree.txt", std::ios::app);
    if (file.is_open()) {
        file << "Block ID: " << block->ID
             << ", Parent ID: " << (block->parentBlock ? std::to_string(block->parentBlock->ID) : "None")
             << ", Received at" << elapsed_ms << ", Timestamp: " << block->timestamp << ", Block Miner " << block->miner->ID << ", ringmaster " << this->sim->ringmaster<< ", type1-"<< block->miner->slow << ",type2-"<< block->miner->low <<"\n";
        file.close();
    } else {
        std::cerr << "Error: Unable to open file for Peer " << ID << "\n";
    }
    


}

// action to do when a block is received
void Peer::on_receive_block(Block* block) {
    
    if(block==NULL){
        cerr<< "Error: Received NULL PEER block"<<endl;
        exit(1);
    }
    if (block_map.find(block->ID) != block_map.end()){
        return;
    }
    int parentID = block->parentBlock->ID;
    //updating the block pool with new orphan block
    if (block_map.find(parentID) == block_map.end()) {
        block_pool.insert(block);
        return;
    }
    

    //if longest chain endpoint is the parent of the block
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
    } 
    
    else {
        
        //calculating the updated temporary balance when a new block attaches to a side chain
        Block* parent = block_map[parentID];
        vector<int> temp_balance = this->balance;
        Block* actual_branch = miningBlock;
        int act_depth = actual_branch->depth;
        int curr_branch_depth = parent->depth;
        
        //updating till forking point
        while (act_depth != curr_branch_depth) {
            for (auto txn : actual_branch->txns) {
                temp_balance[txn->senderID] += txn->amount;
                temp_balance[txn->receiverID] -= txn->amount;
            }
            temp_balance[(actual_branch->miner)->ID] -= sim->miner_reward;
            actual_branch = actual_branch->parentBlock;
            act_depth--;
        }
        
        //updating from forking point to the new side chain
        while (parent->ID != actual_branch->ID) {
            for (auto txn : actual_branch->txns) {
                temp_balance[txn->senderID] += txn->amount;
                temp_balance[txn->receiverID] -= txn->amount;
            }
            temp_balance[(actual_branch->miner)->ID] -= sim->miner_reward;
            for (auto txn : parent->txns) {
                temp_balance[txn->senderID] -= txn->amount;
                temp_balance[txn->receiverID] += txn->amount;
            }
            temp_balance[(parent->miner)->ID] += sim->miner_reward;
            parent = parent->parentBlock;
            actual_branch = actual_branch->parentBlock;
        }
        
        //checking validity
        for (auto txn : block->txns) {
            if (temp_balance[txn->senderID] < txn->amount)
            {
                return;
            }
        }
        
        add_block_to_chain(block);
        
    }
    
    //loop to check for orphan block which have same parent ID as the upcomming block
    //and recursively adding it to the blockchain
    if(block_map.find(block->ID) == block_map.end()){
        Block* _block = block->make_copy_for_peer(this);
        
        block_map[_block->ID] = _block;
    }
    vector<Block*> toProcess;
    for (auto orphan : block_pool) {
        if (orphan->parentBlock->ID == block->ID) {
            toProcess.push_back(orphan);
        }
    }
    for (auto orphan : toProcess) {
        block_pool.erase(orphan); 
        on_receive_block(orphan); 
    }
}

//calculating number of peer's block in his own longest chain
int Peer::countMyBlks() {
    int count = 0;
    Block* curr = miningBlock;
    while(curr != genesisLocal && curr!=NULL){
        if(curr->miner == this){
            count++;
        }
        curr = curr->parentBlock;
    }
    return count;
}

int Peer::total_block() {
    int count = 0;
    Block* curr = miningBlock;
    while(curr != genesisLocal && curr!=NULL){
        count++;
        curr = curr->parentBlock;
    }
    return count;
}