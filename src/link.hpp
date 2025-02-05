#ifndef LINK_HPP
#define LINK_HPP

#include "headers.hpp"  

class Link {
public:
   Peer* other;
   ld p;
   ld d;
   ld c;
   Link(Peer* other, ld p, ld c) : other(other), p(p), c(c) {}
   void setD();
   ld getTotalDelay(int msgSize);
};

#endif // LINK_HPP
