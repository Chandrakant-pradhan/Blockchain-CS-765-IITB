#include "defs.hpp"

Peer::Peer(Simulation * sim, int ID, bool slow, bool low) : sim(sim), ID(ID), slow(slow), low(low) {
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
}

Block* Peer::on_order_to_create_block(ld timestamp) {
    vector<Transaction*> txns_to_include;
    while (!txn_pool.empty() && txns_to_include.size() <= 0.6 * ((ld)sim->max_block_size / sim->transaction_size)) {
        Transaction* txn = *txn_pool.begin();
        txns_to_include.push_back(txn);
        txn_pool.erase(txn);
    }

    // multiple longest block of longest chain then we should randomly choose out of them
    Block* parent = longest_chain_tips[sim->getRandomNumber(0, longest_chain_tips.size() - 1)];
    // next it has pointer to parent and current block of miner and all the transactions to include
    // there is in blockchain that
    Block* block = new Block(timestamp, parent, this, txns_to_include);
    return block;
}

void Peer::add_block_to_chain(Block* block) {
    Block* newBlock = block->make_copy_for_peer(this);
    
    block_map[newBlock->ID] = newBlock;
    block_chain.push_back(newBlock);

    // id the newBlock->depth > longest chain tips
    // any tips might be same only

    if (newBlock->depth > longest_chain_tips[0]->depth) {
        longest_chain_tips.clear();
        longest_chain_tips.push_back(newBlock);
    } else if (newBlock->depth == longest_chain_tips[0]->depth) {
        longest_chain_tips.push_back(newBlock);
    }
}

void Peer::on_receive_block(Block* block) {
    if (block_map.find(block->ID) != block_map.end()) return; // Block already known

    bool isValid = true;
    for (Transaction* txn : block->txns) {
        if (balance[txn->senderID] < txn->amount) { 
            isValid = false;
            break;
        }
    }
    if (isValid) {
        for (Transaction* txn : block->txns) {
            balance[txn->senderID] -= txn->amount;
            balance[txn->receiverID] += txn->amount;
        }

        for (Transaction* txn : block->txns) {
            txn_pool.erase(txn_map[txn->ID]); // Remove processed transactions
        }

        add_block_to_chain(block);
    }
}
