#ifndef EVENT_HPP
#define EVENT_HPP

#include "headers.hpp"  

class Event {
public:
    static Simulator* sim;
    ld executeTime;

    Event(ld ts) : executeTime(ts) {}

    virtual void execute() = 0;  

    virtual ~Event() = default; 
};

class CreateTxnEvent : public Event {

public:
    Peer* peer;
    ld executeTime;
    CreateTxnEvent(Peer* p,ld ts) : Event(ts + globalTime), peer(p) {
        sim->eventQ.push(this);
    }  
    void execute() override;  
};

// class ForwardTxn : public Event {
// public:
//     ForwardTxn(ld ts) : Event(ts) {}
//     void execute() override;
// };

class ReceiveTxn : public Event {
public:
    Peer* receiver;
    Transaction* txn;

    ReceiveTxn(ld ts, Peer* recv, Transaction*txn) : Event(ts + globalTime) {
        sim->eventQ.push(this);
    }
    void execute() override;
};

class ReceiveBlk : public Event {
public:
    ReceiveBlk(ld ts) : Event(ts) {}
    void execute() override;
};

class ForwardBlk : public Event {
public:
    ForwardBlk(ld ts) : Event(ts) {}
    void execute() override;
};

class BroadcastBlk : public Event {
public:
    BroadcastBlk(ld ts) : Event(ts) {}
    void execute() override;
};

#endif // EVENT_HPP
