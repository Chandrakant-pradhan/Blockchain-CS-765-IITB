#include<bits/stdc++.h>
using namespace std;
#include <cstdlib>  // For rand()
#include <ctime>    // For seeding rand()

using namespace std;

void dfs(int node, vector<vector<int>>& adj, vector<bool>& visited) {
    visited[node] = true;
    for (auto x : adj[node]) {
        if (!visited[x]) {
            dfs(x, adj, visited);
        }
    }
}

bool graph_is_disconnected(vector<peer>& peers) {
    int n = peers.size();
    vector<vector<int>> adjacency_graph(n);

    for (int i = 0; i < n; i++) {
        for (auto neighbor : peers[i].adjacent_nodes) {
            adjacency_graph[i].push_back(neighbor - &peers[0]); // Get index
        }
    }
    vector<bool> visited(n, false);
    dfs(0, adjacency_graph, visited);
    for (int i = 0; i < n; i++) {
        if (!visited[i]) return true;
    }
    return false;
}

void connected_graphs(vector<peer>& peers){
    int n = peers.size();
    while (true) {
        for (int i = 0; i < n; i++) {
            peers[i].adjacent_nodes.clear();
        }

        for (int i = 0; i < n; i++) {
            int existing_connections = peers[i].adjacent_nodes.size();
            int num_neighbors = 3 - existing_connections + rand() % 4;
            set<int> neighbors;
            while ((int)neighbors.size() < num_neighbors) {
                int new_peer = rand() % n;
                if (new_peer != i && neighbors.find(new_peer) == neighbors.end()) {
                    neighbors.insert(new_peer);
                    peers[i].adjacent_nodes.push_back(&peers[new_peer]);
                    peers[new_peer].adjacent_nodes.push_back(&peers[i]);
                }
            }
        }

        if (graph_is_disconnected(peers)) {
            continue;
        } else {
            break;
        }
    }
}

vector<peer> assign_slow_fast(vector<peer>& peers, int z0){
    // random z0 percent assign slow_fast
    int n = peers.size() * (z0/100);
    set<int>selected;
    while(selected.size() < n){
        int random_peer = rand()%peers.size();
        if(selected.find(random_peer) == selected.end()){
            peers[random_peer].is_slow = true;
            selected.insert(random_peer);
        }
    }
    return peers;
}

vector<peer> assign_low_high(vector<peer>& peers, int z1){
    // random z1 percent assign low_high
    int n = peers.size() * (z1/100);
    set<int> selected;
    while(selected.size() < n){
        int random_peer = rand()%peers.size();
        if(selected.find(random_peer) == selected.end()){
            peers[random_peer].is_low = true;
            selected.insert(random_peer);
        }
    }
    return peers;
}

void simulation(vector<peer>& peers){

    // number of peers
    int n = peers.size();

    //assign fast and slow peers
    peers = assign_slow_fast(peers);
    peers = assign_low_high(peers);

    // initialize graph
    connected_graphs(peers);

    // assign latencies;
    for(auto& x: peers){
        vector<Link*>links
        for(auto* neighbour : peer.neighbour_peers){
            int p;
            int c;
            Link* new_link = new Link(neighbour, p, c);
            links.push_back(new_link);
        }
        x.links = links;
    }

    

}