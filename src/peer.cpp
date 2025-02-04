#include "peer.hpp"
#include <iostream>

int Peer:: last_id = 0;

Peer::Peer(bool fast, bool high, Blockchain* blkch)
: isfast(fast), ishigh(high){
    
}