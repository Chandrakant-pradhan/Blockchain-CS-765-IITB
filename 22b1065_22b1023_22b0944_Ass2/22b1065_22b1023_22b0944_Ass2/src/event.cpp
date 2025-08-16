#include "defs.hpp"

int sim_flag;
//initializing class
Event :: Event(ld timestamp) {
    this->executeTime = timestamp;
}

Tell_node_they_rcvd_hashed_block::Tell_node_they_rcvd_hashed_block(Peer * node,Peer* neigh, string hashed_block, ll network_type, ld curr_time, int full_cast) : Event(0), node(node), neigh(neigh),hashed_block(hashed_block),  network_type(network_type), full_cast(full_cast){
    this->type = 1;
    this->executeTime = curr_time;
}

Tell_node_to_create_txn::Tell_node_to_create_txn(Peer* node, ld curr_time) : Event(0), node(node) {
    this->type = 2;
    static default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    //setting txn create event to take place at current time + random time taken from exponential distribution with mean Ttx
    exponential_distribution<double> exp_dist(1.0 / node->sim->Ttx);
    this->executeTime = curr_time + exp_dist(generator);
}

Tell_node_to_create_block::Tell_node_to_create_block(Peer* node, ld curr_time) : Event(0), node(node) {
    this->type = 3;
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    //setting block create event to take place at current time + random time taken from exponential distribution with mean (I/hk)
    std::exponential_distribution<double> exp_dist(node->hk / node->sim->I);
    this->executeTime = curr_time + exp_dist(generator);
    isStillValid = true;
}


Tell_node_they_rcvd_txn::Tell_node_they_rcvd_txn(Peer* node, Transaction* txn, ld curr_time) : Event(0), node(node), txn(txn) {
    this->type = 4;
    this->executeTime = curr_time;
}

Tell_node_they_rcvd_block::Tell_node_they_rcvd_block(Peer* node, Block* block, int network_type, ld curr_time, int full_cast) : Event(0), node(node), block(block) ,network_type(network_type), full_cast(full_cast) {
    this->type = 5;
    this->executeTime = curr_time;
}

ask_for_block::ask_for_block(Peer* neigh, Peer* node, string hash,ll network_type, ld curr_time, int full_cast) : Event(0), neigh(neigh), node(node), hashed_block(hash), network_type(network_type), full_cast(full_cast) {
    this->type = 6;
    ld delay = 0.0;
    if(network_type ==1){
        Link* link  = neigh->link_map[node];
        delay = link->getTotalDelay(0.064);
    }
    else{
        Attacker * attacker = dynamic_cast<Attacker*>(neigh);
        Attacker* attacker_node = dynamic_cast<Attacker*>(node);
        Link* link  = attacker->attacker_link_map[attacker_node];
        delay = link->getTotalDelay(0.064);
    }
    this->executeTime = curr_time + delay;
    ld timeout = curr_time + neigh->sim->Tty;
    Event* timeoutEvent = new BlockRequestTimeout(node, hashed_block, neigh, network_type, timeout, full_cast);
    neigh->sim->event_queue.push(timeoutEvent);
}

BlockRequestTimeout::BlockRequestTimeout(Peer* node, string hashed_block, Peer* neigh, ll network_type,  ld executeTime, int full_cast) : Event(0), node(node), hashed_block(hashed_block), neigh(neigh), network_type(network_type), full_cast(full_cast){
    this->type = 7;
    this->executeTime = executeTime;
}

void BlockRequestTimeout::execute(){
    if(neigh->hashed_block_seen.find(hashed_block) != neigh->hashed_block_seen.end()){
        if(neigh->hashed_block_seen[hashed_block] == 1) {
            neigh->last_time_request[hashed_block] = this->executeTime;
            Event* event = new ask_for_block(neigh, node, hashed_block, network_type,  this->executeTime, full_cast); 
            neigh->sim->event_queue.push(event);
        }
    }
}

void Tell_node_to_create_txn::execute(){
    if(node->balance[node->ID] > 0){
        Transaction* txn = node->on_order_to_create_txn(this->executeTime);
        node->txn_pool.insert(txn);
        for (Peer* neigh : node->neigh) {
            Link* link = node->link_map[neigh];
            ld delay = link->getTotalDelay(txn->size);
            Event * event = new Tell_node_they_rcvd_txn(neigh, txn->make_copy(), this->executeTime + delay);
            node->sim->event_queue.push(event);
        }
    }
    node->sim->event_queue.push(new Tell_node_to_create_txn(node, this->executeTime));
}

void Tell_node_they_rcvd_txn::execute(){
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
    if(!isStillValid) return;
    Block* block ;
    if(!(node->slow)){
        Attacker * attacker = dynamic_cast<Attacker*>(node);
        block = attacker->on_order_to_create_block(this->executeTime);
        for(auto txn : block->txns){
            attacker->private_balance[txn->senderID] -= txn->amount;
            attacker->private_balance[txn->receiverID] += txn->amount;
        }
        attacker->add_block_to_chain(block);
    }
    else{
        block = node->on_order_to_create_block(this->executeTime);
        for(auto txn : block->txns){
            node->balance[txn->senderID] -= txn->amount;
            node->balance[txn->receiverID] += txn->amount;
        }
        node->add_block_to_chain(block);
    }
    string hash_ptr = hashed_block(block);
    node->have_block_map[block->ID] = block;
    node->hashed_block_seen[hash_ptr] = 2;
    node->hash_block_reverse_map[hash_ptr] = block->ID;

    if(!(node->slow)){
        Attacker* attacker = dynamic_cast<Attacker*>(node);
        int ringmaster = attacker->ringmaster;
        if(node->ID == ringmaster){
            for(Attacker* neigh: attacker->attack_neigh){
                Link* link  = attacker->attacker_link_map[neigh];
                ld delay = link->getTotalDelay(0.064);
                Event * event = new Tell_node_they_rcvd_hashed_block(node, neigh, hash_ptr, 2,  this->executeTime + delay, 0);
                node->sim->event_queue.push(event);
            }
        }
    }
    else{
        for (Peer* neigh : node->neigh) {
            Link* link = node->link_map[neigh];
            ld delay = link->getTotalDelay(0.064);
            Event * event = new Tell_node_they_rcvd_hashed_block(node, neigh, hash_ptr, 1, this->executeTime + delay, 0);
            node->sim->event_queue.push(event);
        }
    }
    node->sim->event_queue.push(new Tell_node_to_create_block(node,this->executeTime));
}

void Tell_node_they_rcvd_hashed_block::execute(){
    DEBUG_PRINT();
    if((neigh->hashed_block_seen).find(hashed_block) == neigh->hashed_block_seen.end()){
        neigh->hashed_block_seen[hashed_block] = 1;
        neigh->string_setblock[hashed_block].insert({node, network_type});
        neigh->last_time_request[hashed_block] = this->executeTime;
        Event * event = new ask_for_block(neigh, node, hashed_block, network_type, this->executeTime, full_cast);
        neigh->sim->event_queue.push(event);
    }
    else{
        DEBUG_PRINT();
        if(neigh->hashed_block_seen[hashed_block]==1){
            neigh->string_setblock[hashed_block].insert({node, network_type});
            ld timeout = neigh->last_time_request[hashed_block];
            timeout += neigh->sim->Ttx;
            neigh->last_time_request[hashed_block] = timeout;
            set<pair<Peer*, ll>> nodes = neigh->string_setblock[hashed_block];
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, nodes.size() - 1);
            int index = dis(gen);
            int count = 0;
            Peer* node = NULL;
            ll network_type = 0;
            for(auto &x: nodes){
                if(count == index){
                    node = x.first;
                    network_type = x.second;
                    break;
                }
                count++;
            }
            Event * timeoutEvent = new BlockRequestTimeout(node, hashed_block, neigh, network_type,timeout, full_cast);
            neigh->sim->event_queue.push(timeoutEvent);
        }
        DEBUG_PRINT();
    }
}

void ask_for_block::execute(){
    DEBUG_PRINT();
    if(network_type ==1){
        if(sim_flag == 0 && full_cast ==0){
            if(node->slow  || (!(node->slow) && !(neigh->slow))){
                DEBUG_PRINT();
                Link* link = node->link_map[neigh];
                int num = node->hash_block_reverse_map[hashed_block];
                Block* block = node->have_block_map[num];
                ld delay = link->getTotalDelay(block->size);
                Event * event = new Tell_node_they_rcvd_block(neigh, block, network_type, this->executeTime + delay, full_cast);
                node->sent_map_block[block->ID].insert(neigh);
                node->sim->event_queue.push(event);
                DEBUG_PRINT();
            }
        }
        else{
            DEBUG_PRINT();
            // cerr<<node->ID<<" "<<neigh->ID<<endl;
            Link* link = node->link_map[neigh];
            int num = node->hash_block_reverse_map[hashed_block];
            DEBUG_PRINT();
            Block* block = node->have_block_map[num];
            if(block==NULL){
                cerr<<"No block found"<<endl;
                return;
            }
            ld delay = link->getTotalDelay(block->size);
            DEBUG_PRINT();
            Event * event = new Tell_node_they_rcvd_block(neigh, block, network_type, this->executeTime + delay, full_cast);
            node->sent_map_block[block->ID].insert(neigh);
            node->sim->event_queue.push(event);
            DEBUG_PRINT();
        }
    }
    else{
        DEBUG_PRINT();
        Attacker* attacker_node = dynamic_cast<Attacker*>(node);
        DEBUG_PRINT();
        int num = node->hash_block_reverse_map[hashed_block];
        DEBUG_PRINT();
        Block* block = node->have_block_map[num];
        Attacker* attacker = dynamic_cast<Attacker*>(neigh);
        Link* link = attacker_node->attacker_link_map[attacker];
        ld delay = link->getTotalDelay(block->size);
        Event * event = new Tell_node_they_rcvd_block(neigh, block, network_type, this->executeTime + delay, full_cast);
        attacker_node->attack_sent_map_block[block->ID].insert(attacker);
        node->sim->event_queue.push(event);
        DEBUG_PRINT();
    }
}

void Tell_node_they_rcvd_block::execute(){
    DEBUG_PRINT();
    string hash_ptr = hashed_block(block);
    DEBUG_PRINT();
    if(!(node->slow)){
        DEBUG_PRINT();
        Attacker* attacker = dynamic_cast<Attacker*>(node);
        if(attacker->block_map.find(block->ID)== attacker->block_map.end()){
            attacker->on_receive_block(block);
        }
        DEBUG_PRINT();
    }
    else{
        DEBUG_PRINT();
        if(node->block_map.find(block->ID) == node->block_map.end()){
            node->on_receive_block(block);
        }
        DEBUG_PRINT();
    }
    DEBUG_PRINT();
    node->have_block_map[block->ID] = block;
    node->hashed_block_seen[hash_ptr] = 2;
    node->hash_block_reverse_map[hash_ptr] = block->ID;
    DEBUG_PRINT();
    if(!(node->slow)){
        DEBUG_PRINT();
        Attacker* attacker = dynamic_cast<Attacker*>(node);
        if(full_cast == 1){
            DEBUG_PRINT();
            attacker->command_received[block->ID] = 1;
            for(Attacker* neigh: attacker->attack_neigh){
                if(neigh->command_received.find(block->ID) == neigh->command_received.end()){
                    Link* link  = attacker->attacker_link_map[neigh];
                    ld delay = link->getTotalDelay(0.064);
                    Event* event  = new give_Command_toRelease(attacker, neigh, block->ID,hash_ptr, this->executeTime + delay);
                    neigh->sim->event_queue.push(event);
                }
            }
            DEBUG_PRINT();
            attacker->releasePrivateChain(block->ID);
        }
        else{
            for(Attacker* neigh: attacker->attack_neigh){
                if(attacker->attack_sent_map_block[block->ID].find(neigh) == attacker->attack_sent_map_block[block->ID].end() &&
                    neigh->attack_sent_map_block[block->ID].find(attacker) == neigh->attack_sent_map_block[block->ID].end()){
                    Link* link  = attacker->attacker_link_map[neigh];
                    ld delay = link->getTotalDelay(0.064);
                    Event * event = new Tell_node_they_rcvd_hashed_block(node , neigh, hash_ptr, 2, this->executeTime + delay, full_cast);
                    attacker->sim->event_queue.push(event);
                }
            }
            if((block->miner->ID != attacker->ringmaster && sim_flag==1) || sim_flag==0 || full_cast==1){// that means normal block
                for (Peer* neigh : node->neigh) {
                    if(node->sent_map_block[block->ID].find(neigh) == node->sent_map_block[block->ID].end() &&
                        neigh->sent_map_block[block->ID].find(node) == neigh->sent_map_block[block->ID].end()){
                        Link* link = node->link_map[neigh];
                        ld delay = link->getTotalDelay(0.064); // 64 for hash size
                        Event* event = new Tell_node_they_rcvd_hashed_block(node, neigh, hash_ptr, 1, this->executeTime + delay, full_cast);
                        node->sim->event_queue.push(event);
                    }
                }
            }
        }
    }
    else{
        DEBUG_PRINT();
        for (Peer* neigh : node->neigh) {
            if(node->sent_map_block[block->ID].find(neigh) == node->sent_map_block[block->ID].end() &&
                neigh->sent_map_block[block->ID].find(node) == neigh->sent_map_block[block->ID].end()){
                Link* link = node->link_map[neigh];
                ld delay = link->getTotalDelay(0.064); // 64 for hash size
                Event* event = new Tell_node_they_rcvd_hashed_block(node, neigh, hash_ptr, 1, this->executeTime + delay, full_cast);
                node->sim->event_queue.push(event);
            }
        }
        DEBUG_PRINT();
    }
}

give_Command_toRelease::give_Command_toRelease(Attacker* node, Attacker* slave, int newAttack, string hashed_block, ld executeTime)
    : Event(0), node(node), slave(slave), newAttack(newAttack), hashed_block(hashed_block)
{
    this->type = 8;
    this->executeTime = executeTime;
}

void give_Command_toRelease::execute(){
    if(slave->hashed_block_seen[hashed_block]!=2){
        slave->hashed_block_seen[hashed_block] = 1;
        slave->string_setblock[hashed_block].insert({node,2});
        slave->last_time_request[hashed_block] = this->executeTime;
        Event * event = new ask_for_block(slave, node, hashed_block, 2 , this->executeTime, 1);
        slave->sim->event_queue.push(event);
    }
    else{
        slave->command_received[newAttack] = 1;
        DEBUG_PRINT();
        for(Attacker* neigh: slave->attack_neigh){
            if(neigh->command_received.find(newAttack) == neigh->command_received.end()){
                Link* link  = slave->attacker_link_map[neigh];
                ld delay = link->getTotalDelay(0.064);
                auto event  = new give_Command_toRelease(slave, neigh,newAttack, hashed_block, this->executeTime+ delay);
                neigh->sim->event_queue.push(event);
            }
        }
        slave->releasePrivateChain(newAttack);
    }
}

