#include "defs.hpp"

//instantiating class
Simulation::Simulation(int n, ld z0, ld z1, ld Ttx, ld Tty, ld I, int timeLimit)
    : n(n), z0(z0), z1(z1), Ttx(Ttx), Tty(Tty), I(I), timeLimit(timeLimit) {
    vector<Transaction*> txn;
    global_genesis_block = new Block(0, nullptr, nullptr, txn);
    adjacency_list.resize(n);
    create_connected_graph();
    setup_peers();
}

//utility function to get random number in between min and max
int Simulation::getRandomNumber(int min, int max) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

//utility function to check if an edge exist between ID a and b
bool Simulation::edgeExists(int a, int b, vector<vector<int> > & adjacency_list){
    for (int neighbor : adjacency_list[a]) {
        if (neighbor == b) return true;
    }
    return false;
}

//BFS traversal to check connectivity of graph
bool Simulation::isConnected(vector<vector<int>>& adjacency_list){
    ll n  = adjacency_list.size();
    if(n==0) return true;
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

//function to create connected graph with constraints on degree
void Simulation::create_connected_graph() {
    bool connected = false;
    ll n= adjacency_list.size();
    
    while (!connected) {
        for (auto& neighbors : adjacency_list) {
            neighbors.clear();
        }
        for (int i = 0; i < n; ++i) {
            while (adjacency_list[i].size() < 3) { 
                int peer = getRandomNumber(0, n - 1);
                if (peer != i && adjacency_list[i].size() < 6 && adjacency_list[peer].size() < 6 && !edgeExists(i, peer, adjacency_list)) {
                    adjacency_list[i].push_back(peer);
                    adjacency_list[peer].push_back(i); 
                }
            }
        }
        connected = isConnected(adjacency_list);
    }
}


void Simulation::create_attack_graph() {
    ll k = attack_list.size();
    map<int, int> front_map;
    ll tt =0;
    for(auto &x: attack_list){
        front_map[tt] = x.first;
        tt++;
    }
    vector<vector<int>> adjacent_attack;
    adjacent_attack.resize(k);

    bool connected = false;
    while (!connected) {
        for (auto& neighbors : adjacent_attack) {
            neighbors.clear();
        }
        for (int i = 0; i < k; ++i) {
            while ((ll)adjacent_attack[i].size() < min(3LL, k-1)) { 
                int peer = getRandomNumber(0, k - 1);
                if (peer != i && adjacent_attack[i].size() < 6 && adjacent_attack[peer].size() < 6 && !edgeExists(i, peer, adjacent_attack)) {
                    adjacent_attack[i].push_back(peer);
                    adjacent_attack[peer].push_back(i); 
                }
            }
        }
        connected = isConnected(adjacent_attack);
    }

    for(int i = 0; i< (ll)adjacent_attack.size(); i++){
        int a = front_map[i];
        for(auto &x : adjacent_attack[i]){
            ll k = front_map[x];
            attack_list[a].push_back(k);
        }
    }
    adjacent_attack.clear();
}

//function to execute events from event queue
void Simulation::run() {
    while (global_time < timeLimit && !event_queue.empty()) {
        Event* event = event_queue.top();
        // cerr<<event->type<<"type 1"<<endl;
        event_queue.pop();
        global_time = event->executeTime;
        if(event){
            event->execute();
            delete event;
        }
    }

    while(!event_queue.empty()) {
        Event* event = event_queue.top();
        if((event->type == 1 || event->type >=4) && (event->type !=7)){
            // cer/r<<event->type<<"type 2"<<endl;
            if(event){
                event->execute();
            }
        }
        global_time = event->executeTime;
        event_queue.pop();
        if (event) {
            delete event;
        }
    }

    // event where gets ended may be some sort of 
    // release chain event
    Attacker* ring  = dynamic_cast<Attacker*>(peer_map[ringmaster]);
    ring->commandRelease(ring->private_mining_block->ID, ring->private_mining_block);
    while(!event_queue.empty()) {
        Event* event = event_queue.top();
        if((event->type == 1 || event->type >=4) && (event->type !=7)){
            // cerr<<event->type<<"type 3"<<endl;
            if(event){
                event->execute();
            }
        }
        global_time = event->executeTime;
        event_queue.pop();
        if (event) {
            delete event;
        }
    }

}

//function returning true with some probability
bool probabilisticYes(ld probability) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_real_distribution<ld> dis(0.0, 100.0);
    return dis(gen) < probability; 
}

//function to get random propogation delay
ld generatePropagationDelay() {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_real_distribution<ld> dis(10.0, 500.0); 
    return dis(gen) / 1000.0; 
}


ld generatePropagation_attackDelay(){
    static random_device rd;
    static mt19937 gen(rd());
    uniform_real_distribution<ld> dis(1.0, 10.0); 
    return dis(gen) / 1000.0;
}
//function to link speed
ld generateLinkSpeed(bool isSlow_i, bool isSlow_j) {
    return (isSlow_i || isSlow_j) ? (5000.0) : (100000.0);
}


void Simulation::setup_peers() {
    int no_of_low = 0;

    //setting low and slow property of peer
    ll count_attackers = 0;
    for (int i = 0; i < n; i++) {
        bool is_slow = probabilisticYes(z0);
        bool is_low = probabilisticYes(z1);
        no_of_low += is_low;
        if(is_slow){
            peer_map[i] = new Peer(this, i, is_slow, is_low);  // slow are normal nodes and fast is malicious node
        }
        else{
            peer_map[i] = new Attacker(this, i, is_slow, is_low);  // normal nodes are normal nodes and fast is malicious node
            attacker_map[i] = dynamic_cast<Attacker*>(peer_map[i]);
            vector<int>rh;
            cout<<i<<"attackers_"<<endl;
            attack_list[i] = rh;
            count_attackers++;
        }
    }

    create_attack_graph();
    //creating delay links

    for(auto & x: attack_list){
        for(auto & y: attack_list[x.first]){
        if(x.first  < y){
            ld pij = generatePropagation_attackDelay();
            ld cij = generateLinkSpeed(attacker_map[x.first]->slow, attacker_map[y]->slow);
            Link* link = new Link(pij, cij);
            attacker_map[x.first]->attacker_link_map[attacker_map[y]] = link;
            attacker_map[y]->attacker_link_map[attacker_map[x.first]] = link;
            }
        }
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

    //creating neighbour sets of attackers
    for (auto & x: attack_list) {
        ll i = x.first;
        for (int j : attack_list[i]) {
            attacker_map[i]->attack_neigh.insert(attacker_map[j]);
            attacker_map[j]->attack_neigh.insert(attacker_map[i]);
        }
    }

    //creating neighbour sets of peers
    for (int i = 0; i < n; i++) {
        for (int j : adjacency_list[i]) {
            peer_map[i]->neigh.insert(peer_map[j]);
            peer_map[j]->neigh.insert(peer_map[i]);
        }
    }
    
    //adding genesis block to blockchain of all peers
    for (int i = 0; i < n; i++) {
        peer_map[i]->block_chain.push_back(global_genesis_block->make_copy_for_peer(peer_map[i]));        
    }
    
    //defining the hashing power of each peer
    ld low_hk = 1.0 / (no_of_low + (n - no_of_low) * 10);
    for (int i = 0; i < n; i++) {
        peer_map[i]->hk = peer_map[i]->low ? low_hk : 10 * low_hk;
    }


    //mixing of hash powers of malicious powers
    if(attack_list.size()>0) {
        set<int> temp;
        for (auto & x: attack_list) {
            temp.insert(x.first);
        }
        int rm = getRandomNumber(0, temp.size() - 1);
        int count=0;
        for(auto & x: attack_list) {
            if(count == rm){
                ringmaster = x.first;
                break;
            }
            count++;
        }
        for(auto & x: attack_list){
            attacker_map[x.first]->ringmaster = ringmaster;
        }
        ld total_hash =0;
        for(auto & x: attack_list){
            if(!(peer_map[x.first]->slow)){  // if the nodes are fast they are malicious
                total_hash += peer_map[x.first]->hk;
                peer_map[x.first]->hk = 0;
                attacker_map[x.first]->hk = 0;
            }
        }
        peer_map[ringmaster]->hk =  total_hash;
        attacker_map[ringmaster]->hk = total_hash;
        cout<<ringmaster<<"ringmaster"<<endl;
    }


    const string filename = "graph_struct.txt";
    ofstream outfile(filename); // Open file for writing

    if (!outfile) {
        cerr << "Error: Could not open the file!" << endl;
        return;
    }

    // Write attack_list
    outfile << "attack_list\n";
    for (auto &x : attack_list) {
        outfile << x.first << ":";
        for (auto &y : x.second) {
            outfile << y << " ";
        }
        outfile << "\n";
    }

    // Write adjacency_list
    outfile << "\nadjacent_list\n";
    for (int i = 0; i < n; i++) {
        outfile << i << ":";
        for (auto &y : adjacency_list[i]) {
            outfile << y << " ";
        }
        outfile << "\n";
    }

    outfile.close();

    //initializing balances
    for(int i=0 ; i<n ; i++){
        (peer_map[i]->balance).assign(n, 0);
        if(!(peer_map[i]->slow)){
            Attacker* a = dynamic_cast<Attacker*>(peer_map[i]);
            (a->balance).assign(n, 0);
            (a->private_balance).assign(n, 0);
        }
    }

    //pushing intial event and blocks
    for (int i = 0; i < n; i++) {
        auto eventTxn = new Tell_node_to_create_txn(peer_map[i], 0);
        if(peer_map[i]->slow || peer_map[i]->ID == ringmaster){
        auto eventB = new Tell_node_to_create_block(peer_map[i], 0);
        peer_map[i]->createProc = eventB;
        event_queue.push(eventB);
        }
        event_queue.push(eventTxn);
    }

}