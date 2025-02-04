#ifndef LINK_HPP
#define LINK_HPP

#include "headers.hpp"  

class Link {
public:
   Peer* Other;
   ld p;
   ld d;
   ld c;
   Link(Peer* other, ld p, ld c) : Other(other), p(p), c(c) {}
   void setD();
   void getTotalDelay(int msgSize);
};

#endif // LINK_HPP
