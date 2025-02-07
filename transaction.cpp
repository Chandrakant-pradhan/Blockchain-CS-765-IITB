#include "defs.hpp"

int Transaction::counter=0;


Transaction::Transaction(ld ts, int sID, int rID, int a) : timestamp(ts), senderID(sID), receiverID(rID), amount(a) {
    ID = counter++;
}
Transaction* Transaction::make_copy() {
    Transaction* t = new Transaction(timestamp, senderID, receiverID, amount);
    // here check once is it ok
    t->ID = ID;
    return t;
}