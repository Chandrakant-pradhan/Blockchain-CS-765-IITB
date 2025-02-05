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

Simulation::Simulation(int n, double z0, double z1, double Ttx, double Tk, int timeLimit)
    : n(n), z0(z0), z1(z1), Ttx(Ttx), Tk(Tk), timeLimit(timeLimit) {
    setup_peers();
}

void Simulation::run() {
    generate_initial_events();

    while (!event_queue.empty()) {
        Event* current_event = event_queue.top();
        event_queue.pop();

        if (current_event->timestamp > timeLimit) {
            delete current_event;
            break;
        }

        process_event(current_event);
        generate_other_events(current_event);

        delete current_event;
    }
}

void Simulation::assign_slow_fast() {
    int slow_count = n * (z0 / 100.0);
    std::vector<int> indices(n);
    for (int i = 0; i < n; ++i) indices[i] = i;
    std::random_shuffle(indices.begin(), indices.end());
    for (int i = 0; i < slow_count; ++i) {
        peers[indices[i]].is_slow = true;
    }
}

void Simulation::assign_low_high() {
    int low_count = n * (z1 / 100.0);
    std::vector<int> indices(n);
    for (int i = 0; i < n; ++i) indices[i] = i;
    std::random_shuffle(indices.begin(), indices.end());
    for (int i = 0; i < low_count; ++i) {
        peers[indices[i]].is_low = true;
    }
}

void Simulation::create_connected_graph() {
    while (true) {
        for (Peer& peer : peers) {
            peer.adjacent_nodes.clear();
        }

        for (int i = 0; i < n; i++) {
            int num_neighbors = 3 + rand() % 4;
            set<int> neighbors;
            while (neighbors.size() < num_neighbors) {
                int new_peer = rand() % n;
                if (new_peer != i && neighbors.find(new_peer) == neighbors.end()) {
                    neighbors.insert(new_peer);
                    peers[i].adjacent_nodes.push_back(&peers[new_peer]);
                    peers[new_peer].adjacent_nodes.push_back(&peers[i]);
                }
            }
        }

        if (!graph_is_disconnected(peers)) {
            break;
        }
    }
}

void Simulation::setup_peers() {
    peers.reserve(n);
    for (int i = 0; i < n; ++i) {
        peers.emplace_back(i);
    }

    assign_slow_fast();
    assign_low_high();
    create_connected_graph();

    // Set up links between peers
}

void Simulation::generate_initial_events() {
    // Generate initial events for each peer
    for (auto& peer : peers) {
        event_queue.push(new Event(Event::generateTxn_event, &peer, 0, 0));
        event_queue.push(new Event(Event::generateBlk_event, &peer, 0, 0));
    }
}

void Simulation::process_event(Event* event) {
    Peer* owner = event->owner;
    double timestamp = event->timestamp;
    int msg_size = event->msg_size;

    switch (event->type) {
        case Event::generateTxn_event:
            owner->createTxn(timestamp);
            break;
        case Event::forwardTxn_event:
            owner->broadcastTxn(timestamp, msg_size);
            break;
        case Event::ReceiveTxn_event:
            owner->receiveTxn(timestamp, msg_size);
            break;
        case Event::Broadcast_event:
            owner->broadcastBlk(timestamp);
            break;
        case Event::broadcastmined_event:
            owner->broadcastMinedBlk(timestamp);
            break;
        case Event::ReceiveBlk_event:
            owner->receiveBlk(timestamp, msg_size);
            break;
        case Event::forwardBlk_event:
            owner->receiveMinedBlk(timestamp, msg_size);
            break;
    }
}

void Simulation::generate_other_events(Event* event) {
    // Implement logic to generate follow-up events based on the current event
    // This may include scheduling new transaction generations, block creations, etc.
}
