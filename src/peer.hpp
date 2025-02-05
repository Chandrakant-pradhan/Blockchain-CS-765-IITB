#ifndef PEER_HPP
#define PEER_HPP

#include "headers.hpp"
using namespace std;

class Peer{
public:
    static int last_id;
    int id;
    bool isfast;
    bool ishigh;
    int block_count;
    int tx_time;

    vector<int> balances;
    Blockchain* chain;
    vector<Link*> links;
    vector<Peer*> neighbour_peers;
    set<Transaction*> pool;
    map<int, Block*> blockMap;

public:
    Peer(bool isfast, bool ishigh, Blockchain* blockchain)
        : id(last_id++), isfast(isfast), ishigh(ishigh), 
          chain(blockchain), block_count(0), tx_time(0) {}

    void add_link(Link* link);
    void receiveBlk(Block* block);
    void generateBlk(int T_k, int threshold_time);
    void generateTxn(int T_x, int threshold_time);
    void receiveTxn(Transaction* transaction);
    void broadcastTxn(Transaction* transaction);
    bool checkTxn(Transaction* txn);
    void generateTxn_timestamp(int T_x, int threshold_time, double curr_time);
};

#endif // PEER_HPP