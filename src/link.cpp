#include "link.hpp"

void Link::setD() {
    //dont know how to do it
}

ld Link::getTotalDelay(int msgSize) {
    return p + msgSize/d + c;
}