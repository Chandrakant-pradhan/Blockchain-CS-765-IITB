#include "event.hpp"
#include "peer.hpp"

void CreateTxnEvent::execute(){
    Peer * owner = this->peer;
    peer->createTxn(globalTime);
    peer->runTxn();
}