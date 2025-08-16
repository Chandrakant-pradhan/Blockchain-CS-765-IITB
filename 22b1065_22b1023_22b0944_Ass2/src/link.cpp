#include "defs.hpp"


//initializing class
Link::Link(ld pij, ld cij) : pij(pij), cij(cij), dij(0.0) {
    compute_queue_delay();
}


void Link::compute_queue_delay() {
    static random_device rd;
    static mt19937 gen(rd());
    ld mean = 96.0 / cij;
    exponential_distribution<ld> expDist(1.0 / mean);
    dij = expDist(gen);
}

ld Link::getTotalDelay(ld msgSize) {
    compute_queue_delay();
    ld totalDelay = pij + (ld)msgSize / cij + dij;
    return totalDelay;
}
