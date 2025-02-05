#include <iostream>
#include "headers.hpp"
#include "transaction.hpp"
#include "link.hpp"
#include "peer.hpp"

void Peer::add_link(Link* lk){
    if(links.size() > 6) return;// Not more than 6 neighbours

    auto it = find(neighbour_peers.begin(), neighbour_peers.end(),lk->other);
    if(it == neighbour_peers.end()) return; // Repeated link between two peers

    neighbour_peers.push_back(lk->other);
    links.push_back(lk);
}
bool Peer::checkTxn(Transaction * txn){
    Peer* sender = txn->sender;
    Peer* receiver = txn->receiver;
    if(sender->id >= last_id || receiver->id >= last_id) return false;
    if(balances[sender->id] < txn->amount) return false;
    else return true;
}


void Peer::receiveTxn(Transaction* txn) {
    pool.insert(txn);
}

void Peer::broadcast_transaction(Transaction* txn){
    for(auto& Link: links){
        Peer* peer = Link->other;
        sleep(peer->total_delay);
        // delay in transaction
        peer->receiveTxn(txn);
    }
}


void Peer::generateTxn_timestamp(int T_x, int threshold_time, double curr_time){
    random_device rd;
    mt19937 gen(rd());
    exponential_distribution<ld> expDist(1.0 / T_x);
    ld sample_time = expDist(gen);  // Mean interarrival time = T_x
    event_queue.push_back({curr_time+sample_time,  event_type<Type>, this->id });
}

void Peer::create_transaction_event(){
    vector<Transaction*> valid_transactions;

    for (auto& txn : pool) {
        if (checkTxn(txn)) {
            valid_transactions.push_back(txn);
        }
    }
    sort(valid_transactions.begin(), valid_transactions.end(), [](Transaction* a, Transaction* b) {
        return a->amount > b->amount;
    });

}

void Peer::receive_block(Block* block){
    if(block->height > last_height){
        pool.insert(block->transactions.begin(), block->transactions.end());
        balances[block->creator_id] += block->total_reward;
        last_height = block->height;
        last_id = block->transactions.back()->id;
    }
}

