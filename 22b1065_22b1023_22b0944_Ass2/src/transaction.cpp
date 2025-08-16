#include "defs.hpp"


int Transaction::counter=0;

//instantiating class
Transaction::Transaction(ld ts, int sID, int rID, int a) : timestamp(ts), senderID(sID), receiverID(rID), amount(a) {
    ID = counter++;
    ld totalBits = 8 * (sizeof(timestamp) + sizeof(senderID) + sizeof(receiverID) + sizeof(amount));
    size = (totalBits + 999) / 1000;
}

//function to make copy of a txn object
Transaction* Transaction::make_copy() {
    Transaction* t = new Transaction(timestamp, senderID, receiverID, amount);
    t->ID = ID;
    return t;
}