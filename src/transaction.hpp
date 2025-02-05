#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include "headers.hpp"  

class Transaction {
public:
    static int counter;
    int ID;
    ld timestamp;
    Peer* sender;  
    Peer* receiver;
    int amount;
    int size = 1; // 1 Kb
    
    Transaction(ld ts, Peer* send, Peer* recv , int amt) 
        : timestamp(ts), sender(send), receiver(recv) , amount(amt){
        counter++;
        ID = counter;  
    }
};

#endif // TRANSACTION_HPP
