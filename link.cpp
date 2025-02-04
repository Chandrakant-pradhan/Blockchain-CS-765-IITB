#include "link.hpp"

void Link::setD() {
    //dont know how to do it
}

int Link::getTotalDelay(int msgSize) {
    return p + msgSize/d + c;
}