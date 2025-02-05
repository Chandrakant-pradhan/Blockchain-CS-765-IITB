#include <iostream>
#include "headers.hpp"
#include "transaction.hpp"
#include "link.hpp"
#include "peer.hpp"
#include "event.hpp"

// Add a link to neighbour
void Peer::addLink(Link* lk){
    if(links.size() > 6) return; // Not more than 6 neighbours

    auto it = find(neighbour_peers.begin(), neighbour_peers.end(),lk->other);
    if(it == neighbour_peers.end()) return; // Repeated link between two peers

    neighbour_peers.push_back(lk->other);
    links.push_back(lk);

}

// Validity of Txn
bool Peer::checkTxn(Transaction * txn){
    int sender = txn->senderID;
    int receiver = txn->receiverID;
    if(sender > last_id || receiver > last_id) return false;
    if(balances[sender] < txn->amount) return false;
    else return true;
}

// Runs the creation of txn event
void Peer::runTxn(){ 
    ld sample_time = randomExp(Ttx);
    CreateTxnEvent * event = new CreateTxnEvent(this, sample_time);
}

// Create the actual Txn object
void Peer::createTxn(ld ts){
    int balance = balances[id];
    int receiver = randomInt(1,last_id);
    int amount = randomInt(0, balance); // Doubt : 0 should be allowed?? 
    Transaction * txn = new Transaction(ts,id,receiver,amount);
    pool.insert(txn);
    broadcastTxn(txn);
}

// Receive the Txn block from neighbour
void Peer::receiveTxn(Transaction* txn){
    if(!checkTxn(txn)) return;

}

void Peer::broadcastTxn(Transaction * txn){
    for(auto neigh : neighbour_peers){
        if(true){ // Should have some check
            int delay;
            for(auto lk : links){
                if(lk->other == neigh) 
                    delay = lk->getTotalDelay(txn->size);
            }
            ReceiveTxn * event = new ReceiveTxn(delay, neigh,txn);
        }
    }
}
