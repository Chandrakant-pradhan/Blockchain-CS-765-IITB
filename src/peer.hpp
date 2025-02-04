#ifndef PEER_HPP
#define PEER_HPP

#include "headers.hpp"
using namespace std;

class Peer{
private:
    static int last_id; // For maintaining current unsed id;
    int id;
    bool isfast;
    bool ishigh;
    vector <int> balances;
    Blockchain * chain;
    Block* latest_block;
    vector<Link*> links;
    int block_count;
    vector<Peer*> neighbour_peers;
    set<Transaction*> pool;
    int tx_time;

public:
    Peer(bool isfast, bool ishigh, Blockchain* blockchain);
    void add_link(Link* link);
    void broadcast_transaction(Transaction* transaction);
};

#endif // PEER_HPP

