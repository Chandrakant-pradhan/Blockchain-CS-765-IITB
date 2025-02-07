#ifndef DEFS_HPP
#define DEFS_HPP

#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <unordered_set>

using namespace std;

typedef long long ll;
typedef unsigned int uint;
typedef unsigned long long ull;
typedef long double ld;





// Forward declarations
class Event;
class Peer;
class Block;
class Transaction;
class Link;
class Simulation;

// Event class
class Event {
public:
    ld executeTime;
    Event(ld executeTime);
    virtual void execute() = 0;
    virtual ~Event() = default;
    struct Compare {
        bool operator()(const Event* a, const Event* b) const {
            return a->executeTime > b->executeTime;
        }
    };
};

// Link class
class Link {
public:
    ld pij;
    ld cij;
    ld dij;
    Link(ld pij, ld cij);
    void compute_queue_delay();
    ld getTotalDelay(int msgSize);//msgSize in kilobits
};

// Transaction class
class Transaction {
public:
    static int counter;
    int ID;
    ld timestamp;
    int senderID;
    int receiverID;
    int amount;
    int size;

    Transaction(ld ts, int senderID, int receiverID, int amt);
    Transaction* make_copy();
};

// Block class
class Block {
public:
    static int counter;
    int ID;
    ld timestamp;
    int depth;
    int size;
    Block* parentBlock;
    Peer* miner;
    vector<Transaction*> txns;
    vector<Block*> childBlocks;
    Block(ld ts, Block* parent, Peer* miner, vector<Transaction*> &txns);
    Block* make_copy_for_peer(Peer* other);
    int computeSize();
    int computeDepth();
};

// Peer class
class Peer {
public:

    Simulation* sim;
    int ID;
    int slow;
    int low;
    ld hk;
    
    bool isMakingTxn = false;
    bool isMakingBlock = false;

    map<int, Block*> block_map;
    map<int, Transaction*> txn_map;
    map<int, int> balance;
    map<Transaction*, set<Peer*>> sent_map_txn;
    map<Block*, set<Peer*>> sent_map_block;
    map<Peer*, Link*> link_map;

    set<Peer*> neigh;
    vector<Block*> block_chain;
    vector<Block*> longest_chain_tips;
    set<Transaction*> txn_pool;

    Peer(Simulation* sim, int ID, bool slow, bool low);
    virtual ~Peer() = default;

    Peer* get_random_peer();
    int get_random_money();
    Transaction* on_order_to_create_txn(ld timestamp);
    void on_receive_txn(Transaction* txn);
    Block* on_order_to_create_block(ld timestamp);
    void on_receive_block(Block* block);
    void add_block_to_chain(Block* block);
};

// Simulation class
class Simulation {
public:
    ld global_time = 0;
    map<int, Peer*> peer_map;
    vector<vector<int>> adjacency_list;
    Block* global_genesis_block;
    priority_queue<Event*, vector<Event*>, Event::Compare> event_queue;
    int n;
    ld z0 = 20, z1 = 20;
    ld Ttx = 5.0, I = 600;
    int timeLimit = 3600;
    int transaction_size = 8; // kilobits
    int max_block_size = 8000; // kilobits
    int miner_reward = 50; // coins

    Simulation(int n, ld z0, ld z1, ld Ttx, ld I, int timeLimit);

    void run();
    void create_connected_graph();
    void setup_peers();
    int getRandomNumber(int min, int max);
    bool edgeExists(int a, int b);
    bool isConnected();
};


// Transaction event classes
class Tell_node_to_create_txn : public Event {
public:
    Peer* node;
     //simulator telling node to create a txn at curr_time + random time
    Tell_node_to_create_txn(Peer* node, ld curr_time);
    void execute();
};

class Tell_node_they_rcvd_txn : public Event {
public:
    Peer* node;
    Transaction* txn;
    //an event that tells a node that they will receive a txn at execution time
    Tell_node_they_rcvd_txn(Peer* node, Transaction* txn, ld executeTime);
    void execute();
};

// Block event classes
class Tell_node_to_create_block : public Event {
public:
    Peer* node;
    //simulator telling node to create a block at curr_time + random time
    Tell_node_to_create_block(Peer* node, ld curr_time);
    void execute();
};

class Tell_node_they_rcvd_block : public Event {
public:
    Peer* node;
    Block* block;
    //an event that tells a node that they will receive a block at execution time
    Tell_node_they_rcvd_block(Peer* node, Block* block, ld executeTime);
    void execute();
};

#endif // DEFS_HPP
