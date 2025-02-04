#ifndef LINK_HPP
#define LINK_HPP

#include "headers.hpp"  

class Link {
public:
    Peer* Other;    // Pointer to the other peer connected by this link
    ld c;           // Link speed in bits per second (c_ij)
    ld d;           // Queuing delay at node i (d_ij)
    ld p;           // Propagation delay (ρ_ij)

    Link(Peer* other, ld speed, ld minDelay);
    ld getLatency(ld messageLength);
    void setP();
    void setD();
    void setC(bool isPeer1Fast, bool isPeer2Fast);
};

#endif // LINK_HPP
