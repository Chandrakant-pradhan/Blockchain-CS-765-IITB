#include "defs.hpp"

Event :: Event(ld timestamp) {
    this->executeTime = timestamp;
}

Tell_node_to_create_txn::Tell_node_to_create_txn(Peer* node, ld curr_time) : Event(0), node(node) {
    static default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    exponential_distribution<double> exp_dist(1.0 / node->sim->Ttx);
    this->executeTime = curr_time + exp_dist(generator);
    // cout<<"Order Txn creation "<<node->ID<<"-->"<<curr_time<<"<--"<<executeTime<<endl;
}

Tell_node_to_create_block::Tell_node_to_create_block(Peer* node, ld curr_time) : Event(0), node(node) {
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::exponential_distribution<double> exp_dist(node->hk / node->sim->I);
    this->executeTime = curr_time + exp_dist(generator);
    // cout<<"Order Blk creation "<<node->ID<<"-->"<<curr_time<<"<--"<<executeTime<<endl;
}


Tell_node_they_rcvd_txn::Tell_node_they_rcvd_txn(Peer* node, Transaction* txn, ld curr_time) : Event(0), node(node), txn(txn) {
    this->executeTime = curr_time;
}

Tell_node_they_rcvd_block::Tell_node_they_rcvd_block(Peer* node, Block* block, ld curr_time) : Event(0), node(node), block(block) {
    this->executeTime = curr_time;
}

void Tell_node_to_create_txn::execute(){
    if(node->balance[node->ID] > 0){
        Transaction* txn = node->on_order_to_create_txn(this->executeTime);
        node->txn_pool.insert(txn);
        for (Peer* neigh : node->neigh) {
            Link* link = node->link_map[neigh];
            ld delay = link->getTotalDelay(txn->size);
            Tell_node_they_rcvd_txn* event = new Tell_node_they_rcvd_txn(neigh, txn->make_copy(), this->executeTime + delay);
            node->sim->event_queue.push(event);
        }
    }
    node->sim->event_queue.push(new Tell_node_to_create_txn(node, this->executeTime));
}

void Tell_node_they_rcvd_txn::execute(){
    // cout<<"RECEIVED TXN"<<"---"<<node->ID<<"--"<<this->executeTime<<endl;
    node->on_receive_txn(txn);
    for (Peer* neigh : node->neigh) {
        if(node->sent_map_txn[txn->ID].find(neigh) == node->sent_map_txn[txn->ID].end() &&
           neigh->sent_map_txn[txn->ID].find(node) == neigh->sent_map_txn[txn->ID].end()){
            Link* link = node->link_map[neigh];
            ld delay = link->getTotalDelay(txn->size);
            Event* event = new Tell_node_they_rcvd_txn(neigh, txn->make_copy(), this->executeTime + delay);
            node->sent_map_txn[txn->ID].insert(neigh);
            node->sim->event_queue.push(event);
        }
    }
} 

void Tell_node_to_create_block::execute(){
    Block* block = node->on_order_to_create_block(this->executeTime);
    for(auto txn : block->txns){
        node->balance[txn->senderID] -= txn->amount;
        node->balance[txn->receiverID] += txn->amount;
    }
    node->add_block_to_chain(block);
    for (Peer* neigh : node->neigh) {
        Link* link = node->link_map[neigh];
        ld delay = link->getTotalDelay(block->size);
        Tell_node_they_rcvd_block* event = new Tell_node_they_rcvd_block(neigh, block, this->executeTime + delay);
        node->sim->event_queue.push(event);
    }
    node->sim->event_queue.push(new Tell_node_to_create_block(node,this->executeTime));
}

void Tell_node_they_rcvd_block::execute(){
    // cout<<"RECEIVED BLK"<<"--"<<block->ID<<"---"<<node->ID<<"--"<<this->executeTime<<endl;
    node->on_receive_block(block);
    for (Peer* neigh : node->neigh) {
        if(node->sent_map_block[block->ID].find(neigh) == node->sent_map_block[block->ID].end() &&
           neigh->sent_map_block[block->ID].find(node) == neigh->sent_map_block[block->ID].end()){
            Link* link = node->link_map[neigh];
            ld delay = link->getTotalDelay(block->size);
            Event* event = new Tell_node_they_rcvd_block(neigh, block, this->executeTime + delay);
            node->sent_map_block[block->ID].insert(neigh);
            node->sim->event_queue.push(event);
        }
    }
}