#include "defs.hpp"



Event :: Event(ld timestamp) {
    this->executeTime = timestamp;
}

Tell_node_to_create_txn::Tell_node_to_create_txn(Peer* node, ld curr_time) : Event(0), node(node) {
    // node is making transaction
    node->isMakingTxn = true;
    static default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    exponential_distribution<double> exp_dist(1.0 / node->sim->Ttx);
    // this is random time of distribution that is added to curr_time
    // this->executeTime is updated every time for that
    this->executeTime = curr_time + exp_dist(generator);
    node->sim->event_queue.push(new Tell_node_to_create_txn(node, this->executeTime));
}

Tell_node_to_create_block::Tell_node_to_create_block(Peer* node, ld curr_time) : Event(0), node(node) {
    node->isMakingBlock = true;
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::exponential_distribution<double> exp_dist(node->hk / node->sim->I);
    this->executeTime = curr_time + exp_dist(generator);
    node->sim->event_queue.push(new Tell_node_to_create_block(node,this->executeTime));
    //node is making block after that executeTime is updated of event 
}


Tell_node_they_rcvd_txn::Tell_node_they_rcvd_txn(Peer* node, Transaction* txn, ld curr_time) : Event(curr_time), node(node), txn(txn) {
    // nothing to delcare as such all things are declared in function definition
}

Tell_node_they_rcvd_block::Tell_node_they_rcvd_block(Peer* node, Block* block, ld curr_time) : Event(curr_time), node(node), block(block) {
    // nothing to delcare as such all things are declared in function definition
}
void Tell_node_to_create_txn::execute(){
    Transaction* txn = node->on_order_to_create_txn(this->executeTime);
    node->txn_pool.insert(txn);
    for (Peer* neigh : node->neigh) {
        // here i got to find the link that is node->link_map[neigh]
        Link* link = node->link_map[neigh];
        ld delay = link->getTotalDelay(txn->size);
        // this is event that took place 
        Tell_node_they_rcvd_txn* event = new Tell_node_they_rcvd_txn(neigh, txn, this->executeTime + delay);
        // this is but different for every peer that is node->sim will be different
        //but passed by pointer so ok only
        node->sim->event_queue.push(event);
    }
    //marking the Txn of the link
    node->isMakingTxn = false;
}

void Tell_node_they_rcvd_txn::execute(){
    node->on_receive_txn(txn);
    for (Peer* neigh : node->neigh) {
        if(node->sent_map_txn[txn].find(neigh) == node->sent_map_txn[txn].end() &&
           neigh->sent_map_txn[txn].find(node) == neigh->sent_map_txn[txn].end()){
            //tabhi forward akr
            Link* link = node->link_map[neigh];
            ld delay = link->getTotalDelay(txn->size);
            Event* event = new Tell_node_they_rcvd_txn(neigh, txn, this->executeTime + delay);
            // noe issue in this mostly likely to be  node->sim-> event_queue
            node->sim->event_queue.push(event);
        }
    }
} 

void Tell_node_to_create_block::execute(){
    Block* block = node->on_order_to_create_block(this->executeTime);
    // everything is similar but here it is adding to blockchain rather then 
    // just adding to set of blocks of chain
    // adding copy of blockchain here parent pointer could be different fro both
    // so taking care of that
    node->add_block_to_chain(block);
    for (Peer* neigh : node->neigh) {
        Link* link = node->link_map[neigh];
        // block message so block->size and ok is of different size
        ld delay = link->getTotalDelay(block->size);
        Tell_node_they_rcvd_block* event = new Tell_node_they_rcvd_block(neigh, block, this->executeTime + delay);
        node->sim->event_queue.push(event);
    }
    node->isMakingBlock = false;
}

void Tell_node_they_rcvd_block::execute(){
    node->on_receive_block(block);
    // mostly same
    for (Peer* neigh : node->neigh) {
        if(node->sent_map_block[block].find(neigh) == node->sent_map_block[block].end() &&
           neigh->sent_map_block[block].find(node) == neigh->sent_map_block[block].end()){
            //tabhi forward akr
            Link* link = node->link_map[neigh];
            ld delay = link->getTotalDelay(block->size);
            Event* event = new Tell_node_they_rcvd_block(neigh, block, this->executeTime + delay);
            node->sim->event_queue.push(event);
        }
    }
}
