#ifndef DEFS_HPP
#define DEFS_HPP

#include <iostream>
#include <openssl/sha.h>
#include <string>
#include <iomanip> 
#include <random>
#include <chrono>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace std;

typedef long long ll;
typedef unsigned int uint;
typedef unsigned long long ull;
typedef long double ld;

extern std::chrono::steady_clock::time_point start_time;
#define DEBUG_PRINT()
extern int sim_flag; // Global simulation flag
extern int last_seen_event; // Last event
extern set<int> st;

// Forward declarations
class Event;
class Peer;
class Block;
class Transaction;
class Link;
class Simulation;

// Event Derived classes
class Tell_node_to_create_txn;
class Tell_node_they_rcvd_txn;
class Tell_node_to_create_block;
class Tell_node_they_rcvd_block;
class Tell_node_they_rcvd_hashed_block;
class ask_for_block;
class BlockRequestTimeout;
class give_Command_toRelease;

// Event class

class Event {
public:
     /*
    - executeTime: The time at which the event is scheduled to execute.
    - Compare: A comparator for ordering events in a priority queue.
    */
    ld executeTime;
    int type;
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
     /*
    - pij: Propagation delay of the link.
    - cij: Link capacity.
    - dij: Queueing delay of the link.
    */
    ld pij;
    ld cij;
    ld dij;
    Link(ld pij, ld cij);
    void compute_queue_delay();
    ld getTotalDelay(ld msgSize);
};

// Transaction class
class Transaction {
public:
    /*
      - counter: Static counter to assign unique IDs.
      - ID: Unique ID for the transaction.
      - timestamp: Time when the transaction is created.
      - senderID: ID of the sender.
      - receiverID: ID of the receiver.
      - amount: Amount transferred in the transaction.
      - size: Size of the transaction in kilobits.
    */
    static int counter;
    int ID;
    ld timestamp;
    int senderID;
    int receiverID;
    int amount;
    ld size;
    Transaction(ld ts, int senderID, int receiverID, int amt);

    bool operator<(const Transaction& other) const {
        if (timestamp == other.timestamp)
            return ID < other.ID;
        return timestamp < other.timestamp;
    }
    
    Transaction* make_copy();

};

// Block class
class Block {
public:
    /*
      - counter: Static counter to assign unique IDs.
      - ID: Unique ID for the block.
      - timestamp: Time when the block is created.
      - depth: Depth of the block in the blockchain.
      - size: Size of the block in kilobits.
      - parentBlock: Pointer to the parent block in the chain.
      - miner: Pointer to the Peer who mined this block.
      - txns: List of transactions included in this block.
      - childBlocks: List of child blocks extending this block.
    */
    static int counter;
    int ID;
    ld timestamp;
    int depth;
    ld size;
    Block* parentBlock;
    Peer* miner;
    vector<Transaction*> txns;
    vector<Block*> childBlocks;
    Block(ld ts, Block* parent, Peer* miner, vector<Transaction*> &txns);
    Block* make_copy_for_peer(Peer* other);
    ld computeSize();
    int computeDepth();
};

// Peer class
class Peer {
public:
    /*
      - sim: Pointer to the Simulation instance.
      - ID: Unique ID for the peer.
      - slow: Indicates if the peer has a slow connection.
      - low: Indicates if the peer has low hashing power.
      - hk: Hashing power of the peer.
      - numBlocks: Number of blocks mined by the peer.
      - block_map: Mapping block ID to block pointers.
      - txn_map: Mapping txn ID to txn pointers.
      - balance: List storing balances of all peers.
      - sent_map_txn: Tracks transactions forwarded to peers.
      - sent_map_block: Tracks blocks forwarded to peers.
      - link_map: Map for storing link info with neigh.
      - genesisLocal: Pointer to the local genesis block.
      - miningBlock: Tip of blockchain
      - neigh: Set of neighboring peers.
      - block_chain: Vector of blocks forming the blockchain.
      - longest_chain_tips: Vector of tips of the longest chain.
      - txn_pool: Set of idle txns.
      - block_pool: Set of orphaned blocks.
      - createProc: Pointer to the block creation event for the peer.
    */
    Simulation* sim;
    int ID;
    int slow;
    int low;
    ld hk;
    int numBlocks;
    map<int, Block*> block_map;
    map<int, Block*> have_block_map; // which block ID refers to which hash
    map<string, int> hash_block_reverse_map; // which block number refers to which hash
    map<int, Transaction*> txn_map;
    map<string, int> hashed_block_seen;
    map<string, set<pair<Peer*,ll>>> string_setblock;
    map<string, ld> last_time_request;
    vector<int> balance;
    map<int, set<Peer*>> sent_map_txn;
    map<int, set<Peer*>> sent_map_block;
    map<Peer*, Link*> link_map;
    Block* genesisLocal;
    Block* miningBlock;

    set<Peer*> neigh;
    vector<Block*> block_chain;
    vector<Block*> longest_chain_tips;
    set<Transaction*> txn_pool;
    set<Block*> block_pool;
    Tell_node_to_create_block* createProc;

    Peer(Simulation* sim, int ID, bool slow, bool low);
    virtual ~Peer() = default;

    Peer* get_random_peer();
    int get_random_money();
    Transaction* on_order_to_create_txn(ld timestamp);
    void on_receive_txn(Transaction* txn);
    int countMyBlks();
    int total_block();
    virtual Block* on_order_to_create_block(ld timestamp);
    virtual void on_receive_block(Block* block);
    virtual void add_block_to_chain(Block* block);
    void change_branch(Block* newBlock);
};


class Attacker : public Peer {
public:
    int ringmaster;
    map<int, int>command_received;
    set<Attacker*> attack_neigh;
    map<int, set<Attacker*>> attack_sent_map_block;
    map<Attacker*, Link*> attacker_link_map;

    vector<int> private_balance;
    set<Transaction*> private_txn_pool;
    Block * forkingPoint;
    vector<Block*> private_block_chain;
    Block* private_mining_block;

    // parent derived functions specific to attacker
    void add_block_to_chain(Block* block) override;
    Block* on_order_to_create_block(ld timestamp) override;
    void on_receive_block(Block* block) override;


    void commandRelease(int newPoint, Block* block);
    void releasePrivateChain(int newAttackPoint);
    int getPrivateChainLength();
    Attacker(Simulation* sim, int ID, bool slow, bool low);
}
;

// Simulation class
class Simulation {
public:
    /*
      - global_time: Tracks current time.
      - peer_map: Mapping peer ID to peer pointers.
      - adjacency_list: Graph edges.
      - event_queue: Priority queue for scheduling events.
      - n: Number of peers.
      - z0, z1: Percentages of slow and low nodes.
      - Ttx: Average txn inter-arrival time.
      - Tty: Average request again time.
      - I: Average Block inter-arrrival time.
      - timeLimit: Max time to run simulation.
      - transaction_size: Size of a txn in kilobits.
      - max_block_size: Maximum block size in kilobits.
      - miner_reward: Reward for mining a block.
    */
    ld global_time = 0;
    int ringmaster;
    map<int, Peer*> peer_map;
    map<int, Attacker*> attacker_map;
    vector<vector<int>> adjacency_list;
    map<int,vector<int>> attack_list;
    Block* global_genesis_block;
    priority_queue<Event*, vector<Event*>, Event::Compare> event_queue;
    int n;
    ld z0 = 20, z1 = 20;
    ld Ttx = 5.0, Tty = 0.5, I = 600;
    int timeLimit = 3600;
    int transaction_size = 8; // kilobits
    int max_block_size = 8000; // kilobits
    int miner_reward = 50; // coins

    Simulation(int n, ld z0, ld z1, ld Ttx, ld Tty, ld I, int timeLimit);

    void run();
    void create_connected_graph();
    void create_attack_graph();
    void setup_peers();
    int getRandomNumber(int min, int max);
    bool edgeExists(int a, int b, vector<vector<int> > & adjacency_list);
    bool isConnected(vector<vector<int> > & a);
};


// Transaction event classes
class Tell_node_to_create_txn : public Event {
public:
    /*
      - node: Pointer to the peer creating the transaction.
    */
    Peer* node;
    Tell_node_to_create_txn(Peer* node, ld curr_time);
    void execute();
};

class ask_for_block : public Event {
    /*
      - node: Pointer to the peer asking for the block.
      - blockID: ID of the block being asked for.
    */
public:
    Peer* neigh; // this requester
    Peer* node; // this is receiver at that end so node
    string hashed_block;
    ll network_type;
    int full_cast;
    // 1 for normal network , 2 for overlay network
    ask_for_block(Peer* neigh, Peer* node, string hashed_block,ll network_type, ld executeTime, int full_cast);
    void execute();
}
;

class Tell_node_they_rcvd_txn : public Event {
public:
    /*
      - node: Pointer to the receiving peer.
      - txn: Pointer to the transaction being received.
    */
    Peer* node;
    Transaction* txn;
    Tell_node_they_rcvd_txn(Peer* node, Transaction* txn, ld executeTime);
    void execute();
};

// Block event classes
class Tell_node_to_create_block : public Event {
public:
    Peer* node;
    bool isStillValid;
    //simulator telling node to create a block at curr_time + random time
    Tell_node_to_create_block(Peer* node, ld curr_time);
    void execute();
};

class Tell_node_they_rcvd_block : public Event {
public:
    Peer* node;
    Block* block;
    int network_type;
    int full_cast;
    //an event that tells a node that they will receive a block at execution time
    Tell_node_they_rcvd_block(Peer* node, Block* block, int network_type, ld executeTime, int full_cast);
    void execute();
};

class Tell_node_they_rcvd_hashed_block : public Event {
public:
    Peer* node;
    Peer* neigh;
    string hashed_block;
    ll network_type;
    int full_cast;
    Tell_node_they_rcvd_hashed_block(Peer* node, Peer* neigh, string hashed_block, ll network_type, ld executeTime, int full_cast);
    void execute();
}
;

class BlockRequestTimeout : public Event {
public:
    Peer* node;
    string hashed_block;
    Peer* neigh;
    ll network_type;
    int full_cast;
    BlockRequestTimeout(Peer* node, string hashed_block, Peer* neigh, ll network_type, ld executeTime, int full_cast);
    // neigh is requester everywhere remember;
    void execute();
};

class give_Command_toRelease : public Event{
    public:
    Attacker * node;
    Attacker * slave; // the slave to which the command was given
    int newAttack; // the new attack to be given to slave;
    string hashed_block;
    ld executeTime;
    give_Command_toRelease(Attacker* node, Attacker* slave, int newAttack, string hashed_block, ld executeTime);
    void execute();
}
;

inline std::string hashed_block(Block* block){
    stringstream ss;
    ss << block->ID << "_" << block->timestamp << "_"<< (block->miner ? block->miner->ID : -1);
    std::string input_str = ss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input_str.c_str(), input_str.size(), hash); // Hash computation

    std::stringstream hash_ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        hash_ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return hash_ss.str();
}
#endif // DEFS_HPP