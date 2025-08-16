#include "defs.hpp"

int last_seen_event = 0;
set<int> st;
std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
Attacker::Attacker(Simulation* sim, int ID, bool slow, bool low)
    : Peer(sim, ID, slow, low){
    private_mining_block = miningBlock;
    forkingPoint = private_mining_block;
    private_txn_pool = txn_pool;
    private_balance = balance;
}
// correct
Block* Attacker::on_order_to_create_block(ld timestamp){
    vector<Transaction*> txns_to_include;
    auto txn = private_txn_pool.begin();
    vector<int>temp_balance = private_balance;
    while (txn != private_txn_pool.end() && txns_to_include.size() <= ((ld)sim->max_block_size / sim->transaction_size - 1)) {
        int sender = (*txn)->senderID;
        int amount = (*txn)->amount;
        if (temp_balance[sender] <= 0 || temp_balance[sender] < amount) {
            txn++;
            continue;
        }
        txns_to_include.push_back(*txn);
        temp_balance[sender] -= amount;
        txn = private_txn_pool.erase(txn);
    }
    Block* parent = private_mining_block;
    Block* block = new Block(timestamp, parent, this, txns_to_include);
    numBlocks++;
    return block;
}
// this is also correct
void Attacker::add_block_to_chain(Block* _block) {
    DEBUG_PRINT();
    Block* block = _block->make_copy_for_peer(this);
    block_map[block->ID] = block;
    DEBUG_PRINT();
    if(!(block->miner->slow)){
        private_mining_block = block;
        private_block_chain.push_back(block);
        private_balance[block->miner->ID] += sim->miner_reward;
        DEBUG_PRINT();
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        std::ofstream file("peer_" + std::to_string(ID) + "_tree.txt", std::ios::app);
        if (file.is_open()) {
            file << "Block ID: " << block->ID
                << ", Parent ID: " << (block->parentBlock ? std::to_string(block->parentBlock->ID) : "None")
                << ", Received at " << elapsed_ms << ", Timestamp: " << block->timestamp << ", Block Miner " << block->miner->ID << ", ringmaster " << this->sim->ringmaster<< ", type1-"<< block->miner->slow << ",type2-"<< block->miner->low <<"\n";
            file.close();
        } else {
            std::cerr << "Error: Unable to open file for Peer " << ID << "\n";
        }
        int r11 = private_mining_block->depth;
        int r12 = miningBlock->depth;
        if(ID == ringmaster){
            if(last_seen_event== 1 && r11 > r12){
                commandRelease(private_mining_block->ID, _block);
                last_seen_event = 2;
            }
        }
        DEBUG_PRINT();
    }
    else{
        block_chain.push_back(block);
        if(block->parentBlock->ID == miningBlock->ID){
            longest_chain_tips.clear();
            longest_chain_tips.push_back(block);
            miningBlock = block;
            balance[block->miner->ID] += sim->miner_reward; 
        }
        else if(block->depth > miningBlock->depth && block->parentBlock != miningBlock){
            longest_chain_tips.clear();
            longest_chain_tips.push_back(block);
            change_branch(block);
        }
        else if(block->depth == miningBlock->depth && block->parentBlock != miningBlock){
            longest_chain_tips.push_back(block);
        }
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        std::ofstream file("peer_" + std::to_string(ID) + "_tree.txt", std::ios::app);
        if (file.is_open()) {
            file << "Block ID: " << block->ID
                << ", Parent ID: " << (block->parentBlock ? std::to_string(block->parentBlock->ID) : "None")
                << ", Received at " << elapsed_ms << ", Timestamp: " << block->timestamp << ", Block Miner " << block->miner->ID << ", ringmaster " << this->sim->ringmaster<< ", type1-"<< block->miner->slow << ",type2-"<< block->miner->low <<"\n";
            file.close();
        } else {
            std::cerr << "Error: Unable to open file for Peer " << ID << "\n";
        }
        int r12 = miningBlock->depth;
        int r11 = private_mining_block->depth;
        if(ID == ringmaster){
            if(r12  == r11 -1){
                commandRelease(private_mining_block->ID, _block);
                last_seen_event = 2;
            }
            else if(r12 == r11){
                commandRelease(private_mining_block->ID, _block);
                last_seen_event = 1;
            }
            else if(r12 > r11){
                commandRelease(miningBlock->ID, _block);
                last_seen_event = 2;
            }
        }
    }
}

void Attacker::on_receive_block(Block * block){
    bool mal = !(block->miner->slow);
    int parentID = block->parentBlock->ID;
    if(mal){
        DEBUG_PRINT();
        if (block_map.find(block->ID) != block_map.end()){
            return;
        }
        if (block_map.find(parentID) == block_map.end()) {
            block_pool.insert(block);
            return;
        }
        DEBUG_PRINT();
        for (auto txn : block->txns) {
            if (private_balance[txn->senderID] < txn->amount){
                return;
            }
        }
        DEBUG_PRINT();
        for (auto txn : block->txns) {
            private_txn_pool.erase(txn);
        }
        DEBUG_PRINT();
        for (auto txn : block->txns) {
            private_balance[txn->senderID] -= txn->amount;
            private_balance[txn->receiverID] += txn->amount;
        }
        DEBUG_PRINT();
        add_block_to_chain(block);
        DEBUG_PRINT();
    }
    else{
        DEBUG_PRINT();
        if (block_map.find(block->ID) != block_map.end()){
            return;
        }
        DEBUG_PRINT();
        if (block_map.find(parentID) == block_map.end()) {
            block_pool.insert(block);
            return;
        }
        DEBUG_PRINT();
        if (parentID == miningBlock->ID) {
            for (auto txn : block->txns) {
                if (balance[txn->senderID] < txn->amount){
                    return;
                }
            }
            DEBUG_PRINT();
            for (auto txn : block->txns) {
                txn_pool.erase(txn);
            }
            DEBUG_PRINT();
            for (auto txn : block->txns) {
                balance[txn->senderID] -= txn->amount;
                balance[txn->receiverID] += txn->amount;
            }
            DEBUG_PRINT();
            add_block_to_chain(block);
        }
        
        else {
            DEBUG_PRINT();
            Block* parent = block_map[parentID];
            vector<int> temp_balance = this->balance;
            Block* actual_branch = miningBlock;
            DEBUG_PRINT();
            if(actual_branch == NULL){
                cerr<<"hello"<<endl;
                return;
            }
            int act_depth = actual_branch->depth;
            int curr_branch_depth = parent->depth;
            DEBUG_PRINT();
            
            //updating till forking point
             if(act_depth < curr_branch_depth){
                miningBlock = private_mining_block;
                forkingPoint = private_mining_block;
                txn_pool = private_txn_pool;
                balance = private_balance;

                temp_balance = this->balance;
                actual_branch = miningBlock;
                act_depth = actual_branch->depth;
                curr_branch_depth = parent->depth;
            }
            // // assert(act_depth >= curr_branch_depth);
            while (act_depth > curr_branch_depth) {
                for (auto txn : actual_branch->txns) {
                    temp_balance[txn->senderID] += txn->amount;
                    temp_balance[txn->receiverID] -= txn->amount;
                }
                temp_balance[(actual_branch->miner)->ID] -= this->sim->miner_reward;
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
            
            for (auto txn : block->txns) {
                if (temp_balance[txn->senderID] < txn->amount)
                {
                    return;
                }
            }
            DEBUG_PRINT();
            add_block_to_chain(block);
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
}

int Attacker::getPrivateChainLength() {
    return private_block_chain.size();
}

void Attacker::commandRelease(int newAttackPoint, Block* block){
    assert(this->ID == ringmaster);
    string hash_ptr = hashed_block(block);
    have_block_map[block->ID] = block;
    hashed_block_seen[hash_ptr] = 2;
    hash_block_reverse_map[hash_ptr] = block->ID;
    command_received[newAttackPoint] = 1;

    Block* newblock = block_map[newAttackPoint];
    hash_ptr = hashed_block(newblock);
    for(Attacker* slave : attack_neigh){
        if(slave->command_received.find(newAttackPoint) == slave->command_received.end()){
            Link* link = attacker_link_map[slave]; // Use attacker_link_map instead of link_map
            ld delay = link->getTotalDelay(0.064);
            auto event  = new give_Command_toRelease(this, slave, newAttackPoint, hash_ptr, sim->global_time + delay);
            sim->event_queue.push(event);
        }
    }
    releasePrivateChain(newAttackPoint);
}

void Attacker::releasePrivateChain(int newAttackPoint) {// No need to release an empty chain
    DEBUG_PRINT();
    if(newAttackPoint != private_mining_block->ID){
        DEBUG_PRINT();
        private_txn_pool = txn_pool;
        private_balance = balance;
        DEBUG_PRINT();
    }
    else{
        DEBUG_PRINT();
        txn_pool = private_txn_pool;
        balance = private_balance;
        DEBUG_PRINT();
    }
    private_mining_block = block_map[newAttackPoint];
    miningBlock = private_mining_block;
    forkingPoint = private_mining_block;;
    for (Block* block : private_block_chain) {
        for (Peer* neigh : neigh) {  // Use attack_neigh instead of this->neigh
            if(sent_map_block[block->ID].find(neigh) == sent_map_block[block->ID].end() &&
                neigh->sent_map_block[block->ID].find(this) == neigh->sent_map_block[block->ID].end()){// Should fast attackers get the block?
                Link* link = link_map[neigh]; // Use attacker_link_map instead of link_map
                ld delay = link->getTotalDelay(0.064);
                string hash = hashed_block(block);
                auto* event = new Tell_node_they_rcvd_hashed_block(this,neigh,hash, 1 ,sim->global_time + delay, 1);
                sim->event_queue.push(event);
                DEBUG_PRINT();
            }
        }
    }
    DEBUG_PRINT();
    private_block_chain.clear();

}
