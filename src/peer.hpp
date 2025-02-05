#ifndef PEER_HPP
#define PEER_HPP

#include "headers.hpp"
using namespace std;

class Peer{
public:
    static int last_id; // For maintaining current unsed id;
    int id;
    bool isfast;
    bool ishigh;
    ld Ttx;
    // int block_count;

    vector <int> balances;
    Blockchain * chain;
    vector<Link*> links;
    vector<Peer*> neighbour_peers;
    set<Transaction*> pool;
    map<int, Block *> blockMap;
    

    int tx_time;


public:
    Peer(bool isfast, bool ishigh, Blockchain* blockchain)
    : isfast(isfast), ishigh(ishigh),chain(blockchain){}

    void addLink(Link* link);
    void runTxn();
    void createTxn(ld ts);
    void receiveTxn(Transaction * );
    void broadcastTxn(Transaction* transaction);
    bool checkTxn(Transaction * txn);
};

#endif // PEER_HPP