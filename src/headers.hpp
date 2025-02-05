#ifndef HEADERS_HPP
#define HEADERS_HPP

#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <set>
#include <vector>  
#include <thread>

// Type aliases
typedef long long ll;
typedef unsigned int uint;
typedef unsigned long long ull;
typedef long double ld;

extern ld globalTime; 

extern std::mt19937 rng;

// Generates a random integer in the range [min, max]
inline int randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

// Generates a random float in the range [min, max]
inline double randomFloat(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

inline ld randomExp(ld Ttx) {
    std::exponential_distribution<ld> dist(1.0 / Ttx);
    return dist(rng);
}


// Forward declarations of classes
class Transaction;
class Block;
class Blockchain;
class Link;
class Event;
class Peer;

// Corrected forward declarations for derived classes
class CreateTxnEvent;  
class ForwardTransaction;   
class ReceiveTransaction;   
class ReceiveBlock;   
class ForwardBlock;   
class BroadcastBlock;   

#endif // HEADERS_HPP