#ifndef EVENT_HPP
#define EVENT_HPP

#include "headers.hpp"  

class Event {
public:
    ld timestamp;

    Event(ld ts) : timestamp(ts) {}

    virtual void execute() = 0;  

    virtual ~Event() = default; 
};

class GenerateTxn : public Event {
public:
    GenerateTxn(ld ts) : Event(ts) {}  
    void execute() override;  
};

class ForwardTxn : public Event {
public:
    ForwardTxn(ld ts) : Event(ts) {}
    void execute() override;
};

class ReceiveTxn : public Event {
public:
    ReceiveTxn(ld ts) : Event(ts) {}
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
