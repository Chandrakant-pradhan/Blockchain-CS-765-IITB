#include "defs.hpp"

Simulation::Simulation(int n, ld z0, ld z1, ld Ttx, ld I, int timeLimit)
    : n(n), z0(z0), z1(z1), Ttx(Ttx), I(I), timeLimit(timeLimit) {
    adjacency_list.resize(n);
    create_connected_graph();
    setup_peers();
    vector<Transaction*> txn;
    global_genesis_block = new Block(0, nullptr, nullptr, txn);

}

int Simulation::getRandomNumber(int min, int max) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

bool Simulation::edgeExists(int a, int b) {
    for (int neighbor : adjacency_list[a]) {
        if (neighbor == b) return true;
    }
    return false;
}

bool Simulation::isConnected() {
    vector<bool> visited(n, false);
    queue<int> q;
    int visitedCount = 0;
    
    q.push(0);
    visited[0] = true;

    while (!q.empty()) {
        int peer = q.front();
        q.pop();
        visitedCount++;

        for (int neighbor : adjacency_list[peer]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                q.push(neighbor);
            }
        }
    }
    return visitedCount == n; 
}

void Simulation::create_connected_graph() {
    bool connected = false;
    
    while (!connected) {
        for (auto& neighbors : adjacency_list) {
            neighbors.clear();
        }
        for (int i = 0; i < n; ++i) {
            while (adjacency_list[i].size() < 3) { 
                int peer = getRandomNumber(0, n - 1);
                if (peer != i && adjacency_list[i].size() < 6 && adjacency_list[peer].size() < 6 && !edgeExists(i, peer)) {
                    adjacency_list[i].push_back(peer);
                    adjacency_list[peer].push_back(i); 
                }
            }
        }
        connected = isConnected();
    }
}

void Simulation::run() {
    while (global_time < timeLimit && !event_queue.empty()) {
        Event* event = event_queue.top();
        event_queue.pop();
        
        global_time = event->executeTime;

        event->execute();
        
        //here it is wrong might give teo transactions at a time 
        // so it is more of thing thing 
        // add this after you created the block you will add the seed
        // of next block generation in the same function
        // for (int i = 0; i < n; i++) {
        //     if(peer_map[i]->isMakingBlock) {
        //         continue;
        //     }
        //     else{
        //         event_queue.push(new Tell_node_to_create_txn(peer_map[i], global_time));
        //     }
        //     if(peer_map[i]->isMakingBlock){
        //         continue;
        //     }
        //     else{
        //         event_queue.push(new Tell_node_to_create_block(peer_map[i], global_time));
        //     }
        // }
        delete event;
    }
}

bool probabilisticYes(ld probability) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_real_distribution<ld> dis(0.0, 100.0);
    return dis(gen) < probability; 
}

ld generatePropagationDelay() {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_real_distribution<ld> dis(10.0, 500.0); 
    return dis(gen) / 1000.0; 
}

ld generateLinkSpeed(bool isFast_i, bool isFast_j) {
    return (isFast_i && isFast_j) ? (100000.0 / 8.0) : (5000.0 / 8.0);
}

void Simulation::setup_peers() {
    int no_of_low = 0;
    for (int i = 0; i < n; i++) {
        bool is_slow = probabilisticYes(z0);
        bool is_low = probabilisticYes(z1);
        no_of_low += is_low;
        peer_map[i] = new Peer(this, i, is_slow, is_low);
    }

    for (int i = 0; i < n; i++) {
        for (int j : adjacency_list[i]) {
            if (i < j) {
                ld pij = generatePropagationDelay();
                ld cij = generateLinkSpeed(peer_map[i]->slow, peer_map[j]->slow);
                Link* link = new Link(pij, cij);
                peer_map[i]->link_map[peer_map[j]] = link;
                peer_map[j]->link_map[peer_map[i]] = link;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j : adjacency_list[i]) {
            peer_map[i]->neigh.insert(peer_map[j]);
            peer_map[j]->neigh.insert(peer_map[i]);
        }
    }

    for (int i = 0; i < n; i++) {
        peer_map[i]->block_chain.push_back(global_genesis_block->make_copy_for_peer(peer_map[i]));        
    }

    ld low_hk = 1.0 / (no_of_low + (n - no_of_low) * 10);
    for (int i = 0; i < n; i++) {
        peer_map[i]->hk = peer_map[i]->low ? low_hk : 10 * low_hk;
    }

    // start it is ok to have this but better not to have at those steps

    for (int i = 0; i < n; i++) {
        event_queue.push(new Tell_node_to_create_txn(peer_map[i], 0));
        event_queue.push(new Tell_node_to_create_block(peer_map[i], 0));
    }
}
