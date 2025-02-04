#ifndef EVENT_HPP
#define EVENT_HPP

#include "headers.hpp"  

class Event {
public:
    ld timestamp;
    bool generatesBlk;

    Event(ld ts, bool genBlk) : timestamp(ts), generatesBlk(genBlk) {}

    virtual void executeEvent() = 0;  

    virtual ~Event() = default; 
};

class GenerateTxn : public Event {
public:
    void executeEvent() override;
};

class ForwardTxn : public Event {
public:
    void executeEvent() override;
};

class ReceiveTxn : public Event {
public:
    void executeEvent() override;
};

class ReceiveBlk : public Event {
public:
    void executeEvent() override;
};

class ForwardBlk : public Event {
public:
    void executeEvent() override;
};

class BroadcastBlk : public Event {
public:
    void executeEvent() override;
};

#endif // EVENT_HPP
