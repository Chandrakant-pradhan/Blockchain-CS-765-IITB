#include "link.hpp"

void Link::setD() {
    //dont know how to do it
    this->d = randomExp(96/c);
}

ld Link::getTotalDelay(int msgSize) {
    return p + msgSize/d + c;
}