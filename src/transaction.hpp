#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include "headers.hpp"  

class Transaction {
public:
    static int counter;
    int ID;
    ld timestamp;
    int senderID;  
    int receiverID;
    int amount;
    int size = 1; // 1 Kb
    
    Transaction(ld ts, int send, int recv , int amt) 
        : timestamp(ts), senderID(send), receiverID(recv) , amount(amt){
        counter++;
        ID = counter;  
    }
};

#endif // TRANSACTION_HPP
